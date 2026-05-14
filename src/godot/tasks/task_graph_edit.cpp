#include "task_graph_edit.h"
#include "task_graph_resource.h"
#include "metadata_task_resource.h"
#include "query_task_resource.h"
#include "transform_task_resource.h"
#include "transform_task_graph_node.h"
#include "query_task_graph_node.h"
#include "metadata_task_graph_node.h"

#include "../../core/tasks/registration/transform_task_registry.h"
#include "../../core/tasks/registration/metadata_task_registry.h"
#include "../../core/tasks/registration/query_task_registry.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

using namespace godot;

namespace ideam::godot_ext {

void TaskGraphEdit::_bind_methods() {
    // Relying on the base class's _popup_select routing instead of a custom intercept
}

TaskGraphEdit::TaskGraphEdit() {
}

TaskGraphEdit::~TaskGraphEdit() {
}

void TaskGraphEdit::_ready() {
    MemoryGraphEdit::_ready();
    // Removed the manual popup disconnect/reconnect logic since we now hook the sub-menus
    // directly into the parent's native _popup_select.
}

bool TaskGraphEdit::_strategy_supports_layout(core::MemoryStrategy p_strategy, core::BufferLayoutType p_layout) const {
    // Exact mapping between structural memory layouts and safe execution strategies.
    // Prevents cache-thrashing tasks from attempting to process fragmented layouts.
    using namespace core;
    BufferLayoutType strategy_layout_mask = BufferLayoutType::NONE;
    
    switch (p_strategy) {
        case MemoryStrategy::FlatStrategy:      strategy_layout_mask = BufferLayoutType::FLAT; break;
        case MemoryStrategy::SoAStrategy:       strategy_layout_mask = BufferLayoutType::SOA; break;
        case MemoryStrategy::AoSStrategy:       strategy_layout_mask = BufferLayoutType::AOS; break;
        case MemoryStrategy::Spatial2DStrategy: 
        case MemoryStrategy::Spatial3DStrategy: 
        case MemoryStrategy::Spatial4DStrategy: strategy_layout_mask = BufferLayoutType::ANY_SPATIAL; break;
        case MemoryStrategy::TiledSoAStrategy:  strategy_layout_mask = BufferLayoutType::TILED_SOA; break;
        case MemoryStrategy::RingStrategy:      strategy_layout_mask = BufferLayoutType::RING; break;
        case MemoryStrategy::PagedStrategy:     strategy_layout_mask = BufferLayoutType::PAGED; break;
        default: break;
    }

    return core::has_layout(p_layout, strategy_layout_mask);
}

TypedArray<String> TaskGraphEdit::_get_new_node_types() const {
    // We cast away const to modify our internal linear cache and inject sub-menus into the parent's popup.
    auto* mutable_this = const_cast<TaskGraphEdit*>(this);
    mutable_this->spawn_options_cache.clear();

    if (!mutable_this->context_popup) return TypedArray<String>();

    // Clean up any stale sub-menus from the previous popup invocation
    for (int i = mutable_this->context_popup->get_child_count() - 1; i >= 0; --i) {
        Node* child = mutable_this->context_popup->get_child(i);
        if (Object::cast_to<PopupMenu>(child)) {
            child->queue_free();
        }
    }

    // Helper to generate and bind sub-menus
    auto create_submenu = [&](const String& p_name) -> PopupMenu* {
        PopupMenu* sub = memnew(PopupMenu);
        sub->set_name(p_name);
        mutable_this->context_popup->add_child(sub);
        mutable_this->context_popup->add_submenu_node_item(p_name, sub);
        
        // Route selections dynamically back up to _spawn_node_by_type via the parent
        sub->connect("id_pressed", Callable(mutable_this, "_popup_select"));
        return sub;
    };

    PopupMenu* transform_menu = create_submenu("Transform");
    PopupMenu* metadata_menu = create_submenu("Metadata");
    PopupMenu* query_menu = create_submenu("Query");
    PopupMenu* utility_menu = create_submenu("Utility");

    core::BufferLayoutType layout_requirement = static_cast<core::BufferLayoutType>(mutable_this->active_filter_mask);
    bool check_layout = layout_requirement != core::BufferLayoutType::NONE && layout_requirement != core::BufferLayoutType::ANY;

    int current_global_id = 0;

    auto process_matrix = [&](const Dictionary& p_matrix, TaskCategory p_category, PopupMenu* p_submenu) {
        Array keys = p_matrix.keys();
        for (int i = 0; i < keys.size(); ++i) {
            String logic_str = keys[i];
            
            // UINT32_MAX avoids parsing collisions for utility tasks.
            uint32_t logic_id = (p_category == CATEGORY_MANUAL) ? UINT32_MAX : static_cast<uint32_t>(logic_str.to_int());
            Dictionary logic_def = p_matrix[keys[i]];
            
            if (check_layout && logic_def.has("valid_combinations")) {
                PackedInt64Array valid_hashes = logic_def["valid_combinations"];
                bool has_compatible_strategy = false;
                
                for (int h = 0; h < valid_hashes.size(); ++h) {
                    uint64_t hash = valid_hashes[h];
                    uint32_t s_index = 0;
                    if (p_category == CATEGORY_QUERY) {
                        s_index = (hash % 2448 % 153) / 17;
                    } else {
                        s_index = (hash % 153) / 17;
                    }

                    if (_strategy_supports_layout(static_cast<core::MemoryStrategy>(s_index), layout_requirement)) {
                        has_compatible_strategy = true;
                        break;
                    }
                }
                
                if (!has_compatible_strategy) continue;
            }
            
            SpawnDescriptor desc;
            desc.category = p_category;
            desc.logic_id = logic_id;
            desc.logic_name = StringName(logic_str);
            mutable_this->spawn_options_cache.push_back(desc);
            
            // Assuming your DOD dictionaries hold a "name" property for standard matrix tasks.
            // If they don't, this safely falls back to printing the logic_str.
            String display_name = (p_category == CATEGORY_MANUAL) ? logic_str : String(logic_def.get("name", logic_str));
            p_submenu->add_item(display_name, current_global_id);
            current_global_id++;
        }
    };

    process_matrix(core::NativeTaskRegistry::get_ui_transform_matrix(), CATEGORY_TRANSFORM, transform_menu);
    process_matrix(core::NativeTaskRegistry::get_ui_metadata_matrix(), CATEGORY_METADATA, metadata_menu);
    process_matrix(core::NativeTaskRegistry::get_ui_query_matrix(), CATEGORY_QUERY, query_menu);
    process_matrix(core::NativeTaskRegistry::get_ui_utility_matrix(), CATEGORY_MANUAL, utility_menu);

    // Return empty array. We manually constructed the hierarchy, so we don't want the 
    // base class to append any generic flat strings to the root popup menu.
    return TypedArray<String>();
}

void TaskGraphEdit::_spawn_node_by_type(int p_type_id) {
    if (current_blueprint.is_null()) return;
    if (p_type_id < 0 || p_type_id >= spawn_options_cache.size()) return;

    const SpawnDescriptor& desc = spawn_options_cache[p_type_id];
    StringName unique_name = String("TaskNode_") + String::num_int64(UtilityFunctions::randi());

    Dictionary props;
    props["logic_id"] = desc.logic_id;
    props["logic_name"] = desc.logic_name;
    props["view_id"] = 0;
    props["strategy_id"] = 0;
    props["type_id"] = 0;

    // Declare the base reference, but DO NOT instantiate it yet.
    Ref<TaskResource> new_res;

    // Branch to instantiate the highly specialized, memory-packed resource.
    switch (desc.category) {
        case CATEGORY_TRANSFORM: {
            Ref<TransformTaskResource> res;
            res.instantiate();
            res->set_task_type(TASK_NATIVE_CPU);
            new_res = res;
            break;
        }
        case CATEGORY_METADATA: {
            Ref<MetadataTaskResource> res;
            res.instantiate();
            res->set_task_type(TASK_NATIVE_CPU);
            new_res = res;
            break;
        }
        case CATEGORY_QUERY: {
            Ref<QueryTaskResource> res;
            res.instantiate();
            res->set_task_type(TASK_QUERY_CULLER); 
            props["op_id"] = 0; // 0 = CULL, 1 = ADD
            new_res = res;
            break;
        }
        case CATEGORY_MANUAL: {
            // Retrieve the UI definitions dictionary from our native registry
            godot::Dictionary utility_matrix = core::NativeTaskRegistry::get_ui_utility_matrix();
            
            if (utility_matrix.has(desc.logic_name)) {
                godot::Dictionary task_def = utility_matrix[desc.logic_name];
                godot::StringName resource_class = task_def["resource_class"];
                
                // Dynamically allocate exactly what is needed using Godot's ClassDB
                godot::Object* obj = godot::ClassDB::instantiate(resource_class);
                TaskResource* tr = godot::Object::cast_to<TaskResource>(obj);
                
                if (tr) {
                    new_res = godot::Ref<TaskResource>(tr);
                    new_res->set_task_type(TASK_NATIVE_CPU); 
                } else {
                    godot::UtilityFunctions::printerr("TaskGraphEdit: Failed to cast instantiated object to TaskResource for ", desc.logic_name);
                    return;
                }
            } else {
                godot::UtilityFunctions::printerr("TaskGraphEdit: Manual task not found in utility matrix: ", desc.logic_name);
                return;
            }
            break;
        }
        default:
            return; // Safety bailout
    }

    // Apply the configured properties to our safely typed wrapper
    new_res->set_node_name(unique_name);
    new_res->set_task_properties(props);

    // Apply scroll offset compensation
    Vector2 spawn_pos = popup_position;
    if (get_zoom() > 0.0f) {
        spawn_pos = (spawn_pos + get_scroll_offset()) / get_zoom();
    }
    new_res->set_position_offset(spawn_pos);

    // Push to the data blueprint (which should trigger a UI graph sync downstream)
    current_blueprint->action_add_node(new_res);
}

IdeamGraphNode* TaskGraphEdit::_create_graph_node(const godot::Ref<IdeamGraphNodeResource>& p_node_res) {
    Ref<TaskResource> task_res = p_node_res;
    if (task_res.is_null()) {
        return nullptr; // Hard bailout: Keeps type constraints rigid
    }

    IdeamGraphNode* new_node = nullptr;
    StringName res_class = task_res->get_class();

    // 1. Direct Archetype Matching (Hot Path for primary nodes)
    // Using Object::cast_to avoids string hashing overhead where possible.
    if (Object::cast_to<TransformTaskResource>(task_res.ptr())) {
        new_node = memnew(TransformTaskGraphNode);
    } 
    else if (Object::cast_to<QueryTaskResource>(task_res.ptr())) {
        new_node = memnew(QueryTaskGraphNode);
    } 
    else if (Object::cast_to<MetadataTaskResource>(task_res.ptr())) {
        new_node = memnew(MetadataTaskGraphNode);
    } 
    // 2. Dynamic Fallback for Utility / Manual Tasks
    // If it's not a core struct, we query the utility matrix to find its bound UI class.
    else {
        godot::Dictionary utility_matrix = core::NativeTaskRegistry::get_ui_utility_matrix();
        godot::Array keys = utility_matrix.keys();
        
        // Scan the matrix to find the matching resource footprint
        for (int i = 0; i < keys.size(); ++i) {
            godot::Dictionary task_def = utility_matrix[keys[i]];
            if (task_def["resource_class"] == res_class) {
                godot::StringName node_class = task_def["node_class"];
                
                // Allocate the paired UI node dynamically
                godot::Object* obj = godot::ClassDB::instantiate(node_class);
                new_node = godot::Object::cast_to<IdeamGraphNode>(obj);
                break;
            }
        }
    }

    if (new_node) {
        // Wire up the dynamic UI population for DOD memory buffers.
        // Because TaskGraphEdit inherits from MemoryGraphEdit, the Callable will
        // natively resolve to the base class's bound method.
        new_node->connect("buffer_names_requested", 
            callable_mp(static_cast<MemoryGraphEdit*>(this), &MemoryGraphEdit::_on_buffer_names_requested));
    } else {
        godot::UtilityFunctions::printerr("TaskGraphEdit: Failed to resolve UI GraphNode class for data payload: ", res_class);
    }

    // Return the newly instantiated node so the IdeamGraphEdit sync loop can position/add it to the tree
    return new_node;
}

} // namespace ideam::godot_ext