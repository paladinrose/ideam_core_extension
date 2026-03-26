#include "array_of_structures_buffer.h"
#include <algorithm>

namespace ideam::core {

ArrayOfStructuresBuffer::ArrayOfStructuresBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode, size_t p_alignment)
    : MemoryBuffer(p_buffer_id, p_mode) {
    alignment_requirement = p_alignment;
}

void ArrayOfStructuresBuffer::configure(int64_t p_element_count, const std::vector<size_t>& p_member_sizes) {
    element_count = p_element_count;
    _calculate_layout(p_member_sizes);
}

void ArrayOfStructuresBuffer::_calculate_layout(const std::vector<size_t>& p_member_sizes) {
    members.clear();
    size_t current_offset = 0;
    size_t max_align = 1;

    for (uint32_t i = 0; i < p_member_sizes.size(); ++i) {
        size_t size = p_member_sizes[i];
        
        // Simple alignment: align to the size of the power-of-two (up to 16/32/64)
        // Or follow your specific BufferAlignmentMode logic here.
        size_t align_req = std::min(size, (size_t)16); 
        max_align = std::max(max_align, align_req);

        size_t remainder = current_offset % align_req;
        if (remainder != 0) current_offset += (align_req - remainder);

        members.push_back({i, current_offset, size});
        current_offset += size;
    }

    // Pad the total stride to the largest member alignment or global alignment
    size_t final_align = std::max(max_align, alignment_requirement);
    size_t stride_remainder = current_offset % final_align;
    if (stride_remainder != 0) current_offset += (final_align - stride_remainder);

    element_stride = current_offset;
    used_bytes = element_count * element_stride;
    bump_version();
}

const ArrayOfStructuresBuffer::MemberInfo* ArrayOfStructuresBuffer::get_member_info(uint32_t p_member_id) const {
    if (p_member_id >= members.size()) return nullptr;
    return &members[p_member_id];
}

} // namespace ideam::core