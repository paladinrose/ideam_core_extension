#include "runtime_inspector.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <godot_cpp/variant/vector4i.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/classes/margin_container.hpp>

using namespace godot;

namespace ideam::godot_ext {

void RuntimeInspector::_bind_methods() {
    ADD_SIGNAL(MethodInfo("property_changed", 
        PropertyInfo(Variant::STRING_NAME, "property_name"),
        PropertyInfo(Variant::NIL, "new_value")));

    ClassDB::bind_method(D_METHOD("_on_bool_toggled", "pressed", "prop_name"), &RuntimeInspector::_on_bool_toggled);
    ClassDB::bind_method(D_METHOD("_on_spinbox_value_changed", "value", "prop_name"), &RuntimeInspector::_on_spinbox_value_changed);
    ClassDB::bind_method(D_METHOD("_on_hex_text_submitted", "text", "prop_name"), &RuntimeInspector::_on_hex_text_submitted);
    ClassDB::bind_method(D_METHOD("_on_option_item_selected", "index", "prop_name"), &RuntimeInspector::_on_option_item_selected);
    ClassDB::bind_method(D_METHOD("_on_bitfield_toggled", "pressed", "prop_name", "bit_index", "current_mask"), &RuntimeInspector::_on_bitfield_toggled);
    ClassDB::bind_method(D_METHOD("_on_vector_component_changed", "value", "prop_name", "component", "current_vector"), &RuntimeInspector::_on_vector_component_changed);
    ClassDB::bind_method(D_METHOD("_on_color_changed", "color", "prop_name"), &RuntimeInspector::_on_color_changed);
    ClassDB::bind_method(D_METHOD("_on_packed_color_changed", "color", "prop_name"), &RuntimeInspector::_on_packed_color_changed);
    
    // Array Struct Mutators
    ClassDB::bind_method(D_METHOD("_on_array_element_added", "array_name", "schema"), &RuntimeInspector::_on_array_element_added);
    ClassDB::bind_method(D_METHOD("_on_array_element_removed", "array_name", "index"), &RuntimeInspector::_on_array_element_removed);
}

RuntimeInspector::RuntimeInspector() {
}

void RuntimeInspector::clear_inspector() {
    for (int i = 0; i < get_child_count(); ++i) {
        Node* child = get_child(i);
        child->queue_free();
    }
}

void RuntimeInspector::build_inspector(const Array& p_properties, const Dictionary& p_state, Variant::Type p_resolved_t) {
    clear_inspector();
    current_t_type = p_resolved_t;
    cached_state = p_state.duplicate(true); // Cache state for deep mutations

    for (int i = 0; i < p_properties.size(); ++i) {
        Dictionary prop_def = p_properties[i];
        StringName prop_name = prop_def["name"];

        Variant current_val = p_state.has(prop_name) ? p_state[prop_name] : Variant();

        HBoxContainer* row = memnew(HBoxContainer);
        
        // For Array properties, we push the label handling into the creator 
        // to give it full structural width.
        if (prop_def.has("type") && (int)prop_def["type"] == Variant::ARRAY) {
            Control* array_control = _create_control_for_property(prop_def, current_val);
            if (array_control) {
                array_control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
                row->add_child(array_control);
            }
        } else {
            Label* label = memnew(Label);
            label->set_text(String(prop_name).capitalize());
            label->set_custom_minimum_size(Vector2(140, 0)); 
            row->add_child(label);

            Control* input_control = _create_control_for_property(prop_def, current_val);
            if (input_control) {
                input_control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
                row->add_child(input_control);
            }
        }

        add_child(row);
    }
}

Control* RuntimeInspector::_create_control_for_property(const Dictionary& p_prop, const Variant& p_current_value) {
    Variant type_val = p_prop["type"];
    Variant::Type actual_type = Variant::NIL;

    if (type_val.get_type() == Variant::STRING && String(type_val) == "T") {
        actual_type = current_t_type;
    } else {
        actual_type = static_cast<Variant::Type>((int)type_val);
    }

    int hint = p_prop.has("hint") ? (int)p_prop["hint"] : PROPERTY_HINT_NONE;
    String hint_str = p_prop.has("hint_string") ? String(p_prop["hint_string"]) : "";

    switch (actual_type) {
        case Variant::BOOL:
            return _create_bool_edit(p_prop, p_current_value);
        case Variant::INT:
            if (hint == PROPERTY_HINT_ENUM) return _create_enum_dropdown(p_prop, p_current_value);
            if (hint == PROPERTY_HINT_FLAGS || hint_str.begins_with("bitfield:")) return _create_bitfield_edit(p_prop, p_current_value);
            if (hint_str == "packed_color") return _create_packed_color_edit(p_prop, p_current_value);
            return _create_spinbox(p_prop, p_current_value, false);
        case Variant::FLOAT:
            return _create_spinbox(p_prop, p_current_value, true);
        case Variant::VECTOR2:
        case Variant::VECTOR2I:
            return _create_vector_edit(p_prop, p_current_value, 2);
        case Variant::VECTOR3:
        case Variant::VECTOR3I:
            return _create_vector_edit(p_prop, p_current_value, 3);
        case Variant::VECTOR4:
        case Variant::VECTOR4I:
            return _create_vector_edit(p_prop, p_current_value, 4);
        case Variant::COLOR:
            return _create_color_edit(p_prop, p_current_value);
        case Variant::ARRAY:
            return _create_array_edit(p_prop, p_current_value);
        case Variant::OBJECT:
            if (hint == PROPERTY_HINT_RESOURCE_TYPE) return _create_resource_picker(p_prop, p_current_value);
            break;
        default:
            break;
    }

    Label* error_lbl = memnew(Label);
    error_lbl->set_text("[Unsupported DOD Type]");
    return error_lbl;
}

// ... [Existing _create_bool_edit to _create_packed_color_edit remain unchanged] ...

Control* RuntimeInspector::_create_bool_edit(const Dictionary& p_prop, const Variant& p_value) {
    CheckBox* cb = memnew(CheckBox);
    cb->set_text("Enabled");
    if (p_value.get_type() != Variant::NIL) {
        cb->set_pressed(static_cast<bool>(p_value));
    }
    StringName prop_name = p_prop["name"];
    cb->connect("toggled", Callable(this, "_on_bool_toggled").bind(prop_name));
    return cb;
}

Control* RuntimeInspector::_create_spinbox(const Dictionary& p_prop, const Variant& p_value, bool p_is_float) {
    HBoxContainer* container = memnew(HBoxContainer);
    StringName prop_name = p_prop["name"];
    String hint_str = p_prop.has("hint_string") ? String(p_prop["hint_string"]) : "";

    SpinBox* spinbox = memnew(SpinBox);
    spinbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    spinbox->set_step(p_is_float ? 0.001 : 1.0);
    spinbox->set_allow_greater(true);
    spinbox->set_allow_lesser(true);

    if (p_value.get_type() != Variant::NIL) {
        spinbox->set_value(static_cast<double>(p_value));
    }

    if (hint_str.begins_with("suffix:")) {
        spinbox->set_suffix(" " + hint_str.trim_prefix("suffix:"));
    }

    spinbox->connect("value_changed", Callable(this, "_on_spinbox_value_changed").bind(prop_name));
    container->add_child(spinbox);

    if (!p_is_float && hint_str.contains("hex")) {
        LineEdit* hex_view = memnew(LineEdit);
        hex_view->set_custom_minimum_size(Vector2(80, 0));
        uint64_t val = (p_value.get_type() != Variant::NIL) ? static_cast<uint64_t>(p_value) : 0;
        hex_view->set_text("0x" + String::num_uint64(val, 16));
        hex_view->connect("text_submitted", Callable(this, "_on_hex_text_submitted").bind(prop_name));
        container->add_child(hex_view);
    }

    if (p_is_float && hint_str.contains("f64")) {
        Label* f64_tag = memnew(Label);
        f64_tag->set_text("[f64]");
        f64_tag->set_modulate(Color(0.5, 0.8, 1.0)); 
        container->add_child(f64_tag);
    }

    return container;
}

Control* RuntimeInspector::_create_enum_dropdown(const Dictionary& p_prop, const Variant& p_value) {
    OptionButton* ob = memnew(OptionButton);
    if (p_prop.has("hint_string")) {
        String hint_str = p_prop["hint_string"];
        PackedStringArray options = hint_str.split(",");
        for (int i = 0; i < options.size(); ++i) {
            ob->add_item(options[i], i);
        }
    }
    if (p_value.get_type() != Variant::NIL) {
        ob->select(static_cast<int>(p_value));
    }
    StringName prop_name = p_prop["name"];
    ob->connect("item_selected", Callable(this, "_on_option_item_selected").bind(prop_name));
    return ob;
}

Control* RuntimeInspector::_create_bitfield_edit(const Dictionary& p_prop, const Variant& p_value) {
    HBoxContainer* hbox = memnew(HBoxContainer);
    StringName prop_name = p_prop["name"];
    int current_mask = (p_value.get_type() != Variant::NIL) ? static_cast<int>(p_value) : 0;
    
    int bit_count = 8; 
    String hint_str = p_prop.has("hint_string") ? String(p_prop["hint_string"]) : "";
    if (hint_str.begins_with("bitfield:")) {
        bit_count = hint_str.trim_prefix("bitfield:").to_int();
    } else if (p_prop.has("hint") && (int)p_prop["hint"] == PROPERTY_HINT_FLAGS) {
        PackedStringArray flags = hint_str.split(",");
        bit_count = flags.size();
    }

    for (int i = 0; i < bit_count; ++i) {
        CheckBox* cb = memnew(CheckBox);
        cb->set_tooltip_text(String("Bit ") + String::num_int64(i));
        cb->set_pressed((current_mask & (1 << i)) != 0);
        cb->connect("toggled", Callable(this, "_on_bitfield_toggled").bind(prop_name, i, current_mask));
        hbox->add_child(cb);
    }
    return hbox;
}

Control* RuntimeInspector::_create_vector_edit(const Dictionary& p_prop, const Variant& p_value, int p_dimensions) {
    HBoxContainer* hbox = memnew(HBoxContainer);
    StringName prop_name = p_prop["name"];
    String hint_str = p_prop.has("hint_string") ? String(p_prop["hint_string"]) : "";

    Variant current_vec = p_value;
    if (current_vec.get_type() == Variant::NIL) {
        if (p_dimensions == 2) current_vec = Variant(Vector2(0,0));
        else if (p_dimensions == 3) current_vec = Variant(Vector3(0,0,0));
        else current_vec = Variant(Vector4(0,0,0,0));
    }

    bool is_simd = hint_str.contains("simd");
    const char* cartesian_axes[] = {"X", "Y", "Z", "W"};
    const char* simd_lanes[] = {"L0", "L1", "L2", "L3"};
    const char** labels = is_simd ? simd_lanes : cartesian_axes;
    
    for (int i = 0; i < p_dimensions; ++i) {
        SpinBox* comp_spin = memnew(SpinBox);
        comp_spin->set_step(0.001);
        comp_spin->set_allow_greater(true);
        comp_spin->set_allow_lesser(true);
        comp_spin->set_prefix(String(labels[i]) + ": ");
        comp_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);

        if (p_dimensions == 2) comp_spin->set_value(static_cast<Vector2>(current_vec)[i]);
        else if (p_dimensions == 3) comp_spin->set_value(static_cast<Vector3>(current_vec)[i]);
        else if (p_dimensions == 4) comp_spin->set_value(static_cast<Vector4>(current_vec)[i]);

        comp_spin->connect("value_changed", Callable(this, "_on_vector_component_changed").bind(prop_name, i, current_vec));
        hbox->add_child(comp_spin);
    }
    return hbox;
}

