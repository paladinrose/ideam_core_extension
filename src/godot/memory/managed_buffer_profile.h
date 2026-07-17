#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class ManagedBufferProfile : public godot::Resource {
    GDCLASS(ManagedBufferProfile, godot::Resource)

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

    void set_consumer_name(const godot::StringName& p_name);
    godot::StringName get_consumer_name() const;

    void set_purpose(const godot::StringName& p_purpose);
    godot::StringName get_purpose() const;

    void set_layout_type(int p_type);
    int get_layout_type() const;

    void set_byte_footprint(int p_bytes);
    int get_byte_footprint() const;

    void set_alignment(int p_alignment);
    int get_alignment() const;
};

} // namespace ideam::godot_ext
