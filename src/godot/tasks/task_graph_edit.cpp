#include "task_graph_edit.h"
#include "task_graph_resource.h"
#include "metadata_task_resource.h"
#include "query_task_resource.h"
#include "transform_task_resource.h"
#include "transform_task_graph_node.h"
#include "query_task_graph_node.h"
#include "metadata_task_graph_node.h"

#include "../../core/tasks/registration/ideam_task_registry.h"
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

void TaskGraphEdit::_notification(int p_what) {
    // Bubble up to MemoryGraphEdit first
    MemoryGraphEdit::_notification(p_what);

    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            _update_theme_properties();
        } break;
    }
}

void TaskGraphEdit::_update_theme_properties() {
    // 1. Core parent theme configuration (handles context_popup)
    MemoryGraphEdit::_update_theme_properties();

    // 2. Propagate styles down to any currently active submenu layers
    if (context_popup) {
        Ref<StyleBox> panel_style = get_theme_stylebox("popup_menu_panel", "PopupMenu");
        Ref<StyleBox> hover_style = get_theme_stylebox("popup_menu_hover", "PopupMenu");

        for (int i = 0; i < context_popup->get_child_count(); ++i) {
            PopupMenu* sub = Object::cast_to<PopupMenu>(context_popup->get_child(i));
            if (sub) {
                if (panel_style.is_valid()) sub->add_theme_stylebox_override("panel", panel_style);
                if (hover_style.is_valid()) sub->add_theme_stylebox_override("hover", hover_style);
            }
        }
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

TypedArray<String> TaskGraphEdit::_get_new_node_types() const {
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

    // Helper to generate, style, and bind sub-menus instantly
    auto create_submenu = [&](const String& p_name) -> PopupMenu* {
        PopupMenu* sub = memnew(PopupMenu);
        sub->set_name(p_name);
        mutable_this->context_popup->add_child(sub);
        mutable_this->context_popup->add_submenu_node_item(p_name, sub);
        
        // INSTANT THEME PROPAGATION: 
        // Ensure manual styling is set up immediately at the moment of creation
        Ref<StyleBox> panel_style = mutable_this->get_theme_stylebox("popup_menu_panel", "PopupMenu");
        Ref<StyleBox> hover_style = mutable_this->get_theme_stylebox("popup_menu_hover", "PopupMenu");
        if (panel_style.is_valid()) sub->add_theme_stylebox_override("panel", panel_style);
        if (hover_style.is_valid()) sub->add_theme_stylebox_override("hover", hover_style);

        sub->connect("id_pressed", Callable(mutable_this, "_popup_select"));
        return sub;
    };

    PopupMenu* transform_menu = create_submenu("Transform");
    PopupMenu* metadata_menu = create_submenu("Metadata");
    PopupMenu* query_menu = create_submenu("Query");
    PopupMenu* utility_menu = create_submenu("Utility");

    // Context-Aware Selection: Dynamically parse structural traits if dragging out of a port
    mutable_this->active_filter_mask = 0; // Default to zero if right-clicking generic space
    if (drag_source_port != -1 && drag_source_node != String().is_empty()) {
        Node* source_node_base = get_node_or_null(NodePath(drag_source_node));
        TaskGraphNode* source_task_node = Object::cast_to<TaskGraphNode>(source_node_base);
        
        if (source_task_node) {
            Ref<TaskResource> task_res = source_task_node->get_task_node_resource();
            if (task_res.is_valid()) {
                Ref<MemoryGrantResource> grant_res = task_res->get_memory_grant();
                if (grant_res.is_valid()) {
                    TypedArray<GrantPartResource> parts = grant_res->get_configured_parts();
                    if (drag_source_port >= 0 && drag_source_port < parts.size()) {
                        Ref<GrantPartResource> target_part = parts[drag_source_port];
                        if (target_part.is_valid()) {
                            // Extract the layout/buffer type mapping from the serialized layout intent
                            mutable_this->active_filter_mask = static_cast<uint32_t>(target_part->get_buffer_type());
                        }
                    }
                }
            }
        }
        godot::UtilityFunctions::print("Active Filter Mask set to: ", mutable_this->active_filter_mask);
    }
    core::BufferLayoutType layout_requirement = static_cast<core::BufferLayoutType>(mutable_this->active_filter_mask);
    bool check_layout = layout_requirement != core::BufferLayoutType::NONE && layout_requirement != core::BufferLayoutType::ANY;

    int current_global_id = MENU_SPAWN_NODE_START; // Start after reserved IDs for context menu options

    auto process_matrix = [&](const Dictionary& p_matrix, TaskCategory p_category, PopupMenu* p_submenu) {
        Array keys = p_matrix.keys();
        godot::UtilityFunctions::print("Category Check: ", p_category, " | Total Tasks found in Registry Matrix: ", keys.size());
        
        for (int i = 0; i < keys.size(); ++i) {
            String logic_str = keys[i];
            
            uint32_t logic_id = (p_category == CATEGORY_MANUAL) ? UINT32_MAX : static_cast<uint32_t>(logic_str.to_int());
            Dictionary logic_def = p_matrix[keys[i]];
            
            bool has_combos = logic_def.has("valid_combinations");
        
            //godot::UtilityFunctions::print(
                //" -> Task Item: ", logic_str, 
                //" | check_layout variable state: ", check_layout ? "TRUE" : "FALSE",
                //" | Has valid_combinations key: ", has_combos ? "YES" : "NO"
            //);

            if (check_layout && has_combos) {
                PackedInt64Array valid_hashes = logic_def["valid_combinations"];
                bool has_compatible_strategy = false;
                
                //godot::UtilityFunctions::print(
                    //" ====> Entering Hash Verification Loop for ", logic_str, 
                   //" with ", valid_hashes.size(), " combinations."
                //);
                
                uint32_t valid_print_count = 5;
                uint32_t current_print = 0;

                for (int h = 0; h < valid_hashes.size(); ++h) {
                    uint64_t hash = valid_hashes[h];
                    uint32_t s_index = 0;

                    if (p_category == CATEGORY_METADATA) {
                        using namespace core;
                        // Metadata Matrix uses: (View * S_COUNT * T_COUNT) + (Strategy * T_COUNT) + Type
                        // Stripping View leaves: hash % (S_COUNT * T_COUNT)
                        // Dividing by T_COUNT leaves: Strategy Index
                        uint64_t sub_stride = hash % (MetadataTaskRegistry::S_COUNT * MetadataTaskRegistry::T_COUNT);
                        s_index = static_cast<uint32_t>(sub_stride / MetadataTaskRegistry::T_COUNT);
                    } 
                    else if (p_category == CATEGORY_TRANSFORM) {
                        using namespace core;
                        // Substitute with your Transform Registry counts
                        uint64_t sub_stride = hash % (TransformTaskRegistry::S_COUNT * TransformTaskRegistry::T_COUNT);
                        s_index = static_cast<uint32_t>(sub_stride / TransformTaskRegistry::T_COUNT);
                    } 
                    else if (p_category == CATEGORY_QUERY) {
                        using namespace core;
                        // Layout: (Op * V_COUNT * S_COUNT * T_COUNT) + (View * S_COUNT * T_COUNT) + (Strategy * T_COUNT) + Type
                        // 1. Strip Op out by modulo-ing by the combined size of the 3 lower dimensions
                        uint64_t op_sub_stride = QueryTaskRegistry::V_COUNT * QueryTaskRegistry::S_COUNT * QueryTaskRegistry::T_COUNT;
                        uint64_t hash_without_op = hash % op_sub_stride;
                        
                        // 2. Strip View out by modulo-ing by the combined size of the 2 lower dimensions
                        uint64_t view_sub_stride = QueryTaskRegistry::S_COUNT * QueryTaskRegistry::T_COUNT;
                        uint64_t hash_without_view = hash_without_op % view_sub_stride;
                        
                        // 3. Isolate Strategy index by dividing by Type count
                        s_index = static_cast<uint32_t>(hash_without_view / QueryTaskRegistry::T_COUNT);
                    }

                    bool match = _strategy_supports_layout(static_cast<core::MemoryStrategy>(s_index), layout_requirement);
                    
                    if (current_print < valid_print_count) {
                        // --- THE SANITY PRINT PIPELINE ---
                        //godot::UtilityFunctions::print(
                            //"Category: ", p_category, 
                            //" | Hash: ", hash, 
                            //" | Unpacked S_Index: ", s_index, 
                            //" | Requirement Mask: ", static_cast<int>(layout_requirement),
                            //" | Pass Validation: ", match ? "YES" : "NO"
                        //);
                        current_print++;
                    }

                    if (match) {
                        has_compatible_strategy = true;
                        break;
                    }
                }
                
                if (!has_compatible_strategy) continue;
            }
            
            SpawnDescriptor desc;
            desc.category = p_category;
            desc.logic_id = logic_id;
            desc.task_name = StringName(logic_str);
            mutable_this->spawn_options_cache.push_back(desc);
            
            String display_name = (p_category == CATEGORY_MANUAL) ? logic_str : String(logic_def.get("name", logic_str));
            p_submenu->add_item(display_name, current_global_id);
            current_global_id++;
        }
    };

    process_matrix(core::IdeamTaskRegistry::get_ui_transform_matrix(), CATEGORY_TRANSFORM, transform_menu);
    process_matrix(core::IdeamTaskRegistry::get_ui_metadata_matrix(), CATEGORY_METADATA, metadata_menu);
    process_matrix(core::IdeamTaskRegistry::get_ui_query_matrix(), CATEGORY_QUERY, query_menu);
    process_matrix(core::IdeamTaskRegistry::get_ui_utility_matrix(), CATEGORY_MANUAL, utility_menu);

    return TypedArray<String>();
}

void TaskGraphEdit::_spawn_node_by_type(int p_type_id) {
    if (current_blueprint.is_null()) return;
    if (p_type_id < 0 || p_type_id >= spawn_options_cache.size()) return;

    const SpawnDescriptor& desc = spawn_options_cache[p_type_id];
    StringName unique_name = String("TaskNode_") + String::num_int64(UtilityFunctions::randi());

    // Declare the base reference, but DO NOT instantiate it yet.
    Ref<TaskResource> new_res;

    // Branch to instantiate the highly specialized, memory-packed resource.
    switch (desc.category) {
        case CATEGORY_TRANSFORM: {
            Ref<TransformTaskResource> res;
            res.instantiate();
            res->set_task_type(TASK_NATIVE_CPU);
            res->set_logic_id(desc.logic_id); 
            new_res = res;
            break;
        }
        case CATEGORY_METADATA: {
            Ref<MetadataTaskResource> res;
            res.instantiate();
            res->set_task_type(TASK_NATIVE_CPU);
            res->set_logic_id(desc.logic_id);
            new_res = res;
            break;
        }
        case CATEGORY_QUERY: {
            Ref<QueryTaskResource> res;
            res.instantiate();
            res->set_task_type(TASK_QUERY_CULLER);
            res->set_logic_id(desc.logic_id);
            new_res = res;
            break;
        }
        case CATEGORY_MANUAL: {
            core::IdeamTaskRegistry* registry = core::IdeamTaskRegistry::get_singleton();
            auto* factories = registry->get_ui_factories();
        
            if (factories && factories->has(desc.task_name)) {
                // Deterministic, O(1) instantiation. No casting required.
                new_res = (*factories)[desc.task_name].resource_factory();
                new_res->set_task_type(TASK_NATIVE_CPU);
            } else {
                godot::UtilityFunctions::printerr("TaskGraphEdit: Manual task factory not found for ", desc.task_name);
                return;
            }
            break;
        }
        default:
            return; // Safety bailout
    }

    // Apply the configured properties to our safely typed wrapper
    new_res->set_node_name(unique_name);
    new_res->set_task_name(desc.task_name);

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

        godot::StringName task_name = task_res->get_task_name();
        core::IdeamTaskRegistry* registry = core::IdeamTaskRegistry::get_singleton();
        auto* factories = registry->get_ui_factories();
        if (factories && factories->has(task_name)) {
            new_node = (*factories)[task_name].node_factory();
        }
    }

    if (new_node) {
        // Wire up the dynamic UI population for DOD memory buffers.
        // Because TaskGraphEdit inherits from MemoryGraphEdit, the Callable will
        // natively resolve to the base class's bound method.
        
        new_node->connect("memory_grant_requested", godot::Callable(this, "_on_node_memory_grant_requested"));
        new_node->connect("buffer_names_requested", godot::Callable(this, "_on_buffer_names_requested"));
        new_node->connect("connections_requested", godot::Callable(this, "_on_node_connections_requested"));
    } else {
        godot::UtilityFunctions::printerr("TaskGraphEdit: Failed to resolve UI GraphNode class for data payload: ", res_class);
    }

    // Return the newly instantiated node so the IdeamGraphEdit sync loop can position/add it to the tree
    return new_node;
}

} // namespace ideam::godot_ext