Control* RuntimeInspector::_create_color_edit(const Dictionary& p_prop, const Variant& p_value) {
    ColorPickerButton* cp = memnew(ColorPickerButton);
    if (p_value.get_type() != Variant::NIL) {
        cp->set_pick_color(static_cast<Color>(p_value));
    } else {
        cp->set_pick_color(Color(1,1,1,1));
    }
    StringName prop_name = p_prop["name"];
    cp->connect("color_changed", Callable(this, "_on_color_changed").bind(prop_name));
    return cp;
}

Control* RuntimeInspector::_create_packed_color_edit(const Dictionary& p_prop, const Variant& p_value) {
    ColorPickerButton* cp = memnew(ColorPickerButton);
    uint32_t packed_hex = (p_value.get_type() != Variant::NIL) ? static_cast<uint32_t>(static_cast<int>(p_value)) : 0xFFFFFFFF;
    
    float r = ((packed_hex >> 24) & 0xFF) / 255.0f;
    float g = ((packed_hex >> 16) & 0xFF) / 255.0f;
    float b = ((packed_hex >> 8) & 0xFF) / 255.0f;
    float a = (packed_hex & 0xFF) / 255.0f;
    
    cp->set_pick_color(Color(r, g, b, a));
    
    StringName prop_name = p_prop["name"];
    cp->connect("color_changed", Callable(this, "_on_packed_color_changed").bind(prop_name));
    return cp;
}

