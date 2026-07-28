#include "task_graph_resource.h"
#include "sub_graph_task_resource.h"

#include "../../core/tasks/registration/ideam_task_registry.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>

namespace ideam::godot_ext {

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
    if(p_buffer == exec_buffer) return;
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
    // 1. Let the parent MemoryGraphResource instantiate/update its structures
    MemoryGraphResource::_ensure_managed_buffers();

    constexpr int ALIGNMENT = 64;
    constexpr int PAGE_SIZE = 4096; 
    int node_cap = get_is_volatile() ? std::max(static_cast<int>(get_nodes().size()), get_volatile_node_capacity()) : static_cast<int>(get_nodes().size());

    // --- Profile 5: TaskGraph Execution State Arrays (SoA) ---
    int exec_bytes = node_cap * (
        sizeof(core::TaskTypeDOD) + 
        sizeof(core::TaskCPUMetadata) + 
        sizeof(core::TaskGPUMetadata) + 
        sizeof(uint32_t) + // transient bytes
        (sizeof(std::vector<core::TaskPortMetadata>) * 3) + // I/O/Constant port vectors
        sizeof(std::vector<core::TaskPortConnectionDOD>)
    );
    int padded_exec_bytes = (exec_bytes + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    if (exec_buffer.is_null()) {
        exec_buffer.instantiate();
        exec_buffer->set_purpose("Task Execution State (SoA)");
        exec_buffer->set_layout_type(static_cast<int>(core::BufferLayoutType::FLAT));
        exec_buffer->set_alignment(ALIGNMENT);
    }
    
    exec_buffer->set_consumer_name(get_consumer_key());
    exec_buffer->set_byte_footprint(padded_exec_bytes);

    // --- Profile 6: Structural Command Arena ---
    int padded_cmd_arena = ((command_arena_capacity_bytes + (PAGE_SIZE - 1)) / PAGE_SIZE) * PAGE_SIZE;
    
    if (cmd_arena_buffer.is_null()) {
        cmd_arena_buffer.instantiate();
        cmd_arena_buffer->set_purpose("Structural Command Arena");
        cmd_arena_buffer->set_layout_type(static_cast<int>(core::BufferLayoutType::PAGED)); 
        cmd_arena_buffer->set_alignment(ALIGNMENT);
    }
    
    cmd_arena_buffer->set_consumer_name(get_consumer_key());
    cmd_arena_buffer->set_byte_footprint(padded_cmd_arena);

    // --- Profile 7: Selection Command Queue ---
    int sel_bytes = selection_queue_capacity_elements * sizeof(int64_t);
    int padded_sel_arena = ((sel_bytes + (PAGE_SIZE - 1)) / PAGE_SIZE) * PAGE_SIZE;

    if (sel_arena_buffer.is_null()) {
        sel_arena_buffer.instantiate();
        sel_arena_buffer->set_purpose("Selection Command Queue");
        sel_arena_buffer->set_layout_type(static_cast<int>(core::BufferLayoutType::PAGED)); 
        sel_arena_buffer->set_alignment(ALIGNMENT);
    }
    
    sel_arena_buffer->set_consumer_name(get_consumer_key());
    sel_arena_buffer->set_byte_footprint(padded_sel_arena);
}

void TaskGraphResource::_gather_managed_buffers(godot::TypedArray<ManagedBufferResource>& r_managed_buffers) const {
    // 1. Pack the Memory Graph profiles (and by extension, the base Graph profiles)
    MemoryGraphResource::_gather_managed_buffers(r_managed_buffers);

    // 2. Pack the specific Task Graph profiles
    if (exec_buffer.is_valid()) r_managed_buffers.append(exec_buffer);
    if (cmd_arena_buffer.is_valid()) r_managed_buffers.append(cmd_arena_buffer);
    if (sel_arena_buffer.is_valid()) r_managed_buffers.append(sel_arena_buffer);
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

    // Fast-path node compilation
    for (int i = 0; i < current_nodes.size(); ++i) {
        godot::Ref<TaskResource> n = current_nodes[i];
        
        // Skip null instances or pruned nodes directly via strongly-typed virtual call
        if (!n.is_valid() || !n->get_is_active()) continue;

        godot::StringName ui_name = n->get_node_name();
        if (ui_name.is_empty()) continue;
        
        core::TaskTypeDOD type_dod = static_cast<core::TaskTypeDOD>(n->get_task_type());
        
        core::NodeID core_id = task_graph->add_task_node(type_dod);
        r_ui_to_dod_map[ui_name] = core_id;

        godot::Dictionary props = n->get_task_properties();
        
        // --- SUB-GRAPH RECURSIVE COMPILATION ---
        // If this node is a SubGraph, we must eagerly compile its child topology
        // and securely pass the raw pointer down to the native Task instance.
        godot::Ref<SubGraphTaskResource> sub_res = godot::Object::cast_to<SubGraphTaskResource>(n.ptr());
        if (sub_res.is_valid()) {
            godot::Ref<TaskGraphResource> child_res = sub_res->get_child_graph();
            if (child_res.is_valid()) {
                godot::HashMap<godot::StringName, core::NodeID> child_map;
                
                std::shared_ptr<core::TaskGraphDOD> compiled_child = child_res->compile_to_task_graph(p_manager, child_map);
                task_graph->retain_child_graph(compiled_child);
                props["child_graph"] = static_cast<int64_t>(reinterpret_cast<uintptr_t>(compiled_child.get()));
                
                // --- DOD Identity Translation ---
                // The Authoring layer safely tracks nodes via StringName. 
                // We crush them down to strict NodeIDs here for the execution struct.
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

        switch (type_dod) {
            case core::TaskTypeDOD::NATIVE_CPU: {
                if (props.has("native_class")) {
                    godot::StringName native_class = props["native_class"];
                    core::IdeamTaskRegistry* registry = core::IdeamTaskRegistry::get_singleton();
                    auto native_interface = registry->create(native_class);
                        
                    if (native_interface) {
                        native_interface->apply_properties(props);
                        task_graph->configure_native_interface(core_id, std::move(native_interface));
                    } else {
                        godot::UtilityFunctions::printerr("TaskGraph Compiler: Unable to find registered native task '", native_class, "' for node '", ui_name, "'");
                    }
                }
                break;
            }
            default: break;
        }
    }

    // Edge Compilation (Still parsing AoS Dicts)
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
    
    task_graph->configure_command_arenas(
        command_arena_capacity_bytes, 
        selection_queue_capacity_elements * sizeof(int64_t)
    );

    task_graph->defragment();

    return task_graph;
}

} // namespace ideam::godot_ext