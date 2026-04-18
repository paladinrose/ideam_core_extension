#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class ManagedBufferProfile : public godot::RefCounted {
    GDCLASS(ManagedBufferProfile, godot::RefCounted)

private:
    godot::StringName consumer_name;
    godot::StringName purpose;
    int layout_type = 0;      
    int byte_footprint = 0;   
    int alignment = 64;       // Added: Hardware alignment requirement

protected:
    static void _bind_methods();

public:
    ManagedBufferProfile() = default;
    ~ManagedBufferProfile() = default;

    void set_consumer_name(const godot::StringName& p_name) { consumer_name = p_name; }
    godot::StringName get_consumer_name() const { return consumer_name; }

    void set_purpose(const godot::StringName& p_purpose) { purpose = p_purpose; }
    godot::StringName get_purpose() const { return purpose; }

    void set_layout_type(int p_type) { layout_type = p_type; }
    int get_layout_type() const { return layout_type; }

    void set_byte_footprint(int p_bytes) { byte_footprint = p_bytes; }
    int get_byte_footprint() const { return byte_footprint; }

    void set_alignment(int p_alignment) { alignment = p_alignment; }
    int get_alignment() const { return alignment; }
};

} // namespace ideam::godot_ext
