#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/color.hpp>

namespace ideam::core {
    // Forward declaring the core DOD structures ensures clean boundaries.
}

namespace ideam::godot_ext {

/**
 * @class IdeamGraphNodeResource
 * @brief The foundational setup-time authoring data for a graph node.
 * Contains both identity data (for topological compilation) and purely 
 * visual Editor metadata that is explicitly ignored by the runtime DOD compiler.
 */
class IdeamGraphNodeResource : public godot::Resource {
    GDCLASS(IdeamGraphNodeResource, godot::Resource)

private:
    // --- Topological Identity ---
    godot::StringName node_name;
    
    // --- DOD Execution Directives ---
    // Maps directly to BuildNodesSoA::execution_priority.
    // Used to deterministically sort tasks within the same parallel execution wave,
    // maximizing cache locality and resolving data-contention ties.
    int32_t execution_priority = 0;

    // --- Editor-Only Visual & Metadata ---
    // These properties are serialized for the Godot Editor but are strictly 
    // stripped away and ignored by the DOD Graph Compiler to preserve cache fidelity.
    godot::Vector2 position_offset;
    godot::Vector2 size;
    godot::Color node_color = godot::Color(1.0f, 1.0f, 1.0f, 1.0f); // Default White
    godot::String description;

protected:
    static void _bind_methods();

public:
    IdeamGraphNodeResource() = default;
    virtual ~IdeamGraphNodeResource() = default;

    // --- Identity ---
    void set_node_name(const godot::StringName& p_name);
    godot::StringName get_node_name() const;

    // --- Execution ---
    void set_execution_priority(int p_priority);
    int get_execution_priority() const;

    // --- Visuals ---
    void set_position_offset(const godot::Vector2& p_pos);
    godot::Vector2 get_position_offset() const;

    void set_size(const godot::Vector2& p_size);
    godot::Vector2 get_size() const;

    void set_node_color(const godot::Color& p_color);
    godot::Color get_node_color() const;

    // --- Metadata ---
    void set_description(const godot::String& p_desc);
    godot::String get_description() const;

    // --- DOD Compilation Interface ---
    virtual bool validate_for_compilation() const;
};

} // namespace ideam::godot_ext