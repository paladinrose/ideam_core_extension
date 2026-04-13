#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>

namespace ideam::godot_ext {

// DOD NOTE: Disentangling this enum is a great move. When we map this to 
// IDEAM's "Shadow Buffers" (Column Metadata Sequence), this enum will live 
// in a parallel Structure of Arrays (SoA) sequence alongside Group Masks[cite: 4].
// By pulling it out of the class, our SIMD bitwise operations [cite: 7] 
// won't need to know anything about the `Narreme` class itself.
enum NarremeConditionStatus : int8_t {
    CANNOT_MEET = -2,
    NOT_MET = -1,
    UNKNOWN = 0,
    MET = 1,
    PERMANENT = 2
};

class Narreme : public godot::Resource {
    GDCLASS(Narreme, godot::Resource)

protected:
    static void _bind_methods();

private:
    godot::String official_name;

public:
    Narreme();
    ~Narreme();

    // GODOT NOTE: Resources do not receive `_ready()` callbacks from the SceneTree.
    // I have replaced `_ready()` with an explicit `initialize()` function.
    virtual void initialize();

    // Getters and Setters
    void set_official_name(const godot::String &p_name);
    godot::String get_official_name() const;

    // DOD NOTE: While this is now a Resource, virtual functions still dictate 
    // an Array of Structures (AoS) memory layout and vtable lookups. 
    // In IDEAM C++26, we'll want to use `SingleElementView` or `SparseSetView` [cite: 33]
    // for these operations to guarantee zero-cost branchless execution[cite: 25].
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const;
    virtual int score_similarity(Narreme *p_similar) const;
    virtual godot::String get_class_name_str() const;
};

} // namespace ideam::godot_ext

// Godot macro requires the fully qualified name to register the variant
VARIANT_ENUM_CAST(ideam::godot_ext::NarremeConditionStatus);