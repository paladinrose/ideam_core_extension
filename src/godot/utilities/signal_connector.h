#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class SignalConnector : public godot::Resource {
    GDCLASS(SignalConnector, godot::Resource)

protected:
    static void _bind_methods();

private:
    godot::TypedArray<godot::String> source_targets;
    godot::TypedArray<godot::String> source_target_functions;
    godot::TypedArray<godot::String> destinations;
    godot::TypedArray<godot::String> destination_signals;

    // Internal helper
    godot::Node* _get_node(godot::Node* _root, const godot::String& _name) const;

public:
    SignalConnector();
    ~SignalConnector();

    // Setters / Getters
    void set_source_targets(const godot::TypedArray<godot::String>& p_targets);
    godot::TypedArray<godot::String> get_source_targets() const;

    void set_source_target_functions(const godot::TypedArray<godot::String>& p_functions);
    godot::TypedArray<godot::String> get_source_target_functions() const;

    void set_destinations(const godot::TypedArray<godot::String>& p_destinations);
    godot::TypedArray<godot::String> get_destinations() const;

    void set_destination_signals(const godot::TypedArray<godot::String>& p_signals);
    godot::TypedArray<godot::String> get_destination_signals() const;

    // Class Functions
    void add_signal_connection(const godot::String& source, const godot::String& source_function, const godot::String& destination, const godot::String& destination_signal);
    void remove_signal_connection_at(int id);
    void connect_signals(godot::Node* source_root, godot::Node* destination_root);
    void disconnect_signals(godot::Node* source_root, godot::Node* destination_root);
};

} // namespace ideam::godot_ext