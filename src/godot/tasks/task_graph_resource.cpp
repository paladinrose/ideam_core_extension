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
    ClassDB::bind_method(D_METHOD("set_exec_profile", "profile"), &TaskGraphResource::set_exec_profile);
    ClassDB::bind_method(D_METHOD("get_exec_profile"), &TaskGraphResource::get_exec_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "exec_profile", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferProfile"), "set_exec_profile", "get_exec_profile");

    ClassDB::bind_method(D_METHOD("set_cmd_arena_profile", "profile"), &TaskGraphResource::set_cmd_arena_profile);
    ClassDB::bind_method(D_METHOD("get_cmd_arena_profile"), &TaskGraphResource::get_cmd_arena_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "cmd_arena_profile", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferProfile"), "set_cmd_arena_profile", "get_cmd_arena_profile");

    ClassDB::bind_method(D_METHOD("set_sel_arena_profile", "profile"), &TaskGraphResource::set_sel_arena_profile);
    ClassDB::bind_method(D_METHOD("get_sel_arena_profile"), &TaskGraphResource::get_sel_arena_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sel_arena_profile", PROPERTY_HINT_RESOURCE_TYPE, "ManagedBufferProfile"), "set_sel_arena_profile", "get_sel_arena_profile");
}

void TaskGraphResource::set_command_arena_capacity_bytes(int p_bytes) {
    if (p_bytes == command_arena_capacity_bytes) return;
    command_arena_capacity_bytes = p_bytes;
    if (get_is_volatile()) queue_update_managed_profiles();
    emit_changed();
}
int TaskGraphResource::get_command_arena_capacity_bytes() const { return command_arena_capacity_bytes; }

void TaskGraphResource::set_selection_queue_capacity_elements(int p_elements) {
    if (p_elements == selection_queue_capacity_elements) return;
    selection_queue_capacity_elements = p_elements;
    if (get_is_volatile()) queue_update_managed_profiles();
    emit_changed();
}
int TaskGraphResource::get_selection_queue_capacity_elements() const { return selection_queue_capacity_elements; }

void TaskGraphResource::set_exec_profile(const godot::Ref<ManagedBufferProfile>& p_profile) { 
    if(p_profile == exec_profile) return;
    exec_profile = p_profile; 
    emit_changed(); 
}
godot::Ref<ManagedBufferProfile> TaskGraphResource::get_exec_profile() const { return exec_profile; }

void TaskGraphResource::set_cmd_arena_profile(const godot::Ref<ManagedBufferProfile>& p_profile) { 
    if (p_profile == cmd_arena_profile) return;
    cmd_arena_profile = p_profile; 
    emit_changed(); 
}
godot::Ref<ManagedBufferProfile> TaskGraphResource::get_cmd_arena_profile() const { return cmd_arena_profile; }

void TaskGraphResource::set_sel_arena_profile(const godot::Ref<ManagedBufferProfile>& p_profile) { 
    if (p_profile == sel_arena_profile) return;
    sel_arena_profile = p_profile; 
    emit_changed(); 
}
godot::Ref<ManagedBufferProfile> TaskGraphResource::get_sel_arena_profile() const { return sel_arena_profile; }

void TaskGraphResource::_ensure_managed_profiles() {
    // 1. Let the parent MemoryGraphResource instantiate/update its structures
    MemoryGraphResource::_ensure_managed_profiles();

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

    if (exec_profile.is_null()) {
        exec_profile.instantiate();
        exec_profile->set_purpose("Task Execution State (SoA)");
        exec_profile->set_layout_type(static_cast<int>(core::BufferLayoutType::FLAT));
        exec_profile->set_alignment(ALIGNMENT);
    }
    
    exec_profile->set_consumer_name(get_consumer_key());
    exec_profile->set_byte_footprint(padded_exec_bytes);

    // --- Profile 6: Structural Command Arena ---
    int padded_cmd_arena = ((command_arena_capacity_bytes + (PAGE_SIZE - 1)) / PAGE_SIZE) * PAGE_SIZE;
    
    if (cmd_arena_profile.is_null()) {
        cmd_arena_profile.instantiate();
        cmd_arena_profile->set_purpose("Structural Command Arena");
        cmd_arena_profile->set_layout_type(static_cast<int>(core::BufferLayoutType::PAGED)); 
        cmd_arena_profile->set_alignment(ALIGNMENT);
    }
    
    cmd_arena_profile->set_consumer_name(get_consumer_key());
    cmd_arena_profile->set_byte_footprint(padded_cmd_arena);

    // --- Profile 7: Selection Command Queue ---
    int sel_bytes = selection_queue_capacity_elements * sizeof(int64_t);
    int padded_sel_arena = ((sel_bytes + (PAGE_SIZE - 1)) / PAGE_SIZE) * PAGE_SIZE;

    if (sel_arena_profile.is_null()) {
        sel_arena_profile.instantiate();
        sel_arena_profile->set_purpose("Selection Command Queue");
        sel_arena_profile->set_layout_type(static_cast<int>(core::BufferLayoutType::PAGED)); 
        sel_arena_profile->set_alignment(ALIGNMENT);
    }
    
    sel_arena_profile->set_consumer_name(get_consumer_key());
    sel_arena_profile->set_byte_footprint(padded_sel_arena);
}

void TaskGraphResource::_gather_managed_profiles(godot::TypedArray<ManagedBufferProfile>& r_profiles) const {
    // 1. Pack the Memory Graph profiles (and by extension, the base Graph profiles)
    MemoryGraphResource::_gather_managed_profiles(r_profiles);

    // 2. Pack the specific Task Graph profiles
    if (exec_profile.is_valid()) r_profiles.append(exec_profile);
    if (cmd_arena_profile.is_valid()) r_profiles.append(cmd_arena_profile);
    if (sel_arena_profile.is_valid()) r_profiles.append(sel_arena_profile);
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