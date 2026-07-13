#include "memory_graph_node.h"
#include "memory_graph_node_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/popup_menu.hpp>

#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/style_box_texture.hpp>
#include <godot_cpp/classes/style_box_line.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryGraphNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_memory_node_resource"), &MemoryGraphNode::get_memory_node_resource);
    ClassDB::bind_method(D_METHOD("update_telemetry", "inspector"), &MemoryGraphNode::update_telemetry);
    ClassDB::bind_method(D_METHOD("_on_inspect_memory_pressed"), &MemoryGraphNode::_on_inspect_memory_pressed);
    ClassDB::bind_method(D_METHOD("get_port_signature", "port_idx", "is_output"), &MemoryGraphNode::get_port_signature);
    ClassDB::bind_method(D_METHOD("receive_buffer_names_list", "names"), &MemoryGraphNode::receive_buffer_names_list);
    ClassDB::bind_method(D_METHOD("update_memory_port", "slot_index", "is_left", "layout"), &MemoryGraphNode::update_memory_port);
    ClassDB::bind_method(D_METHOD("set_header_state", "state"), &MemoryGraphNode::set_header_state);
    
    ClassDB::bind_method(D_METHOD("receive_memory_grant", "grant"), &MemoryGraphNode::receive_memory_grant);
    ClassDB::bind_method(D_METHOD("_on_request_grant_pressed"), &MemoryGraphNode::_on_request_grant_pressed);

    ADD_SIGNAL(MethodInfo("inspect_memory_requested", PropertyInfo(Variant::OBJECT, "inspector", PROPERTY_HINT_RESOURCE_TYPE, "MemoryGrantInspector")));
    ADD_SIGNAL(MethodInfo("buffer_names_requested", 
        PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "MemoryGraphNode"),
        PropertyInfo(Variant::ARRAY, "buffer_ids", PROPERTY_HINT_ARRAY_TYPE, "int")));

    // New Signal definition
    ADD_SIGNAL(MethodInfo("memory_grant_requested", 
        PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "MemoryGraphNode")));
    
    ADD_SIGNAL(MethodInfo("active_grants_requested", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "MemoryGraphNode")));

    // [Memz] Manually bind external DOD bitflags to int64_t to safely cross the engine boundary.
    // This keeps src/core/ entirely agnostic of Godot's reflection macros.
    ClassDB::bind_integer_constant(get_class_static(), StringName(), StringName("LAYOUT_NONE"), static_cast<int64_t>(core::BufferLayoutType::NONE));
    ClassDB::bind_integer_constant(get_class_static(), StringName(), StringName("LAYOUT_FLAT"), static_cast<int64_t>(core::BufferLayoutType::FLAT));
    ClassDB::bind_integer_constant(get_class_static(), StringName(), StringName("LAYOUT_AOS"), static_cast<int64_t>(core::BufferLayoutType::AOS));
    ClassDB::bind_integer_constant(get_class_static(), StringName(), StringName("LAYOUT_SOA"), static_cast<int64_t>(core::BufferLayoutType::SOA));
    ClassDB::bind_integer_constant(get_class_static(), StringName(), StringName("LAYOUT_SPARSE_SET"), static_cast<int64_t>(core::BufferLayoutType::SPARSE_SET));
    ClassDB::bind_integer_constant(get_class_static(), StringName(), StringName("LAYOUT_TILED_SOA"), static_cast<int64_t>(core::BufferLayoutType::TILED_SOA));
    ClassDB::bind_integer_constant(get_class_static(), StringName(), StringName("LAYOUT_RING"), static_cast<int64_t>(core::BufferLayoutType::RING));
    ClassDB::bind_integer_constant(get_class_static(), StringName(), StringName("LAYOUT_PAGED"), static_cast<int64_t>(core::BufferLayoutType::PAGED));
    
    // Expose LayoutHeaderState to GDScript
    BIND_ENUM_CONSTANT(HEADER_VALID);
    BIND_ENUM_CONSTANT(HEADER_ERROR);

    // Expose TelemetryBadgeState to GDScript
    BIND_ENUM_CONSTANT(TELEMETRY_INACTIVE);
    BIND_ENUM_CONSTANT(TELEMETRY_ACTIVE);
    BIND_ENUM_CONSTANT(TELEMETRY_DIRTY);
    BIND_ENUM_CONSTANT(TELEMETRY_ERROR);
}