Control* RuntimeInspector::_create_resource_picker(const Dictionary& p_prop, const Variant& p_value) {
    Button* pick_btn = memnew(Button);
    pick_btn->set_text(p_value.get_type() == Variant::OBJECT ? "[Resource Loaded]" : "[Assign Resource]");
    return pick_btn;
}

Control* RuntimeInspector::_create_array_edit(const Dictionary& p_prop, const Variant& p_value) {
    VBoxContainer* vbox = memnew(VBoxContainer);
    StringName base_prop_name = p_prop["name"];

    // Title label for the Array
    Label* title_lbl = memnew(Label);
    title_lbl->set_text(String(base_prop_name).capitalize() + " (Array)");
    title_lbl->set_modulate(Color(0.8, 0.8, 0.8));
    vbox->add_child(title_lbl);

    if (p_prop.has("struct_properties")) {
        Array struct_schema = p_prop["struct_properties"];
        Array current_array = (p_value.get_type() == Variant::ARRAY) ? static_cast<Array>(p_value) : Array();

        // Recursively build out the controls for each active element in the array
        for (int i = 0; i < current_array.size(); ++i) {
            VBoxContainer* element_vbox = memnew(VBoxContainer);
            
            HSeparator* sep = memnew(HSeparator);
            element_vbox->add_child(sep);

            // Element Header (Index + Remove Button)
            HBoxContainer* header = memnew(HBoxContainer);
            Label* idx_lbl = memnew(Label);
            idx_lbl->set_text(String("Element ") + String::num_int64(i));
            idx_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            header->add_child(idx_lbl);

            Button* remove_btn = memnew(Button);
            remove_btn->set_text("X");
            remove_btn->connect("pressed", Callable(this, "_on_array_element_removed").bind(base_prop_name, i));
            header->add_child(remove_btn);

            element_vbox->add_child(header);

            Dictionary element_val;
            if (current_array[i].get_type() == Variant::DICTIONARY) {
                element_val = current_array[i];
            }

            // Drill into the struct definition
            for (int j = 0; j < struct_schema.size(); ++j) {
                Dictionary sub_prop = struct_schema[j];
                String sub_name = sub_prop["name"];

                // Path Spoofing: Overwrite the property name with our nested path
                Dictionary path_prop = sub_prop.duplicate();
                path_prop["name"] = String(base_prop_name) + "/" + String::num_int64(i) + "/" + sub_name;

                Variant sub_val = element_val.has(sub_name) ? element_val[sub_name] : Variant();

                HBoxContainer* row = memnew(HBoxContainer);
                Label* label = memnew(Label);
                label->set_text(sub_name.capitalize());
                label->set_custom_minimum_size(Vector2(120, 0));
                row->add_child(label);

                Control* input_control = _create_control_for_property(path_prop, sub_val);
                if (input_control) {
                    input_control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
                    row->add_child(input_control);
                }
                element_vbox->add_child(row);
            }
            
            // Indent the struct contents slightly for visual grouping
            MarginContainer* margin = memnew(MarginContainer);
            margin->add_theme_constant_override("margin_left", 15);
            margin->add_child(element_vbox);
            vbox->add_child(margin);
        }

        Button* add_btn = memnew(Button);
        add_btn->set_text("Add Element");
        add_btn->connect("pressed", Callable(this, "_on_array_element_added").bind(base_prop_name, struct_schema));
        vbox->add_child(add_btn);

    } else {
        // Flat Array Fallback
        Button* add_btn = memnew(Button);
        add_btn->set_text("Edit Array...");
        vbox->add_child(add_btn);
    }

    return vbox;
}

