#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include "../../core/memory/memory_buffer_selection_pod.h"

namespace ideam::godot_ext {

class MemoryBufferSelectionResource : public godot::Resource {
    GDCLASS(MemoryBufferSelectionResource, godot::Resource)

public:
    enum SelectionMode {
        MODE_SPARSE = 0, // Maps to core::SelectionMode::SPARSE
        MODE_DENSE  = 1, // Maps to core::SelectionMode::DENSE
        MODE_RANGE  = 2  // Maps to core::SelectionMode::RANGE
    };

private:
    SelectionMode mode = MODE_RANGE;
    
    // Total number of elements actually selected
    int element_count = 0; 
    
    // Used exclusively for MODE_RANGE
    int start_index = 0;   

    // Emulates the 'data.indices' pointer for MODE_SPARSE
    godot::PackedInt64Array sparse_indices; 

    // Emulates the 'data.bitset' pointer for MODE_DENSE (64-bit words)
    godot::PackedInt64Array dense_bitmask;  

protected:
    static void _bind_methods();

public:
    MemoryBufferSelectionResource() = default;
    ~MemoryBufferSelectionResource() = default;

    void set_mode(int p_mode);
    int get_mode() const;

    void set_element_count(int p_count);
    int get_element_count() const;

    void set_start_index(int p_index);
    int get_start_index() const;

    void set_sparse_indices(const godot::PackedInt64Array& p_indices);
    godot::PackedInt64Array get_sparse_indices() const;

    void set_dense_bitmask(const godot::PackedInt64Array& p_bitmask);
    godot::PackedInt64Array get_dense_bitmask() const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::MemoryBufferSelectionResource::SelectionMode);