MemoryGraphNode::MemoryGraphNode() {
}

Ref<MemoryGraphNodeResource> MemoryGraphNode::get_memory_node_resource() const {
    return Object::cast_to<MemoryGraphNodeResource>(get_node_resource().ptr());
}

void MemoryGraphNode::receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names) {
    
}

void MemoryGraphNode::_notification(int p_what) {
    IdeamGraphNode::_notification(p_what); // Ensure base notifications are processed

    if (p_what == NOTIFICATION_DRAW) {
        
        // 3. Draw Telemetry Text directly onto the node canvas if Active/Dirty
        if (latest_grant_snapshot.is_valid() && (telemetry_state == TELEMETRY_ACTIVE || telemetry_state == TELEMETRY_DIRTY)) {
            Ref<Font> font = get_theme_font("title_font", "GraphNode");
            int font_size = get_theme_font_size("title_font_size", "GraphNode") - 2; // Slightly smaller than title
            
            if (font.is_valid() && latest_grant_snapshot->get_part_count() > 0) {
                // For simplicity on the canvas, we visualize the telemetry of the primary/first part.
                // Complete details are handled by the inspector side-panel via the button.
                Dictionary primary_part = latest_grant_snapshot->get_part_snapshot(0);
                
                String telemetry_str = String("Cap: ") + String::num_int64(primary_part.get("capacity", 0)) +
                                       String(" | Elem: ") + String::num_int64(primary_part.get("element_count", 0));
                
                // Draw text near the bottom of the node
                Vector2 text_pos = Vector2(10, get_size().height - 15);
                draw_string(font, text_pos, telemetry_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, get_theme_color("title_font_color", "GraphNode"));
            }
        }
    }
}

void MemoryGraphNode::_build_ui() {
    IdeamGraphNode::_build_ui(); // Call base setup

    telemetry_badge = memnew(godot::TextureRect);
    telemetry_badge->set_name("TelemetryBadge");
    add_badge(telemetry_badge);
    
    // Establish the container horizontal layout
    memory_controls_hb = memnew(HBoxContainer);
    add_child(memory_controls_hb);

    // Pre-create the telemetry button (Refactored to live inside our HBox)
    inspect_memory_btn = memnew(Button);
    inspect_memory_btn->set_text("No Memory Grant");
    inspect_memory_btn->set_disabled(true);
    inspect_memory_btn->set_h_size_flags(SIZE_EXPAND_FILL); // Fill out space gracefully
    inspect_memory_btn->connect("pressed", Callable(this, "_on_inspect_memory_pressed"));
    memory_controls_hb->add_child(inspect_memory_btn);

    // New Grant Requestor Button
    request_grant_btn = memnew(Button);
    request_grant_btn->set_text("+"); // Keep it compact for the node row layout
    request_grant_btn->set_tooltip_text("Request New Memory Grant from Manager");
    request_grant_btn->connect("pressed", Callable(this, "_on_request_grant_pressed"));
    memory_controls_hb->add_child(request_grant_btn);

    grant_selector_btn = memnew(OptionButton);
    grant_selector_btn->set_text("Assign Grant...");
    grant_selector_btn->set_h_size_flags(SIZE_EXPAND_FILL);
    grant_selector_btn->get_popup()->connect("about_to_popup", Callable(this, "_on_grant_dropdown_about_to_popup"));
    grant_selector_btn->connect("item_selected", Callable(this, "_on_grant_selected"));
    memory_controls_hb->add_child(grant_selector_btn);
}

void MemoryGraphNode::_on_request_grant_pressed() {
    emit_signal("memory_grant_requested", this);
}

void MemoryGraphNode::_on_grant_dropdown_about_to_popup() {
    emit_signal("active_grants_requested", this);
}

