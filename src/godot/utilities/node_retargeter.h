#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class NodeRetargeter : public godot::Resource {
    GDCLASS(NodeRetargeter, godot::Resource)

protected:
    static void _bind_methods();

private:
    godot::TypedArray<godot::String> source_targets;
    godot::TypedArray<godot::String> destinations;
    godot::TypedArray<godot::String> destination_retarget_properties;

    // Internal helper
    godot::Node* _get_node(godot::Node* _root, const godot::String& _name) const;

public:
    NodeRetargeter();
    ~NodeRetargeter();

    // Setters / Getters
    void set_source_targets(const godot::TypedArray<godot::String>& p_targets);
    godot::TypedArray<godot::String> get_source_targets() const;

    void set_destinations(const godot::TypedArray<godot::String>& p_destinations);
    godot::TypedArray<godot::String> get_destinations() const;

    void set_destination_retarget_properties(const godot::TypedArray<godot::String>& p_properties);
    godot::TypedArray<godot::String> get_destination_retarget_properties() const;

    // Class Functions
    void add_retarget(const godot::String& source, const godot::String& destination, const godot::String& destination_property);
    void remove_retarget_at(int id);
    void retarget(godot::Node* source_root, godot::Node* destination_root);
    void clear_set_targets(godot::Node* source_root, godot::Node* destination_root);
};

} // namespace ideam::godot_ext