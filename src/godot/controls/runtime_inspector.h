#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/color_picker_button.hpp>

namespace ideam::godot_ext {

class RuntimeInspector : public godot::VBoxContainer {
    GDCLASS(RuntimeInspector, godot::VBoxContainer)

private:
    // Cached resolved type for properties requesting the dynamic "T" type
    godot::Variant::Type current_t_type = godot::Variant::NIL;
    
    // Internal state cache to handle deep mutations (like Arrays of Structs)
    godot::Dictionary cached_state;

protected:
    static void _bind_methods();

    // UI Generation Routers
    godot::Control* _create_control_for_property(const godot::Dictionary& p_prop, const godot::Variant& p_current_value);
    
    godot::Control* _create_bool_edit(const godot::Dictionary& p_prop, const godot::Variant& p_value);
    godot::Control* _create_spinbox(const godot::Dictionary& p_prop, const godot::Variant& p_value, bool p_is_float);
    godot::Control* _create_enum_dropdown(const godot::Dictionary& p_prop, const godot::Variant& p_value);
    godot::Control* _create_bitfield_edit(const godot::Dictionary& p_prop, const godot::Variant& p_value);
    godot::Control* _create_vector_edit(const godot::Dictionary& p_prop, const godot::Variant& p_value, int p_dimensions);
    godot::Control* _create_color_edit(const godot::Dictionary& p_prop, const godot::Variant& p_value);
    godot::Control* _create_packed_color_edit(const godot::Dictionary& p_prop, const godot::Variant& p_value);
    godot::Control* _create_array_edit(const godot::Dictionary& p_prop, const godot::Variant& p_value);
    godot::Control* _create_resource_picker(const godot::Dictionary& p_prop, const godot::Variant& p_value);
    godot::Control* _create_struct_edit(const godot::Dictionary& p_prop, const godot::Variant& p_value);
    
    // Array Struct Mutators
    void _on_array_element_added(const godot::StringName& p_array_name, const godot::Array& p_schema);
    void _on_array_element_removed(const godot::StringName& p_array_name, int p_index);

    // Bound Callbacks for specific Godot UI components
    void _on_bool_toggled(bool p_pressed, const godot::StringName& p_prop_name);
    void _on_spinbox_value_changed(double p_value, const godot::StringName& p_prop_name);
    void _on_hex_text_submitted(const godot::String& p_text, const godot::StringName& p_prop_name);
    void _on_option_item_selected(int p_index, const godot::StringName& p_prop_name);
    void _on_bitfield_toggled(bool p_pressed, const godot::StringName& p_prop_name, int p_bit_index, int p_current_mask);
    void _on_vector_component_changed(double p_value, const godot::StringName& p_prop_name, int p_component, godot::Variant p_current_vector);
    void _on_color_changed(const godot::Color& p_color, const godot::StringName& p_prop_name);
    void _on_packed_color_changed(const godot::Color& p_color, const godot::StringName& p_prop_name);

    // The universal exit funnel, capable of resolving deep struct paths
    void _emit_property_changed(const godot::StringName& p_prop_name, const godot::Variant& p_value);

public:
    RuntimeInspector();
    virtual ~RuntimeInspector() override = default;

    void clear_inspector();

    /**
     * @brief Parses the DOD logic properties and builds the UI.
     * @param p_properties The array of dictionaries returned by get_ui_properties()
     * @param p_state Dictionary of the current parameter state to populate the UI
     * @param p_resolved_t The Variant::Type that "T" currently represents based on node configuration
     */
    void build_inspector(const godot::Array& p_properties, const godot::Dictionary& p_state, godot::Variant::Type p_resolved_t);
};

} // namespace ideam::godot_ext