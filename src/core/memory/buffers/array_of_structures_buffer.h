#ifndef IDEAM_CORE_MEMORY_ARRAY_OF_STRUCTURES_BUFFER_H
#define IDEAM_CORE_MEMORY_ARRAY_OF_STRUCTURES_BUFFER_H

#include "../memory_buffer.h"
#include <vector>

namespace ideam::core {

/**
 * ArrayOfStructuresBuffer
 * Manages memory where each element (cell) contains multiple data members.
 */
class ArrayOfStructuresBuffer : public MemoryBuffer {
public:
    struct MemberInfo {
        uint32_t id;
        size_t offset;
        size_t size;
    };

private:
    int64_t element_count = 0;
    size_t element_stride = 0;
    std::vector<MemberInfo> members;

    void _calculate_layout(const std::vector<size_t>& p_member_sizes);

public:
    ArrayOfStructuresBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode, size_t p_alignment = 64);
    virtual ~ArrayOfStructuresBuffer() override = default;

    /**
     * configure
     * p_member_sizes: A list of byte-sizes for each member in the struct.
     * The index in this vector becomes the Member ID.
     */
    void configure(int64_t p_element_count, const std::vector<size_t>& p_member_sizes);

    [[nodiscard]] inline int64_t get_count() const { return element_count; }
    [[nodiscard]] inline size_t get_stride() const { return element_stride; }
    
    [[nodiscard]] const MemberInfo* get_member_info(uint32_t p_member_id) const;

    virtual void clear() override {
        element_count = 0;
        MemoryBuffer::clear();
    }
};

} // namespace ideam::core

#endif