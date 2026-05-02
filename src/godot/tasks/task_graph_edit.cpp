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
    ClassDB::bind_method(D_METHOD("_on_task_popup_select", "id"), &TaskGraphEdit::_on_task_popup_select);
}

TaskGraphEdit::TaskGraphEdit() {
}

TaskGraphEdit::~TaskGraphEdit() {
}

void TaskGraphEdit::_ready() {
    MemoryGraphEdit::_ready();

    if (PopupMenu* popup = Object::cast_to<PopupMenu>(find_child("PopupMenu", true, false))) {
        if (popup->is_connected("id_pressed", Callable(this, "_filtered_popup_select"))) {
            popup->disconnect("id_pressed", Callable(this, "_filtered_popup_select"));
        }
        popup->connect("id_pressed", Callable(this, "_on_task_popup_select"));
    }
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

TypedArray<String> TaskGraphEdit::_get_filtered_node_types(uint32_t p_filter_mask) const {
    TypedArray<String> popup_items;
    
    // We cast away const to modify our internal linear cache. 
    // This is safe here as this is strictly a UI generation pass.
    auto* mutable_this = const_cast<TaskGraphEdit*>(this);
    mutable_this->spawn_options_cache.clear();

    core::BufferLayoutType layout_requirement = static_cast<core::BufferLayoutType>(p_filter_mask);
    bool check_layout = layout_requirement != core::BufferLayoutType::NONE && layout_requirement != core::BufferLayoutType::ANY;

    auto process_matrix = [&](const Dictionary& p_matrix, TaskCategory p_category, const String& p_prefix) {
        Array keys = p_matrix.keys();
        for (int i = 0; i < keys.size(); ++i) {
            String logic_str = keys[i];
            uint32_t logic_id = static_cast<uint32_t>(logic_str.to_int());
            Dictionary logic_def = p_matrix[keys[i]];
            
            // Tier 3: ViewCapability Interrogation
            // If dragging from a specific port, validate against the O(1) permutation matrix
            if (check_layout && logic_def.has("valid_combinations")) {
                PackedInt64Array valid_hashes = logic_def["valid_combinations"];
                bool has_compatible_strategy = false;
                
                for (int h = 0; h < valid_hashes.size(); ++h) {
                    uint64_t hash = valid_hashes[h];
                    
                    // Decode the flat index. 
                    // Matrix Dimensions: O_COUNT(2) * V_COUNT(16) * S_COUNT(9) * T_COUNT(17)
                    // The Strategy dimension is heavily restricted by layout types.
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
                
                // Prune task entirely from the UI if it cannot handle the incoming topology
                if (!has_compatible_strategy) continue;
            }
            
            // Add to linear cache
            SpawnDescriptor desc;
            desc.category = p_category;
            desc.logic_id = logic_id;
            desc.logic_name = StringName(logic_str);
            mutable_this->spawn_options_cache.push_back(desc);
            
            // Add to UI
            // Example Output: "Transform: FastNoiseLite"
            popup_items.push_back(p_prefix + logic_str); 
        }
    };

    // Process all sub-registries
    process_matrix(core::NativeTaskRegistry::get_ui_transform_matrix(), CATEGORY_TRANSFORM, "Transform: ");
    process_matrix(core::NativeTaskRegistry::get_ui_metadata_matrix(), CATEGORY_METADATA, "Metadata: ");
    process_matrix(core::NativeTaskRegistry::get_ui_query_matrix(), CATEGORY_QUERY, "Query: ");

    return popup_items;
}

void TaskGraphEdit::_on_task_popup_select(int p_id) {
    if (current_blueprint.is_null()) return;
    if (p_id < 0 || p_id >= spawn_options_cache.size()) return;

    const SpawnDescriptor& desc = spawn_options_cache[p_id];
    StringName unique_name = String("TaskNode_") + String::num_int64(UtilityFunctions::randi());

    // Map Category to the specific DOD sub-type and construct the base dictionary
    Dictionary props;
    props["logic_id"] = desc.logic_id;
    props["logic_name"] = desc.logic_name;

    // Default indices to zero (User configures these via the new dynamic UI)
    props["view_id"] = 0;
    props["strategy_id"] = 0;
    props["type_id"] = 0;

    Dictionary new_node;
    new_node["name"] = unique_name;
    new_node["properties"] = props;

    // We explicitly store the type classification so GraphEdit knows which sub-node UI to instantiate
    switch (desc.category) {
        case CATEGORY_TRANSFORM: new_node["type_id"] = static_cast<uint32_t>(TaskType::TASK_NATIVE_CPU); break; 
        case CATEGORY_METADATA:  new_node["type_id"] = static_cast<uint32_t>(TaskType::TASK_NATIVE_CPU); break;
        case CATEGORY_QUERY:     
            new_node["type_id"] = static_cast<uint32_t>(TaskType::TASK_QUERY_CULLER); 
            props["op_id"] = 0; // 0 = CULL, 1 = ADD
            break;
        default: break;
    }

    // Apply scroll offset compensation
    Vector2 spawn_pos = popup_position;
    if (get_zoom() > 0.0f) {
        spawn_pos = (spawn_pos + get_scroll_offset()) / get_zoom();
    }
    new_node["position"] = spawn_pos;

    current_blueprint->action_add_node(new_node);
}

} // namespace ideam::godot_ext