void MemoryGraphNode::populate_grant_dropdown(const godot::TypedArray<MemoryGrantResource>& p_grants) {
    grant_selector_btn->clear();
    cached_dropdown_grants = p_grants;
    
    if (p_grants.is_empty()) {
        grant_selector_btn->add_item("No Grants Available");
        grant_selector_btn->set_item_disabled(0, true);
        return;
    }
    
    for (int i = 0; i < p_grants.size(); ++i) {
        godot::Ref<MemoryGrantResource> grant = p_grants[i];
        if (grant.is_valid()) {
            godot::String name = grant->get_grant_name().is_empty() ? godot::String("Unnamed Grant") : godot::String(grant->get_grant_name());
            grant_selector_btn->add_item(name, i);
        }
    }
}

void MemoryGraphNode::_on_grant_selected(int p_index) {
    if (p_index >= 0 && p_index < cached_dropdown_grants.size()) {
        godot::Ref<MemoryGrantResource> selected_grant = cached_dropdown_grants[p_index];
        receive_memory_grant(selected_grant);
    }
}

void MemoryGraphNode::receive_connection_info(const godot::Dictionary& p_info) {
    IdeamGraphNode::receive_connection_info(p_info);
    
    bool has_connections = false;
    
    if (p_info.has("inputs")) {
        godot::TypedArray<godot::Dictionary> inputs = p_info["inputs"];
        if (inputs.size() > 0) has_connections = true;
    }
    
    if (p_info.has("outputs")) {
        godot::TypedArray<godot::Dictionary> outputs = p_info["outputs"];
        if (outputs.size() > 0) has_connections = true;
    }
    
    if (request_grant_btn) {
        request_grant_btn->set_disabled(has_connections);
    }
}

// Update receive_memory_grant:
void MemoryGraphNode::receive_memory_grant(const godot::Ref<MemoryGrantResource>& p_grant) {
    godot::UtilityFunctions::print("MemoryGraphNode received memory grant: ", p_grant.is_valid() ? p_grant->get_grant_name() : "null");
    
    Ref<MemoryGraphNodeResource> res = get_memory_node_resource();
    if (res.is_valid()) {
        res->set_memory_grant(p_grant);
        
        if (p_grant.is_valid()) {
            inspect_memory_btn->set_disabled(false);

            godot::String grant_name = p_grant->get_grant_name().is_empty() ? "Unnamed Grant" : p_grant->get_grant_name();
            inspect_memory_btn->set_text(godot::String("Grant: ") + grant_name);

            godot::TypedArray<GrantPartResource> parts = p_grant->get_configured_parts();
            for (int i = 0; i < parts.size(); ++i) {
                godot::Ref<GrantPartResource> part = parts[i];
                if (part.is_valid()) {
                    // Extract core enum from the newly saved int mapping
                    auto layout_val = static_cast<core::BufferLayoutType>(part->get_buffer_type());
                    godot::BitField<core::BufferLayoutType> bf_layout(static_cast<int64_t>(layout_val));
                    
                    // Enable both left and right as matched pairs, assigning the specific shape icon
                    set_slot_enabled_left(i, true);
                    set_slot_enabled_right(i, true);
                    update_memory_port(i, true, bf_layout);
                    update_memory_port(i, false, bf_layout);
                }
            }
        } else {
            inspect_memory_btn->set_disabled(true);
        }
    }
}

