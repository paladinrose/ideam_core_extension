#include "node_retargeter.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace ideam::godot_ext {

void NodeRetargeter::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_source_targets", "targets"), &NodeRetargeter::set_source_targets);
    godot::ClassDB::bind_method(godot::D_METHOD("get_source_targets"), &NodeRetargeter::get_source_targets);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "source_targets", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_source_targets", "get_source_targets");

    godot::ClassDB::bind_method(godot::D_METHOD("set_destinations", "destinations"), &NodeRetargeter::set_destinations);
    godot::ClassDB::bind_method(godot::D_METHOD("get_destinations"), &NodeRetargeter::get_destinations);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "destinations", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_destinations", "get_destinations");

    godot::ClassDB::bind_method(godot::D_METHOD("set_destination_retarget_properties", "properties"), &NodeRetargeter::set_destination_retarget_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("get_destination_retarget_properties"), &NodeRetargeter::get_destination_retarget_properties);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "destination_retarget_properties", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_destination_retarget_properties", "get_destination_retarget_properties");

    godot::ClassDB::bind_method(godot::D_METHOD("add_retarget", "source", "destination", "destination_property"), &NodeRetargeter::add_retarget);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_retarget_at", "id"), &NodeRetargeter::remove_retarget_at);
    godot::ClassDB::bind_method(godot::D_METHOD("retarget", "source_root", "destination_root"), &NodeRetargeter::retarget);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_set_targets", "source_root", "destination_root"), &NodeRetargeter::clear_set_targets);
}

NodeRetargeter::NodeRetargeter() {}
NodeRetargeter::~NodeRetargeter() {}

void NodeRetargeter::set_source_targets(const godot::TypedArray<godot::String>& p_targets) { source_targets = p_targets; }
godot::TypedArray<godot::String> NodeRetargeter::get_source_targets() const { return source_targets; }

void NodeRetargeter::set_destinations(const godot::TypedArray<godot::String>& p_destinations) { destinations = p_destinations; }
godot::TypedArray<godot::String> NodeRetargeter::get_destinations() const { return destinations; }

void NodeRetargeter::set_destination_retarget_properties(const godot::TypedArray<godot::String>& p_properties) { destination_retarget_properties = p_properties; }
godot::TypedArray<godot::String> NodeRetargeter::get_destination_retarget_properties() const { return destination_retarget_properties; }

void NodeRetargeter::add_retarget(const godot::String& source, const godot::String& destination, const godot::String& destination_property) {
    source_targets.append(source);
    destinations.append(destination);
    destination_retarget_properties.append(destination_property);
}

void NodeRetargeter::remove_retarget_at(int id) {
    if (id < 0 || id >= source_targets.size()) {
        return;
    }
    source_targets.remove_at(id);
    destinations.remove_at(id);
    destination_retarget_properties.remove_at(id);
}

void NodeRetargeter::retarget(godot::Node* source_root, godot::Node* destination_root) {
    if (!source_root || !destination_root) return;

    for (int i = 0; i < source_targets.size(); ++i) {
        godot::Node* source = _get_node(source_root, source_targets[i]);
        if (!source) continue;

        godot::Node* destination = _get_node(destination_root, destinations[i]);
        if (!destination) continue;

        godot::StringName prop_name = destination_retarget_properties[i];

        // Emulate GDScript's 'if prop in destination' checking
        bool has_prop = false;
        godot::TypedArray<godot::Dictionary> prop_list = destination->get_property_list();
        for (int p = 0; p < prop_list.size(); ++p) {
            godot::Dictionary dict = prop_list[p];
            if (dict["name"] == prop_name) {
                has_prop = true;
                break;
            }
        }

        if (has_prop) {
            destination->set(prop_name, source);
        }
    }
}

void NodeRetargeter::clear_set_targets(godot::Node* source_root, godot::Node* destination_root) {
    if (!source_root || !destination_root) return;

    for (int i = 0; i < source_targets.size(); ++i) {
        godot::Node* source = _get_node(source_root, source_targets[i]);
        if (!source) continue;

        godot::Node* destination = _get_node(destination_root, destinations[i]);
        if (!destination) continue;

        godot::StringName prop_name = destination_retarget_properties[i];

        // Emulate GDScript's 'if prop in destination' checking
        bool has_prop = false;
        godot::TypedArray<godot::Dictionary> prop_list = destination->get_property_list();
        for (int p = 0; p < prop_list.size(); ++p) {
            godot::Dictionary dict = prop_list[p];
            if (dict["name"] == prop_name) {
                has_prop = true;
                break;
            }
        }

        if (has_prop) {
            godot::Variant current_val = destination->get(prop_name);
            if (current_val.get_type() == godot::Variant::OBJECT && godot::Object::cast_to<godot::Node>(current_val) == source) {
                destination->set(prop_name, nullptr);
            }
        }
    }
}

godot::Node* NodeRetargeter::_get_node(godot::Node* _root, const godot::String& _name) const {
    if (!_root) return nullptr;

    if (_name == godot::String("root")) {
        return _root;
    } else if (_root->has_node(godot::NodePath(_name))) {
        return _root->get_node<godot::Node>(godot::NodePath(_name));
    }
    
    return nullptr;
}

} // namespace ideam::godot_ext