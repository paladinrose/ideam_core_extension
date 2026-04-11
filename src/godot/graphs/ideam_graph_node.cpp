#include "ideam_graph_node.h"
#include "ideam_graph_edit.h"
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void IdeamGraphNode::_bind_methods() {
    // Fired when the user right-clicks the node
    ADD_SIGNAL(MethodInfo("context_clicked", PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "IdeamGraphNode"), PropertyInfo(Variant::VECTOR2, "at")));
    
    // Fired when an internal UI element (like a slider or LineEdit) is modified by the user
    ADD_SIGNAL(MethodInfo("property_changed", PropertyInfo(Variant::STRING_NAME, "blueprint_id"), PropertyInfo(Variant::STRING_NAME, "property_name"), PropertyInfo(Variant::NIL, "new_value")));

    ClassDB::bind_method(D_METHOD("initialize", "node_data"), &IdeamGraphNode::initialize);
    ClassDB::bind_method(D_METHOD("get_blueprint_id"), &IdeamGraphNode::get_blueprint_id);
    ClassDB::bind_method(D_METHOD("get_type_id"), &IdeamGraphNode::get_type_id);
    ClassDB::bind_method(D_METHOD("get_properties"), &IdeamGraphNode::get_properties);
    
    ClassDB::bind_method(D_METHOD("get_context_menu_options"), &IdeamGraphNode::get_context_menu_options);
    ClassDB::bind_method(D_METHOD("select_context_menu_option", "id"), &IdeamGraphNode::select_context_menu_option);
    
    // Protected virtual bindings
    ClassDB::bind_method(D_METHOD("_build_ui"), &IdeamGraphNode::_build_ui);
}

IdeamGraphNode::IdeamGraphNode() {
    set_resizable(true);
}

IdeamGraphNode::~IdeamGraphNode() {
}

void IdeamGraphNode::_ready() {
}

void IdeamGraphNode::initialize(const Dictionary& p_node_data) {
    if (p_node_data.has("name")) {
        blueprint_id = p_node_data["name"];
        set_name(blueprint_id);
    }
    
    if (p_node_data.has("type_id")) {
        type_id = static_cast<uint32_t>(p_node_data["type_id"]);
    }
    
    if (p_node_data.has("properties")) {
        properties = p_node_data["properties"];
    }

    if (p_node_data.has("position")) {
        set_position_offset(p_node_data["position"]);
    }

    // Call the virtual hook so derived nodes can configure their slots based on the parsed data
    _build_ui();
}

void IdeamGraphNode::_build_ui() {
    // Default implementation. 
    // Derived classes should override this to call set_slot() and add UI Controls.
    set_title(String("Type ID: ") + String::num_int64(type_id));
}

void IdeamGraphNode::_gui_input(const Ref<InputEvent> &p_event) {
    Ref<InputEventMouseButton> mouse_btn = p_event;

    // Detect Right-Click for Context Menus
    if (mouse_btn.is_valid() && mouse_btn->get_button_index() == MOUSE_BUTTON_RIGHT && mouse_btn->is_pressed()) {
        _emit_context_request();
        accept_event();
    }
}

void IdeamGraphNode::_emit_context_request() {
    // Route the popup logic centrally through the parent GraphEdit
    IdeamGraphEdit *parent = Object::cast_to<IdeamGraphEdit>(get_parent());
    if (parent) {
        parent->node_context_clicked(this);
    }
    
    // Emit signal for any external listeners
    emit_signal("context_clicked", this, get_global_position());
}

void IdeamGraphNode::emit_property_changed(const StringName& p_property_name, const Variant& p_new_value) {
    emit_signal("property_changed", blueprint_id, p_property_name, p_new_value);
}

TypedArray<String> IdeamGraphNode::get_context_menu_options() const {
    TypedArray<String> options;
    options.push_back("Delete Node");
    options.push_back("Duplicate Node");
    return options;
}

void IdeamGraphNode::select_context_menu_option(int p_id) {
    // In a real application, ID 0 would emit a signal to the GraphEdit to delete this node from the Resource
    switch(p_id) {
        case 0:
            UtilityFunctions::print("Requested Deletion for ", blueprint_id);
            break;
        case 1:
            UtilityFunctions::print("Requested Duplication for ", blueprint_id);
            break;
        default:
            break;
    }
}

} // namespace godot