#include "field_buffer.h"
#include "../../memory/memory_common.h"
#include <algorithm>

namespace ideam::core {

FieldBuffer::FieldBuffer(uint32_t p_buffer_id, BufferAlignmentMode p_mode)
    : SimulationBuffer<ArrayOfStructuresBuffer>(p_buffer_id, SimulationType::FIELD, p_mode) {
    
    internal_buffer = get_internal();
}

void FieldBuffer::configure(int64_t p_cell_count, const std::vector<DataType>& p_layout) {
    property_registry.clear();
    
    std::vector<size_t> member_sizes;
    member_sizes.reserve(p_layout.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(p_layout.size()); ++i) {
        DataType type = p_layout[i];
        
        // Resolve type size based on the buffer's alignment mode (STD140/STD430/Tight)
        size_t size = MemoryUtilities::get_type_byte_size(type, get_memory_buffer()->get_alignment_mode());
        member_sizes.push_back(size);

        property_registry.push_back({ type, i });
    }

    internal_buffer->configure(p_cell_count, member_sizes);
}

int64_t FieldBuffer::get_cell_count() const {
    return internal_buffer->get_count();
}

size_t FieldBuffer::get_cell_stride() const {
    return internal_buffer->get_stride();
}

uint8_t* FieldBuffer::get_cell_ptr(int64_t p_cell_index) const {
    uint8_t* base = internal_buffer->get_raw_ptr();
    if (!base || p_cell_index < 0 || p_cell_index >= internal_buffer->get_count()) {
        return nullptr;
    }
    return base + (p_cell_index * internal_buffer->get_stride());
}

const ArrayOfStructuresBuffer::MemberInfo* FieldBuffer::get_member_info(uint32_t p_property_id) const {
    return internal_buffer->get_member_info(p_property_id);
}

void FieldBuffer::clear() {
    internal_buffer->clear();
}

} // namespace ideam::core