void MemoryGraphNode::_update_theme_properties() {
    IdeamGraphNode::_update_theme_properties();

    godot::StringName type_context = "GraphNode";

    // 1. Direct Titlebar Overrides
    godot::StringName title_style_name = (header_state == HEADER_ERROR) ? "layout_header_error" : "layout_header_valid";
    godot::Ref<godot::StyleBox> tb_style = get_theme_stylebox(title_style_name, type_context);
    if (tb_style.is_valid()) {
        add_theme_stylebox_override("titlebar", tb_style);
        add_theme_stylebox_override("titlebar_selected", tb_style);
    }

    // 2. Modulate Color via Stylebox (Replacing set_self_modulate)
    godot::Color modulation_tint = godot::Color(1, 1, 1, 1);
    switch (telemetry_state) {
        case TELEMETRY_ERROR:   modulation_tint = get_theme_color("telemetry_error_color", type_context); break;
        case TELEMETRY_DIRTY:   modulation_tint = get_theme_color("telemetry_dirty_color", type_context); break;
        case TELEMETRY_ACTIVE:  modulation_tint = get_theme_color("telemetry_active_color", type_context); break;
        case TELEMETRY_INACTIVE:
        default:                modulation_tint = get_theme_color("telemetry_inactive_color", type_context); break;
    }

    // 3. Safely duplicate the panel so we don't mutate the global theme, then apply color
    if (!get_locked()) {
        godot::Color modulation_tint = godot::Color(1, 1, 1, 1);
        switch (telemetry_state) {
            case TELEMETRY_ERROR:   modulation_tint = get_theme_color("telemetry_error_color", type_context); break;
            case TELEMETRY_DIRTY:   modulation_tint = get_theme_color("telemetry_dirty_color", type_context); break;
            case TELEMETRY_ACTIVE:  modulation_tint = get_theme_color("telemetry_active_color", type_context); break;
            case TELEMETRY_INACTIVE:
            default:                modulation_tint = get_theme_color("telemetry_inactive_color", type_context); break;
        }

        godot::Ref<godot::StyleBox> panel_sb = get_theme_stylebox("panel", type_context);
        if (panel_sb.is_valid()) {
            godot::Ref<godot::StyleBox> tinted_panel = panel_sb->duplicate();
            
            if (godot::StyleBoxFlat* flat = godot::Object::cast_to<godot::StyleBoxFlat>(tinted_panel.ptr())) {
                flat->set_bg_color(modulation_tint);
            } 
            else if (godot::StyleBoxTexture* tex = godot::Object::cast_to<godot::StyleBoxTexture>(tinted_panel.ptr())) {
                tex->set_modulate(modulation_tint);
            } 
            else if (godot::StyleBoxLine* line = godot::Object::cast_to<godot::StyleBoxLine>(tinted_panel.ptr())) {
                line->set_color(modulation_tint);
            }

            add_theme_stylebox_override("panel", tinted_panel);
        }
    }

    // 4. Update Badge
    if (telemetry_badge) {
        telemetry_badge->set_texture(_get_telemetry_badge_icon(telemetry_state));
    }

}

// --- Theme Mapping Helpers ---

Ref<Texture2D> MemoryGraphNode::_get_icon_for_layout(core::BufferLayoutType p_layout) const {
    // Map structural memory layout directly to port shapes
    if (static_cast<uint16_t>(p_layout) & static_cast<uint16_t>(core::BufferLayoutType::FLAT | core::BufferLayoutType::AOS | core::BufferLayoutType::SOA)) {
        return get_theme_icon("port_shape_contiguous", "GraphNode");
    } 
    else if (static_cast<uint16_t>(p_layout) & static_cast<uint16_t>(core::BufferLayoutType::SPARSE_SET | core::BufferLayoutType::TILED_SOA)) {
        return get_theme_icon("port_shape_fragmented", "GraphNode");
    }
    else if (static_cast<uint16_t>(p_layout) & static_cast<uint16_t>(core::BufferLayoutType::RING | core::BufferLayoutType::PAGED)) {
        return get_theme_icon("port_shape_virtual", "GraphNode");
    }
    
    return get_theme_icon("port_shape_error", "GraphNode");
}

Ref<Texture2D> MemoryGraphNode::_get_telemetry_badge_icon(TelemetryBadgeState p_state) const {
    switch (p_state) {
        case TELEMETRY_ACTIVE:   return get_theme_icon("telemetry_badge_active", "GraphNode");
        case TELEMETRY_DIRTY:    return get_theme_icon("telemetry_badge_dirty", "GraphNode");
        case TELEMETRY_ERROR:    return get_theme_icon("telemetry_badge_error", "GraphNode");
        case TELEMETRY_INACTIVE: 
        default:                 return get_theme_icon("telemetry_badge_inactive", "GraphNode");
    }
}

