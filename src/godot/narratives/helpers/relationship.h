#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

// Forward declarations
class Narreme;

class Relationship : public godot::RefCounted {
    GDCLASS(Relationship, godot::RefCounted)

private:
    // DOD NOTE: While RefCounted is better than Node, instantiating an object 
    // for every relationship still creates Array of Structures (AoS) heap fragmentation. 
    // In IDEAM Core, we will use a BridgeView  to link a parent entity 
    // to a contiguous block of child relational IDs and metadata, eliminating 
    // this class entirely.
    godot::String title;
    godot::Ref<Narreme> relation;

protected:
    static void _bind_methods();

public:
    Relationship();
    ~Relationship();

    // Getters / Setters
    void set_title(const godot::String &p_title);
    godot::String get_title() const;

    void set_relation(const godot::Ref<Narreme> &p_relation);
    godot::Ref<Narreme> get_relation() const;
};

} // namespace ideam::godot_ext