// --- Signal Interceptors ---

void RuntimeInspector::_on_array_element_added(const StringName& p_array_name, const Array& p_schema) {
    Array arr;
    if (cached_state.has(p_array_name) && cached_state[p_array_name].get_type() == Variant::ARRAY) {
        arr = static_cast<Array>(cached_state[p_array_name]).duplicate(true);
    }
    
    // Add a blank Dictionary element. The backend will initialize it to default DOD values.
    arr.push_back(Dictionary()); 
    
    cached_state[p_array_name] = arr;
    emit_signal("property_changed", p_array_name, arr);
}

void RuntimeInspector::_on_array_element_removed(const StringName& p_array_name, int p_index) {
    if (cached_state.has(p_array_name) && cached_state[p_array_name].get_type() == Variant::ARRAY) {
        Array arr = static_cast<Array>(cached_state[p_array_name]).duplicate(true);
        if (p_index >= 0 && p_index < arr.size()) {
            arr.remove_at(p_index);
            cached_state[p_array_name] = arr;
            emit_signal("property_changed", p_array_name, arr);
        }
    }
}

void RuntimeInspector::_on_bool_toggled(bool p_pressed, const StringName& p_prop_name) {
    _emit_property_changed(p_prop_name, p_pressed);
}

void RuntimeInspector::_on_spinbox_value_changed(double p_value, const StringName& p_prop_name) {
    _emit_property_changed(p_prop_name, p_value);
}

