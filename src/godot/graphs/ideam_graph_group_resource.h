#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class IdeamGraphGroupResource : public godot::Resource {
    GDCLASS(IdeamGraphGroupResource, godot::Resource)

private:
    godot::StringName group_name;
    godot::String title;
    godot::Vector2 position;
    godot::Vector2 size;
    godot::TypedArray<godot::StringName> nodes;

protected:
    static void _bind_methods();

public:
    IdeamGraphGroupResource() = default;
    virtual ~IdeamGraphGroupResource() = default;

    void set_group_name(const godot::StringName& p_name);
    godot::StringName get_group_name() const;

    void set_title(const godot::String& p_title);
    godot::String get_title() const; // Fixed: Returns by value and marked const

    void set_position(const godot::Vector2& p_pos);
    godot::Vector2 get_position() const;

    void set_size(const godot::Vector2& p_size);
    godot::Vector2 get_size() const;

    void set_nodes(const godot::TypedArray<godot::StringName>& p_nodes);
    godot::TypedArray<godot::StringName> get_nodes() const;
};

} // namespace ideam::godot_ext