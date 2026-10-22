#include "task_graph_resource.h"
#include "entry_fill_task_resource.h"
#include "metadata_task_resource.h"
#include "query_task_resource.h"
#include "sub_graph_task_resource.h"
#include "transform_task_resource.h"

#include "../../core/tasks/registration/ideam_task_registry.h"
#include "../../core/tasks/registration/metadata_task_registry.h"
#include "../../core/tasks/registration/query_task_registry.h"
#include "../../core/tasks/registration/transform_task_registry.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>
#include <vector>

namespace ideam::godot_ext {

namespace {

/**
 * Translates GrantPartResource UI access mode (0 = Read, 1 = Write) or raw enum values
 * into core::BufferAccessMode (1 = READ, 2 = WRITE, 3 = READ_WRITE).
 */
inline core::BufferAccessMode resolve_access_mode(int p_ui_access_mode) noexcept {
    switch (p_ui_access_mode) {
        case 0:  return core::BufferAccessMode::READ;
        case 1:  return core::BufferAccessMode::WRITE;
        case 2:  return core::BufferAccessMode::WRITE;
        case 3:  return core::BufferAccessMode::READ_WRITE;
        default: return core::BufferAccessMode::READ;
    }
}

} // namespace

void TaskGraphResource::_bind_methods() {
    using namespace godot;

    // Bind Command Arena Configurations
    ClassDB::bind_method(D_METHOD("set_command_arena_capacity_bytes", "bytes"), &TaskGraphResource::set_command_arena_capacity_bytes);
    ClassDB::bind_method(D_METHOD("get_command_arena_capacity_bytes"), &TaskGraphResource::get_command_arena_capacity_bytes);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "command_arena_capacity_bytes"), "set_command_arena_capacity_bytes", "get_command_arena_capacity_bytes");

    ClassDB::bind_method(D_METHOD("set_selection_queue_capacity_elements", "elements"), &TaskGraphResource::set_selection_queue_capacity_elements);
    ClassDB::bind_method(D_METHOD("get_selection_queue_capacity_elements"), &TaskGraphResource::get_selection_queue_capacity_elements);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "selection_queue_capacity_elements"), "set_selection_queue_capacity_elements", "get_selection_queue_capacity_elements");

    // --- Bind Explicit Profiles ---
    ClassDB::bind_method(D_METHOD("set_exec_buffer", "profile"), &TaskGraphResource::set_exec_buffer);
    ClassDB::bind_method(D_METHOD("get_exec_buffer"), &TaskGraphResource::get_exec_buffer);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "exec_buffer", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferResource"), "set_exec_buffer", "get_exec_buffer");

    ClassDB::bind_method(D_METHOD("set_cmd_arena_buffer", "profile"), &TaskGraphResource::set_cmd_arena_buffer);
    ClassDB::bind_method(D_METHOD("get_cmd_arena_buffer"), &TaskGraphResource::get_cmd_arena_buffer);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "cmd_arena_buffer", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferResource"), "set_cmd_arena_buffer", "get_cmd_arena_buffer");

    ClassDB::bind_method(D_METHOD("set_sel_arena_buffer", "profile"), &TaskGraphResource::set_sel_arena_buffer);
    ClassDB::bind_method(D_METHOD("get_sel_arena_buffer"), &TaskGraphResource::get_sel_arena_buffer);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sel_arena_buffer", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferResource"), "set_sel_arena_buffer", "get_sel_arena_buffer");
}

void TaskGraphResource::set_command_arena_capacity_bytes(int p_bytes) {
    if (p_bytes == command_arena_capacity_bytes) return;
    command_arena_capacity_bytes = p_bytes;
    if (get_is_volatile()) queue_update_managed_buffers();
    emit_changed();
}
int TaskGraphResource::get_command_arena_capacity_bytes() const { return command_arena_capacity_bytes; }

void TaskGraphResource::set_selection_queue_capacity_elements(int p_elements) {
    if (p_elements == selection_queue_capacity_elements) return;
    selection_queue_capacity_elements = p_elements;
    if (get_is_volatile()) queue_update_managed_buffers();
    emit_changed();
}
int TaskGraphResource::get_selection_queue_capacity_elements() const { return selection_queue_capacity_elements; }

void TaskGraphResource::set_exec_buffer(const godot::Ref<ManagedBufferResource>& p_buffer) { 
    if (p_buffer == exec_buffer) return;
    exec_buffer = p_buffer; 
    emit_changed(); 
}
godot::Ref<ManagedBufferResource> TaskGraphResource::get_exec_buffer() const { return exec_buffer; }

