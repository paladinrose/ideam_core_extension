#pragma once

#include "i_native_task.h"
#include "task_graph_dod.h"
#include "task_graph_command_pod.h"
#include <vector>

namespace ideam::core {

/**
 * BubbleSelectionCommand
 * The payload tasks inside the SubGraph push to their Tier 1 TaskGraphCommandPOD
 * when they want the Parent Graph to expand a selection.
 */
struct BubbleSelectionCommand {
    static constexpr uint32_t COMMAND_SIGNATURE = 0x5E1EC710; // "SELECTIO"
    uint32_t signature = COMMAND_SIGNATURE;
    uint32_t target_buffer_id;
    int64_t entity_index;
};

class alignas(64) SubGraphTask final : public INativeTask {
public:
    struct GrantMapping {
        uint32_t parent_buffer_id;
        NodeID child_node_id;
    };

private:
    TaskGraphDOD* child_graph = nullptr;
    std::vector<GrantMapping> memory_mappings;

public:
    explicit SubGraphTask(TaskGraphDOD* p_child_graph = nullptr) 
        : child_graph(p_child_graph) {}
    
    inline void apply_properties(const godot::Dictionary& p_props) override {
        // 1. Unbox the compiled graph pointer
        // Stored as an int64_t to safely bypass Variant's restriction on raw C++ pointers.
        if (p_props.has("child_graph")) {
            int64_t raw_ptr = static_cast<int64_t>(p_props["child_graph"]);
            child_graph = reinterpret_cast<TaskGraphDOD*>(raw_ptr);
        }

        // 2. Unpack the interleaved memory topography constraints
        if (p_props.has("grant_mappings")) {
            godot::PackedInt32Array packed = p_props["grant_mappings"];
            
            // Bypass Godot's Variant abstraction layer entirely to read the raw contiguous memory.
            const int32_t* data = packed.ptr();
            
            // Since the array is strictly interleaved [parent_id, child_node_id, ...], 
            // the true mapping count is exactly half the array size.
            size_t mapping_count = packed.size() / 2;
            
            // Pre-allocate the vector to prevent heap fragmentation and reallocation
            // during the continuous pushing phase.
            memory_mappings.clear();
            memory_mappings.reserve(mapping_count);
            
            for (size_t i = 0; i < mapping_count; ++i) {
                // Compute the flat offsets and cast directly to our strictly typed DOD scalars
                memory_mappings.push_back({
                    static_cast<uint32_t>(data[i * 2]), 
                    static_cast<NodeID>(data[(i * 2) + 1])
                });
            }
        }
    }

    inline void set_child_graph(TaskGraphDOD* p_child_graph) {
        child_graph = p_child_graph;
    }

    inline void add_grant_mapping(uint32_t p_parent_buffer_id, NodeID p_child_node_id) {
        memory_mappings.push_back({p_parent_buffer_id, p_child_node_id});
    }

    inline void prepare(const TaskContextPOD& p_context) override {
        if (!child_graph) [[unlikely]] return;
        child_graph->sync_with_manager();
    }

    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void execute(const TaskContextPOD& p_context) override {
        if (!child_graph || memory_mappings.empty()) [[unlikely]] return;

        // --- Phase 1: Zero-Copy Topology Aliasing ---
        for (const auto& map : memory_mappings) {
            const GrantPartPOD* parent_part = p_context.get_grant_part(map.parent_buffer_id);
            if (!parent_part) [[unlikely]] continue;

            // Synthesize a localized grant bypassing the manager's collision checks.
            // The parent has already paid the synchronization cost.
            MemoryGrantPOD injected_grant;
            injected_grant.parts[0] = *parent_part;
            injected_grant.part_count = 1;
            injected_grant.manager_version_at_issue = p_context.grant->manager_version_at_issue;
            injected_grant.global_manager_version_ptr = p_context.grant->global_manager_version_ptr;
            injected_grant.active = true;

            child_graph->inject_external_grant(map.child_node_id, injected_grant);
        }

        // --- Phase 2: Frame Protection ---
        // CRITICAL: The child graph's wave execution will call manager->reset_transient().
        // We MUST preserve the parent's lock-free bump offset, or the child will 
        // obliterate the transient workspaces of this task's siblings.
        size_t transient_mark = p_context.manager->get_transient_mark();

        // --- Phase 3: Synchronous Sub-Graph Execution ---
        child_graph->execute_graph_dod(p_context.delta);

        // --- Phase 4: Frame Restoration ---
        p_context.manager->restore_transient_mark(transient_mark);

        // --- Phase 5: Tier 1 -> Tier 2 Command Bubbling ---
        _harvest_child_commands(p_context);
    }

    inline void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {
        // If the parent Graph dynamically culls a selection, the SubGraphTask automatically 
        // passes that culled MemoryBufferSelectionPOD down during the next execute() phase.
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void _harvest_child_commands(const TaskContextPOD& p_context) {
        if (!p_context.wave_commands || !child_graph) return;

        // Iterate the child graph's Tier 1 command arenas (one per node)
        std::span<const TaskGraphCommandPOD> child_tier1 = child_graph->get_tier1_commands();
        
        for (const auto& arena : child_tier1) {
            if (arena.write_offset == 0) continue;

            // Linear scan through the raw byte bump-allocator
            uint32_t read_cursor = 0;
            while (read_cursor + sizeof(uint32_t) <= arena.write_offset) {
                // Peak at the signature to identify our Bubble command
                uint32_t signature = *reinterpret_cast<uint32_t*>(arena.arena_ptr + read_cursor);
                
                if (signature == BubbleSelectionCommand::COMMAND_SIGNATURE) {
                    if (read_cursor + sizeof(BubbleSelectionCommand) <= arena.write_offset) {
                        auto* cmd = reinterpret_cast<BubbleSelectionCommand*>(arena.arena_ptr + read_cursor);
                        
                        // Push to the parent's Tier 2 (Wave) command buffer
                        if (p_context.wave_commands->target_buffer_id == 0 || p_context.wave_commands->target_buffer_id == cmd->target_buffer_id) {
                            p_context.wave_commands->target_buffer_id = cmd->target_buffer_id;
                            p_context.wave_commands->push_addition(cmd->entity_index);
                        }
                        
                        read_cursor += sizeof(BubbleSelectionCommand);
                    } else {
                        break; // Corrupted or truncated command tail
                    }
                } else {
                    // If the child pushed other custom commands, we must gracefully skip them.
                    // Note: This requires all custom Tier 1 commands to have a consistent header size
                    // if you plan to mix and match. For now, assuming only Bubble commands exist.
                    break; 
                }
            }
        }
    }
};

} // namespace ideam::core