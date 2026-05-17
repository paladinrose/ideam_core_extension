#include "ideam_graph_node_resource.h"
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void IdeamGraphNodeResource::_bind_methods() {
    // --- Identity Group ---
    ADD_GROUP("Identity", "");
    godot::ClassDB::bind_method(godot::D_METHOD("set_node_name", "name"), &IdeamGraphNodeResource::set_node_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_node_name"), &IdeamGraphNodeResource::get_node_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "node_name"), "set_node_name", "get_node_name");

    // --- DOD Execution Group ---
    ADD_GROUP("Execution Directives", "");
    godot::ClassDB::bind_method(godot::D_METHOD("set_execution_priority", "priority"), &IdeamGraphNodeResource::set_execution_priority);
    godot::ClassDB::bind_method(godot::D_METHOD("get_execution_priority"), &IdeamGraphNodeResource::get_execution_priority);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "execution_priority"), "set_execution_priority", "get_execution_priority");

    // --- Editor Visuals Group ---
    // Grouping these in the Inspector keeps the UI clean and cognitively separates 
    // visual properties from compilation properties.
    ADD_GROUP("Editor Visuals", "");
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_position_offset", "position"), &IdeamGraphNodeResource::set_position_offset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_position_offset"), &IdeamGraphNodeResource::get_position_offset);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR2, "position_offset"), "set_position_offset", "get_position_offset");

    godot::ClassDB::bind_method(godot::D_METHOD("set_size", "size"), &IdeamGraphNodeResource::set_size);
    godot::ClassDB::bind_method(godot::D_METHOD("get_size"), &IdeamGraphNodeResource::get_size);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR2, "size"), "set_size", "get_size");

    godot::ClassDB::bind_method(godot::D_METHOD("set_node_color", "color"), &IdeamGraphNodeResource::set_node_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_node_color"), &IdeamGraphNodeResource::get_node_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "node_color"), "set_node_color", "get_node_color");

    // --- Editor Metadata Group ---
    ADD_GROUP("Editor Metadata", "");
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_description", "description"), &IdeamGraphNodeResource::set_description);
    godot::ClassDB::bind_method(godot::D_METHOD("get_description"), &IdeamGraphNodeResource::get_description);
    // Use PROPERTY_HINT_MULTILINE_TEXT so the user gets a proper text box in the Godot inspector
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "description", godot::PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");

    // --- Compilation ---
    godot::ClassDB::bind_method(godot::D_METHOD("validate_for_compilation"), &IdeamGraphNodeResource::validate_for_compilation);
}

// --- Identity ---
void IdeamGraphNodeResource::set_node_name(const godot::StringName& p_name) { node_name = p_name; }
godot::StringName IdeamGraphNodeResource::get_node_name() const { return node_name; }

// --- Execution ---
void IdeamGraphNodeResource::set_execution_priority(int p_priority) { execution_priority = static_cast<int32_t>(p_priority); }
int IdeamGraphNodeResource::get_execution_priority() const { return static_cast<int>(execution_priority); }

// --- Visuals ---
void IdeamGraphNodeResource::set_position_offset(const godot::Vector2& p_pos) { position_offset = p_pos; }
godot::Vector2 IdeamGraphNodeResource::get_position_offset() const { return position_offset; }

void IdeamGraphNodeResource::set_size(const godot::Vector2& p_size) { size = p_size; }
godot::Vector2 IdeamGraphNodeResource::get_size() const { return size; }

void IdeamGraphNodeResource::set_node_color(const godot::Color& p_color) { node_color = p_color; }
godot::Color IdeamGraphNodeResource::get_node_color() const { return node_color; }

// --- Metadata ---
void IdeamGraphNodeResource::set_description(const godot::String& p_desc) { description = p_desc; }
godot::String IdeamGraphNodeResource::get_description() const { return description; }

// --- DOD Compilation Interface ---
bool IdeamGraphNodeResource::validate_for_compilation() const {
    // The graph compiler only cares about topology.
    // Notice how we intentionally DO NOT validate position, size, color, or description.
    // They are allowed to be empty or arbitrary because the execution layer doesn't care.
    if (node_name.is_empty()) {
        return false;
    }
    return true;
}

} // namespace ideam::godot_ext