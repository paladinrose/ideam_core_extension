#pragma once

#include "memory_buffer_selection_resource.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

/**
 * @class GrantPartResource
 * @brief Setup-time serialization for a single Buffer claim.
 * Maps directly to core::GrantPartPOD parameters. This object exists purely
 * to safely capture UI intent and serialize it to .tres format before the 
 * graph compiles it into a contiguous runtime struct.
 */
class GrantPartResource : public godot::Resource {
    GDCLASS(GrantPartResource, godot::Resource)

private:
    uint32_t buffer_id = 0xFFFFFFFF;
    uint32_t element_stride = 0;
    uint32_t column_id = 0;
    uint32_t buffer_type = 0;
    
    // Setup-time safety strings for user convenience in the editor
    godot::StringName target_buffer_name;
    godot::StringName target_column_name;

    // 0 = READ, 1 = WRITE (maps to core::BufferAccessMode)
    int access_mode = 0; 
    bool is_contiguous = false;

    godot::Ref<MemoryBufferSelectionResource> selection;

protected:
    static void _bind_methods();

public:
    GrantPartResource() = default;
    ~GrantPartResource() = default;

    // Setters
    void set_buffer_id(int p_id);
    void set_element_stride(int p_stride);
    void set_column_id(int p_id);
    void set_buffer_type(int p_type);
    void set_target_buffer_name(const godot::StringName& p_name);
    void set_target_column_name(const godot::StringName& p_name);
    void set_access_mode(int p_mode);
    void set_is_contiguous(bool p_contiguous);
    void set_selection(const godot::Ref<MemoryBufferSelectionResource>& p_selection);

    // Getters
    int get_buffer_id() const;
    int get_element_stride() const;
    int get_column_id() const;
    int get_buffer_type() const;
    godot::StringName get_target_buffer_name() const;
    godot::StringName get_target_column_name() const;
    int get_access_mode() const;
    bool get_is_contiguous() const;
    godot::Ref<MemoryBufferSelectionResource> get_selection() const;
};

} // namespace ideam::godot_ext