// --- Public Mutations ---

void MemoryGraphNode::update_memory_port(int p_slot_index, bool p_is_left, godot::BitField<core::BufferLayoutType> p_layout) {
    // Extract the underlying enum class value from the BitField wrapper
    core::BufferLayoutType layout_val = static_cast<core::BufferLayoutType>(static_cast<int64_t>(p_layout));
    Ref<Texture2D> shape_icon = _get_icon_for_layout(layout_val);

    // Persist existing settings, only overwrite the icon
    bool enable_left  = is_slot_enabled_left(p_slot_index);
    int  type_left    = get_slot_type_left(p_slot_index);
    Color color_left  = get_slot_color_left(p_slot_index);
    Ref<Texture2D> icon_left = p_is_left ? shape_icon : get_slot_custom_icon_left(p_slot_index);

    bool enable_right = is_slot_enabled_right(p_slot_index);
    int  type_right   = get_slot_type_right(p_slot_index);
    Color color_right = get_slot_color_right(p_slot_index);
    Ref<Texture2D> icon_right = !p_is_left ? shape_icon : get_slot_custom_icon_right(p_slot_index);

    set_slot(p_slot_index, 
             enable_left, type_left, color_left, 
             enable_right, type_right, color_right, 
             icon_left, icon_right);
}

void MemoryGraphNode::set_header_state(LayoutHeaderState p_state) {
    if (header_state == p_state) return;
    header_state = p_state;
    notification(NOTIFICATION_THEME_CHANGED); 
}

void MemoryGraphNode::update_telemetry(const Ref<MemoryGrantInspector>& p_inspector) {
    latest_grant_snapshot = p_inspector;
    
    if (latest_grant_snapshot.is_valid()) {
        if (latest_grant_snapshot->has_error()) {
            telemetry_state = TELEMETRY_ERROR;
            inspect_memory_btn->set_disabled(true);
            inspect_memory_btn->set_text("Grant Error (Dangling Pointer)");
        } 
        else if (latest_grant_snapshot->is_dirty()) {
            telemetry_state = TELEMETRY_DIRTY;
            inspect_memory_btn->set_disabled(false);
            inspect_memory_btn->set_text("Inspect Grant (Stale Data)");
        }
        else if (latest_grant_snapshot->is_active()) {
            telemetry_state = TELEMETRY_ACTIVE;
            inspect_memory_btn->set_disabled(false);
            inspect_memory_btn->set_text(String("Inspect Grant (") + String::num_int64(latest_grant_snapshot->get_part_count()) + " parts)");
        } 
        else {
            telemetry_state = TELEMETRY_INACTIVE;
            inspect_memory_btn->set_disabled(true);
            inspect_memory_btn->set_text("Grant Inactive");
        }
    } else {
        telemetry_state = TELEMETRY_INACTIVE;
        inspect_memory_btn->set_disabled(true);
        inspect_memory_btn->set_text("No Memory Grant");
    }

    // Rely on centralized update properties to shift node tracking colors smoothly
    notification(NOTIFICATION_THEME_CHANGED);
}

void MemoryGraphNode::_on_inspect_memory_pressed() {
    if (latest_grant_snapshot.is_valid()) {
        emit_signal("inspect_memory_requested", latest_grant_snapshot);
    }
}

void MemoryGraphNode::register_port_signature(int p_port_idx, bool p_is_output, uint32_t p_trait_mask) {
    if (p_is_output) {
        output_port_signatures[p_port_idx] = p_trait_mask;
    } else {
        input_port_signatures[p_port_idx] = p_trait_mask;
    }
}

uint32_t MemoryGraphNode::get_port_signature(int p_port_idx, bool p_is_output) const {
    if (p_is_output) {
        auto it = output_port_signatures.find(p_port_idx);
        return it != output_port_signatures.end() ? it->second : TRAIT_NONE;
    } else {
        auto it = input_port_signatures.find(p_port_idx);
        return it != input_port_signatures.end() ? it->second : TRAIT_NONE;
    }
}

} // namespace ideam::godot_ext