#include "signal_connector.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace ideam::godot_ext {

void SignalConnector::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_source_targets", "targets"), &SignalConnector::set_source_targets);
    godot::ClassDB::bind_method(godot::D_METHOD("get_source_targets"), &SignalConnector::get_source_targets);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "source_targets", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_source_targets", "get_source_targets");

    godot::ClassDB::bind_method(godot::D_METHOD("set_source_target_functions", "functions"), &SignalConnector::set_source_target_functions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_source_target_functions"), &SignalConnector::get_source_target_functions);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "source_target_functions", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_source_target_functions", "get_source_target_functions");

    godot::ClassDB::bind_method(godot::D_METHOD("set_destinations", "destinations"), &SignalConnector::set_destinations);
    godot::ClassDB::bind_method(godot::D_METHOD("get_destinations"), &SignalConnector::get_destinations);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "destinations", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_destinations", "get_destinations");

    godot::ClassDB::bind_method(godot::D_METHOD("set_destination_signals", "signals"), &SignalConnector::set_destination_signals);
    godot::ClassDB::bind_method(godot::D_METHOD("get_destination_signals"), &SignalConnector::get_destination_signals);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "destination_signals", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_destination_signals", "get_destination_signals");

    godot::ClassDB::bind_method(godot::D_METHOD("add_signal_connection", "source", "source_function", "destination", "destination_signal"), &SignalConnector::add_signal_connection);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_signal_connection_at", "id"), &SignalConnector::remove_signal_connection_at);
    godot::ClassDB::bind_method(godot::D_METHOD("connect_signals", "source_root", "destination_root"), &SignalConnector::connect_signals);
    godot::ClassDB::bind_method(godot::D_METHOD("disconnect_signals", "source_root", "destination_root"), &SignalConnector::disconnect_signals);
}

SignalConnector::SignalConnector() {}
SignalConnector::~SignalConnector() {}

void SignalConnector::set_source_targets(const godot::TypedArray<godot::String>& p_targets) { source_targets = p_targets; }
godot::TypedArray<godot::String> SignalConnector::get_source_targets() const { return source_targets; }

void SignalConnector::set_source_target_functions(const godot::TypedArray<godot::String>& p_functions) { source_target_functions = p_functions; }
godot::TypedArray<godot::String> SignalConnector::get_source_target_functions() const { return source_target_functions; }

void SignalConnector::set_destinations(const godot::TypedArray<godot::String>& p_destinations) { destinations = p_destinations; }
godot::TypedArray<godot::String> SignalConnector::get_destinations() const { return destinations; }

void SignalConnector::set_destination_signals(const godot::TypedArray<godot::String>& p_signals) { destination_signals = p_signals; }
godot::TypedArray<godot::String> SignalConnector::get_destination_signals() const { return destination_signals; }

void SignalConnector::add_signal_connection(const godot::String& source, const godot::String& source_function, const godot::String& destination, const godot::String& destination_signal) {
    source_targets.append(source);
    source_target_functions.append(source_function);
    destinations.append(destination);
    destination_signals.append(destination_signal);
}

void SignalConnector::remove_signal_connection_at(int id) {
    if (id < 0 || id >= source_targets.size()) {
        return;
    }
    source_targets.remove_at(id);
    source_target_functions.remove_at(id);
    destinations.remove_at(id);
    destination_signals.remove_at(id);
}

void SignalConnector::connect_signals(godot::Node* source_root, godot::Node* destination_root) {
    if (!source_root || !destination_root) return;

    for (int i = 0; i < source_targets.size(); ++i) {
        godot::Node* source = _get_node(source_root, source_targets[i]);
        if (!source) continue;
        if (!source->has_method(source_target_functions[i])) continue;

        godot::Node* destination = _get_node(destination_root, destinations[i]);
        if (!destination) continue;
        
        godot::StringName dest_signal = destination_signals[i];
        if (!destination->has_signal(dest_signal)) continue;

        godot::Callable source_callable(source, source_target_functions[i]);

        if (!destination->is_connected(dest_signal, source_callable)) {
            destination->connect(dest_signal, source_callable);
        }
    }
}

void SignalConnector::disconnect_signals(godot::Node* source_root, godot::Node* destination_root) {
    if (!source_root || !destination_root) return;

    for (int i = 0; i < source_targets.size(); ++i) {
        godot::Node* source = _get_node(source_root, source_targets[i]);
        if (!source) continue;
        if (!source->has_method(source_target_functions[i])) continue;

        godot::Node* destination = _get_node(destination_root, destinations[i]);
        if (!destination) continue;
        
        godot::StringName dest_signal = destination_signals[i];
        if (!destination->has_signal(dest_signal)) continue;

        godot::Callable source_callable(source, source_target_functions[i]);

        if (destination->is_connected(dest_signal, source_callable)) {
            destination->disconnect(dest_signal, source_callable);
        }
    }
}

godot::Node* SignalConnector::_get_node(godot::Node* _root, const godot::String& _name) const {
    if (!_root) return nullptr;

    if (_name == godot::String("root")) {
        return _root;
    } else if (_root->has_node(godot::NodePath(_name))) {
        return _root->get_node<godot::Node>(godot::NodePath(_name));
    }
    
    return nullptr;
}

} // namespace ideam::godot_ext