void TaskGraphResource::set_cmd_arena_buffer(const godot::Ref<ManagedBufferResource>& p_buffer) { 
    if (p_buffer == cmd_arena_buffer) return;
    cmd_arena_buffer = p_buffer; 
    emit_changed(); 
}
godot::Ref<ManagedBufferResource> TaskGraphResource::get_cmd_arena_buffer() const { return cmd_arena_buffer; }

void TaskGraphResource::set_sel_arena_buffer(const godot::Ref<ManagedBufferResource>& p_buffer) { 
    if (p_buffer == sel_arena_buffer) return;
    sel_arena_buffer = p_buffer; 
    emit_changed(); 
}
godot::Ref<ManagedBufferResource> TaskGraphResource::get_sel_arena_buffer() const { return sel_arena_buffer; }

void TaskGraphResource::_ensure_managed_buffers() {
    // 1. Parent MemoryGraphResource updates its structures
    MemoryGraphResource::_ensure_managed_buffers();

    // Retrieve the attached MemoryManagerResource from base class
    godot::Ref<MemoryManagerResource> mgr = get_memory_manager();
    if (mgr.is_null()) {
        return;
    }

    constexpr int ALIGNMENT = 64;
    constexpr int PAGE_SIZE = 4096; 
    int node_cap = get_is_volatile() ? std::max(static_cast<int>(get_nodes().size()), get_volatile_node_capacity()) : static_cast<int>(get_nodes().size());

    // --- Profile 5: TaskGraph Execution State Arrays (SoA) ---
    int exec_bytes = node_cap * (
        sizeof(core::TaskTypeDOD) + 
        sizeof(core::TaskCPUMetadata) + 
        sizeof(core::TaskGPUMetadata) + 
        sizeof(uint32_t) + 
        (sizeof(std::vector<core::TaskPortMetadata>) * 3) + 
        sizeof(std::vector<core::TaskPortConnectionDOD>)
    );
    int padded_exec_bytes = (exec_bytes + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
    // Ensure non-zero footprint for telemetry when graph has 0 nodes
    if (padded_exec_bytes == 0) {
        padded_exec_bytes = ALIGNMENT;
    }

    // --- Profile 6: Structural Command Arena ---
    int padded_cmd_arena = ((command_arena_capacity_bytes + (PAGE_SIZE - 1)) / PAGE_SIZE) * PAGE_SIZE;

    // --- Profile 7: Selection Command Queue ---
    int sel_bytes = selection_queue_capacity_elements * sizeof(int64_t);
    int padded_sel_arena = ((sel_bytes + (PAGE_SIZE - 1)) / PAGE_SIZE) * PAGE_SIZE;

    // Request buffers from the manager rather than instantiating locally
    exec_buffer = mgr->request_managed_buffer(
        get_consumer_key(),
        godot::StringName("Task Execution State (SoA)"),
        static_cast<int>(core::BufferLayoutType::FLAT),
        padded_exec_bytes,
        ALIGNMENT
    );

    cmd_arena_buffer = mgr->request_managed_buffer(
        get_consumer_key(),
        godot::StringName("Structural Command Arena"),
        static_cast<int>(core::BufferLayoutType::PAGED),
        padded_cmd_arena,
        ALIGNMENT
    );

    sel_arena_buffer = mgr->request_managed_buffer(
        get_consumer_key(),
        godot::StringName("Selection Command Queue"),
        static_cast<int>(core::BufferLayoutType::PAGED),
        padded_sel_arena,
        ALIGNMENT
    );
}

void TaskGraphResource::_gather_managed_buffers(godot::TypedArray<ManagedBufferResource>& r_managed_buffers) const {
    // 1. Pack the Memory Graph profiles (and by extension, the base Graph profiles)
    MemoryGraphResource::_gather_managed_buffers(r_managed_buffers);

    // 2. Pack the specific Task Graph profiles
    if (exec_buffer.is_valid()) r_managed_buffers.append(exec_buffer);
    if (cmd_arena_buffer.is_valid()) r_managed_buffers.append(cmd_arena_buffer);
    if (sel_arena_buffer.is_valid()) r_managed_buffers.append(sel_arena_buffer);
}

void TaskGraphResource::sync_execution_snapshots(
    const std::shared_ptr<core::TaskGraphDOD>& p_graph,
    const godot::HashMap<godot::StringName, core::NodeID>& p_ui_to_dod_map) 
{
    if (!p_graph) return;

    godot::TypedArray<godot::Ref<IdeamGraphNodeResource>> current_nodes = get_nodes();

    for (int i = 0; i < current_nodes.size(); ++i) {
        godot::Ref<TaskResource> task_res = current_nodes[i];
        if (task_res.is_null() || !task_res->get_is_active()) continue;

        godot::StringName ui_name = task_res->get_node_name();
        if (!p_ui_to_dod_map.has(ui_name)) continue;

        core::NodeID dod_id = p_ui_to_dod_map[ui_name];

        // Forward any newly requested UI snapshots down to the native task instance
        if (task_res->is_snapshot_requested()) {
            p_graph->set_task_max_preview_size(dod_id, task_res->get_max_preview_size());
            p_graph->request_task_image_snapshot(dod_id);
            task_res->clear_snapshot_request();
        }

        godot::Ref<godot::Image> snapshot = p_graph->get_task_image_snapshot(dod_id);
        if (snapshot.is_valid() && !snapshot->is_empty()) {
            task_res->set_snapshot_image(snapshot);
        }
    }
}

std::shared_ptr<core::TaskGraphDOD> TaskGraphResource::compile_to_task_graph(
    core::MemoryManagerDOD* p_manager, 
    godot::HashMap<godot::StringName, core::NodeID>& r_ui_to_dod_map) const 
{
    auto task_graph = std::make_shared<core::TaskGraphDOD>(p_manager);
    
    godot::TypedArray<godot::Ref<IdeamGraphNodeResource>> current_nodes = get_nodes();
    godot::TypedArray<godot::Dictionary> current_edges = get_edges();

    task_graph->reserve(current_nodes.size(), current_edges.size());
    r_ui_to_dod_map.clear();

    // 1. Node Compilation & Memory Requirement Registration
    for (int i = 0; i < current_nodes.size(); ++i) {
        godot::Ref<TaskResource> n = current_nodes[i];
        
        // Skip null instances or deactivated nodes
        if (!n.is_valid() || !n->get_is_active()) continue;

        godot::StringName ui_name = n->get_node_name();
        if (ui_name.is_empty()) continue;
        
        core::TaskTypeDOD type_dod = static_cast<core::TaskTypeDOD>(n->get_task_type());
        
        core::NodeID core_id = task_graph->add_task_node(type_dod);
        task_graph->set_node_priority(core_id, n->get_execution_priority());
        r_ui_to_dod_map[ui_name] = core_id;

        // --- A. Compile Independent Memory Grants ---
        std::vector<core::GrantPartPOD> dod_parts;
        if (n->get_derivation_mode() == MemoryGraphNodeResource::MODE_INDEPENDENT) {
            godot::Ref<MemoryGrantResource> grant_res = n->get_memory_grant();
            if (grant_res.is_valid()) {
                godot::TypedArray<GrantPartResource> ui_parts = grant_res->get_configured_parts();
                dod_parts.reserve(ui_parts.size());

                for (int p = 0; p < ui_parts.size(); ++p) {
                    godot::Ref<GrantPartResource> part_res = ui_parts[p];
                    if (!part_res.is_valid()) continue;

                    core::GrantPartPOD pod_part{};
                    pod_part.buffer_id = static_cast<uint32_t>(part_res->get_buffer_id());
                    pod_part.element_stride = static_cast<uint32_t>(part_res->get_element_stride());
                    pod_part.column_id = static_cast<uint32_t>(part_res->get_column_id());
                    pod_part.access_mode = resolve_access_mode(part_res->get_access_mode());
                    pod_part.is_contiguous = part_res->get_is_contiguous();

                    godot::Ref<MemoryBufferSelectionResource> sel_res = part_res->get_selection();
                    if (sel_res.is_valid()) {
                        pod_part.selection.mode = static_cast<core::SelectionMode>(sel_res->get_mode());
                        pod_part.selection.element_count = sel_res->get_element_count();
                        pod_part.selection.start_index = sel_res->get_start_index();
                    } else if (part_res->get_is_contiguous()) {
                        pod_part.selection.mode = core::SelectionMode::RANGE;
                    }

                    if (p_manager) {
                        if (const core::MemoryBufferPOD* buf = p_manager->get_buffer(pod_part.buffer_id)) {
                            pod_part.selection.target_buffer_id = pod_part.buffer_id;
                            pod_part.selection.capacity = buf->max_elements;
                            if (pod_part.element_stride == 0) {
                                pod_part.element_stride = (buf->element_stride > 0) ? buf->element_stride : 1;
                            }
                            if (pod_part.selection.mode == core::SelectionMode::RANGE && pod_part.selection.element_count == 0) {
                                pod_part.selection.element_count = buf->max_elements;
                            }
                        }
                    }

                    dod_parts.push_back(pod_part);
                }

                if (!dod_parts.empty()) {
                    task_graph->set_node_requirements(core_id, dod_parts);
                }
            }
        }

        // --- B. Prepare Task Properties ---
        godot::Dictionary props = n->get_task_properties().duplicate();

        if (n->get_functional_texture().is_valid() &&
            !props.has("target_image") && !props.has("source_image") && !props.has("image")) {
            props["target_image"] = n->get_functional_texture();
        }

        // Provide primary buffer_id fallback from the node's first GrantPart when not explicitly set in props
        if (!dod_parts.empty()) {
            const int64_t primary_buf_id = static_cast<int64_t>(dod_parts[0].buffer_id);
            if (!props.has("buffer_id")) {
                props["buffer_id"] = primary_buf_id;
            }
            if (!props.has("target_buffer_id")) {
                props["target_buffer_id"] = primary_buf_id;
            }
            if (!props.has("grid_buffer_id")) {
                props["grid_buffer_id"] = primary_buf_id;
            }
            if (!props.has("primary_buffer_id")) {
                props["primary_buffer_id"] = primary_buf_id;
            }
        }

        // --- C. Sub-Graph Recursive Compilation ---
        godot::Ref<SubGraphTaskResource> sub_res = godot::Object::cast_to<SubGraphTaskResource>(n.ptr());
        if (sub_res.is_valid()) {
            godot::Ref<TaskGraphResource> child_res = sub_res->get_child_graph();
            if (child_res.is_valid()) {
                godot::HashMap<godot::StringName, core::NodeID> child_map;
                
                std::shared_ptr<core::TaskGraphDOD> compiled_child = child_res->compile_to_task_graph(p_manager, child_map);
                task_graph->retain_child_graph(compiled_child);
                props["child_graph"] = static_cast<int64_t>(reinterpret_cast<uintptr_t>(compiled_child.get()));
                
                godot::PackedInt32Array compiled_mappings;
                godot::Dictionary authoring_mappings = sub_res->get_grant_mappings();
                godot::Array keys = authoring_mappings.keys();
                
                for (int m = 0; m < keys.size(); ++m) {
                    int parent_buffer_id = keys[m];
                    godot::StringName child_name = authoring_mappings[parent_buffer_id];
                    
                    if (child_map.has(child_name)) {
                        compiled_mappings.push_back(parent_buffer_id);
                        compiled_mappings.push_back(child_map[child_name]);
                    } else {
                        godot::UtilityFunctions::printerr("TaskGraph Compiler: Mapped child node '", child_name, "' not found in compiled sub-graph!");
                    }
                }
                props["grant_mappings"] = compiled_mappings;
            }
        }

        // --- D. Task Execution Interface Instantiation ---
        switch (type_dod) {
            case core::TaskTypeDOD::NATIVE_CPU:
            case core::TaskTypeDOD::QUERY_CULLER: {
                std::unique_ptr<core::INativeTask> native_interface = nullptr;

                if (auto* transform_res = godot::Object::cast_to<TransformTaskResource>(n.ptr())) {
                    native_interface = core::TransformTaskRegistry::create(
                        transform_res->get_logic_id(),
                        static_cast<uint32_t>(transform_res->get_view_id()),
                        static_cast<uint32_t>(transform_res->get_strategy_id()),
                        static_cast<uint32_t>(transform_res->get_type_id())
                    );
                } else if (auto* query_res = godot::Object::cast_to<QueryTaskResource>(n.ptr())) {
                    native_interface = core::QueryTaskRegistry::create(
                        static_cast<uint32_t>(query_res->get_op_id()),
                        query_res->get_logic_id(),
                        static_cast<uint32_t>(query_res->get_view_id()),
                        static_cast<uint32_t>(query_res->get_strategy_id()),
                        static_cast<uint32_t>(query_res->get_type_id())
                    );
                } else if (auto* meta_res = godot::Object::cast_to<MetadataTaskResource>(n.ptr())) {
                    native_interface = core::MetadataTaskRegistry::create(
                        meta_res->get_logic_id(),
                        static_cast<uint32_t>(meta_res->get_view_id()),
                        static_cast<uint32_t>(meta_res->get_strategy_id()),
                        static_cast<uint32_t>(meta_res->get_type_id())
                    );
                } else if (godot::Object::cast_to<EntryFillTaskResource>(n.ptr()) != nullptr) {
                    if (core::IdeamTaskRegistry* registry = core::IdeamTaskRegistry::get_singleton()) {
                        native_interface = registry->create(godot::StringName("Entry Fill"));
                    }
                } else if (sub_res.is_valid()) {
                    if (core::IdeamTaskRegistry* registry = core::IdeamTaskRegistry::get_singleton()) {
                        native_interface = registry->create(godot::StringName("Sub-Graph"));
                    }
                } else if (props.has("native_class")) {
                    godot::StringName native_class = props["native_class"];
                    if (core::IdeamTaskRegistry* registry = core::IdeamTaskRegistry::get_singleton()) {
                        native_interface = registry->create(native_class);
                    }
                } else if (!n->get_task_name().is_empty()) {
                    if (core::IdeamTaskRegistry* registry = core::IdeamTaskRegistry::get_singleton()) {
                        native_interface = registry->create(n->get_task_name());
                    }
                }

                if (native_interface) {
                    native_interface->apply_properties(props);
                    native_interface->set_max_preview_size(n->get_max_preview_size());
                    if (n->is_snapshot_requested()) {
                        native_interface->request_snapshot();
                    }

                    core::TaskContextPOD query_ctx{ 0.0, nullptr, p_manager, nullptr, nullptr, nullptr };
                    uint32_t transient_req = static_cast<uint32_t>(native_interface->get_transient_requirement(query_ctx));
                    if (props.has("transient_bytes")) {
                        transient_req = std::max(transient_req, static_cast<uint32_t>(static_cast<int64_t>(props["transient_bytes"])));
                    }
                    if (transient_req > 0) {
                        task_graph->set_node_transient_requirement(core_id, transient_req);
                    }

                    task_graph->configure_native_interface(core_id, std::move(native_interface));
                } else {
                    godot::UtilityFunctions::printerr(
                        "TaskGraph Compiler: Unable to instantiate native task for node '",
                        ui_name, "' (class: ", n->get_class(), ", logic_id: ", n->get_logic_id(), ")"
                    );
                }
                break;
            }
            case core::TaskTypeDOD::GODOT_REFLECTION: {
                if (props.has("reflection_target") && props.has("execution_method")) {
                    godot::Object* target_obj = props["reflection_target"];
                    godot::StringName method_name = props["execution_method"];
                    task_graph->configure_cpu_task(core_id, target_obj, method_name);
                }
                break;
            }
            default:
                break;
        }
    }

    // 2. Edge Compilation
    for (int i = 0; i < current_edges.size(); ++i) {
        godot::Dictionary e = current_edges[i];
        if (!e.has("from") || !e.has("to")) continue;

        godot::StringName from_name = e["from"];
        godot::StringName to_name = e["to"];

        if (r_ui_to_dod_map.has(from_name) && r_ui_to_dod_map.has(to_name)) {
            core::NodeID from_id = r_ui_to_dod_map[from_name];
            core::NodeID to_id = r_ui_to_dod_map[to_name];

            uint32_t from_port = e.has("from_port") ? static_cast<uint32_t>(static_cast<int64_t>(e["from_port"])) : 0;
            uint32_t to_port = e.has("to_port") ? static_cast<uint32_t>(static_cast<int64_t>(e["to_port"])) : 0;

            task_graph->connect_nodes(from_id, from_port, to_id, to_port);
        }
    }
    
    // 3. Command Arena Allocation & Kahn Wave Topological Sort
    task_graph->configure_command_arenas(
        command_arena_capacity_bytes, 
        selection_queue_capacity_elements * sizeof(int64_t)
    );

    task_graph->defragment();

    // 4. Rebuild r_ui_to_dod_map after defragmentation compaction & bake initial grants
    r_ui_to_dod_map.clear();
    core::NodeID compacted_id = 0;
    for (int i = 0; i < current_nodes.size(); ++i) {
        godot::Ref<TaskResource> n = current_nodes[i];
        if (!n.is_valid() || !n->get_is_active() || n->get_node_name().is_empty()) continue;
        r_ui_to_dod_map[n->get_node_name()] = compacted_id++;
    }

    // Validate independent grants first so parent leases are active before forking
    task_graph->validate_grants();

    // 5. Apply Forked Grants (MODE_FORKED) along topological edges
    for (int i = 0; i < current_edges.size(); ++i) {
        godot::Dictionary e = current_edges[i];
        if (!e.has("from") || !e.has("to")) continue;

        godot::StringName from_name = e["from"];
        godot::StringName to_name = e["to"];

        if (r_ui_to_dod_map.has(from_name) && r_ui_to_dod_map.has(to_name)) {
            godot::Ref<MemoryGraphNodeResource> to_res = _get_node_by_name(to_name);
            if (to_res.is_valid() && to_res->get_derivation_mode() == MemoryGraphNodeResource::MODE_FORKED) {
                task_graph->fork_grant(r_ui_to_dod_map[from_name], r_ui_to_dod_map[to_name]);
            }
        }
    }

    return task_graph;
}

} // namespace ideam::godot_ext