void RuntimeInspector::_on_hex_text_submitted(const String& p_text, const StringName& p_prop_name) {
    String clean_text = p_text.trim_prefix("0x");
    uint64_t val = clean_text.hex_to_int();
    _emit_property_changed(p_prop_name, static_cast<int>(val));
}

void RuntimeInspector::_on_option_item_selected(int p_index, const StringName& p_prop_name) {
    _emit_property_changed(p_prop_name, p_index);
}

void RuntimeInspector::_on_bitfield_toggled(bool p_pressed, const StringName& p_prop_name, int p_bit_index, int p_current_mask) {
    int new_mask = p_current_mask;
    if (p_pressed) {
        new_mask |= (1 << p_bit_index);
    } else {
        new_mask &= ~(1 << p_bit_index);
    }
    _emit_property_changed(p_prop_name, new_mask);
}

void RuntimeInspector::_on_vector_component_changed(double p_value, const StringName& p_prop_name, int p_component, Variant p_current_vector) {
    Variant new_vec = p_current_vector;

    if (new_vec.get_type() == Variant::VECTOR2 || new_vec.get_type() == Variant::VECTOR2I) {
        Vector2 v = new_vec;
        v[p_component] = p_value;
        new_vec = v;
    } else if (new_vec.get_type() == Variant::VECTOR3 || new_vec.get_type() == Variant::VECTOR3I) {
        Vector3 v = new_vec;
        v[p_component] = p_value;
        new_vec = v;
    } else if (new_vec.get_type() == Variant::VECTOR4 || new_vec.get_type() == Variant::VECTOR4I) {
        Vector4 v = new_vec;
        v[p_component] = p_value;
        new_vec = v;
    }

    _emit_property_changed(p_prop_name, new_vec);
}

void RuntimeInspector::_on_color_changed(const Color& p_color, const StringName& p_prop_name) {
    _emit_property_changed(p_prop_name, p_color);
}

void RuntimeInspector::_on_packed_color_changed(const Color& p_color, const StringName& p_prop_name) {
    uint32_t r = static_cast<uint32_t>(p_color.r * 255.0f) & 0xFF;
    uint32_t g = static_cast<uint32_t>(p_color.g * 255.0f) & 0xFF;
    uint32_t b = static_cast<uint32_t>(p_color.b * 255.0f) & 0xFF;
    uint32_t a = static_cast<uint32_t>(p_color.a * 255.0f) & 0xFF;
    
    uint32_t packed_hex = (r << 24) | (g << 16) | (b << 8) | a;
    _emit_property_changed(p_prop_name, static_cast<int>(packed_hex));
}

void RuntimeInspector::_emit_property_changed(const StringName& p_prop_name, const Variant& p_value) {
    String prop_str = p_prop_name;
    
    // Path Resolution: Intercept nested struct updates
    if (prop_str.contains("/")) {
        PackedStringArray parts = prop_str.split("/");
        
        // Expected Format: "array_name/index/property_name"
        if (parts.size() == 3) {
            StringName array_name = parts[0];
            int index = parts[1].to_int();
            StringName sub_prop = parts[2];

            if (cached_state.has(array_name) && cached_state[array_name].get_type() == Variant::ARRAY) {
                Array arr = static_cast<Array>(cached_state[array_name]).duplicate(true);
                
                if (index >= 0 && index < arr.size()) {
                    // Update the specific dictionary field
                    Dictionary elem = arr[index];
                    elem[sub_prop] = p_value;
                    arr[index] = elem;
                    
                    // Commit to cache and emit top-level array
                    cached_state[array_name] = arr;
                    emit_signal("property_changed", array_name, arr);
                    return; // Prevent standard emission
                }
            }
        }
    }
    
    // Standard flat property routing
    cached_state[p_prop_name] = p_value;
    emit_signal("property_changed", p_prop_name, p_value);
}

} // namespace ideam::godot_ext
