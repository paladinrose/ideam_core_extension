#include "task_graph_edit.h"
#include "task_graph_resource.h"
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

    // Direct instantiation of the tightly packed C++ DOD Resource instead of generic maps
    Ref<TaskResource> new_res;
    new_res.instantiate();
    new_res->set_node_name(unique_name);

    switch (desc.category) {
        case CATEGORY_TRANSFORM: new_res->set_task_type(TASK_NATIVE_CPU); break; 
        case CATEGORY_METADATA:  new_res->set_task_type(TASK_NATIVE_CPU); break;
        case CATEGORY_QUERY:     
            new_res->set_task_type(TASK_QUERY_CULLER); 
            props["op_id"] = 0; // 0 = CULL, 1 = ADD
            break;
        case CATEGORY_MANUAL:
            // STUB: You'll want to map this to whatever your manual task GUI node type is mapped to.
            new_res->set_task_type(TASK_NATIVE_CPU); 
            break;
        default: break;
    }

    new_res->set_task_properties(props);

    // Apply scroll offset compensation
    Vector2 spawn_pos = popup_position;
    if (get_zoom() > 0.0f) {
        spawn_pos = (spawn_pos + get_scroll_offset()) / get_zoom();
    }
    new_res->set_position_offset(spawn_pos);

    current_blueprint->action_add_node(new_res);
}

} // namespace ideam::godot_ext