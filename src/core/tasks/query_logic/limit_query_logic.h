#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <algorithm>
#include <cstring>
#include <bit>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace ideam::core {

/**
 * LimitQueryLogic
 * Caps the number of active elements in a selection to a fixed limit.
 * MAGIC: Add mode acts as an exact-count allocator by scanning the Availability Mask.
 */
struct LimitQueryLogic {
    using ValueType       = uint8_t; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<uint8_t, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY;
    static constexpr DataType required_types              = DataType::ANY;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;

    static constexpr size_t transient_workspace_bytes     = 0;
    
    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    static constexpr std::string_view display_name = "Limit";
    
    uint32_t target_buffer_id = 0;
    int64_t limit = 0;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary limit_prop;
        limit_prop["name"] = "limit";
        limit_prop["type"] = godot::Variant::INT;
        limit_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(limit_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("limit")) {
            limit = static_cast<int64_t>(p_props["limit"]);
        }
    }
    
    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if (limit <= 0) return;

        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.element_count <= limit) return;

            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection);
            else _cull_sparse(r_selection);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_limit(r_selection, p_context);
        }
    }

private:
    void _cull_dense(MemoryBufferSelectionPOD& r_selection) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t word_count = (r_selection.capacity + 63) >> 6;
        
        int64_t found_count = 0;

        for (int64_t i = 0; i < word_count; ++i) {
            if (bitset[i] == 0) continue;

#if defined(_MSC_VER)
            int64_t word_pop = __popcnt64(bitset[i]);
#else
            int64_t word_pop = __builtin_popcountll(bitset[i]);
#endif

            if (found_count + word_pop > limit) {
                int64_t needed = limit - found_count;
                uint64_t mask = bitset[i];
                
                while (needed > 0) {
                    mask &= (mask - 1); 
                    needed--;
                }
                
                bitset[i] ^= mask; 
                
                if (i + 1 < word_count) {
                    std::memset(bitset + i + 1, 0, (word_count - (i + 1)) * sizeof(uint64_t));
                }
                
                r_selection.element_count = limit;
                return;
            }

            found_count += word_pop;
        }
    }

    void _cull_sparse(MemoryBufferSelectionPOD& r_selection) const {
        // Sparse Truncation is simply reducing the element count boundary.
        r_selection.element_count = limit;
    }

    void _add_limit(const MemoryBufferSelectionPOD& r_selection, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        int64_t added_count = 0;
        const int64_t words = (r_selection.capacity + 63) >> 6;

        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0 && added_count < limit) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                p_ctx.queue_selection_command(target_buffer_id, global_index);
                added_count++;
                
                mask &= (mask - 1); 
            }
            if (added_count >= limit) break;
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_LIMIT_QUERY_LOGIC_H