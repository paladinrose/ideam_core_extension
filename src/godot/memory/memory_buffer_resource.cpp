#include "memory_buffer_resource.h"

namespace ideam::godot_ext {

void MemoryBufferResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(SELECTION_DENSE);
    BIND_ENUM_CONSTANT(SELECTION_SPARSE);
    
    ClassDB::bind_method(D_METHOD("set_buffer_name", "name"), &MemoryBufferResource::set_buffer_name);
    ClassDB::bind_method(D_METHOD("get_buffer_name"), &MemoryBufferResource::get_buffer_name);
    
    ClassDB::bind_method(D_METHOD("set_layout_type", "type"), &MemoryBufferResource::set_layout_type);
    ClassDB::bind_method(D_METHOD("get_layout_type"), &MemoryBufferResource::get_layout_type);
    
    ClassDB::bind_method(D_METHOD("set_max_elements", "elements"), &MemoryBufferResource::set_max_elements);
    ClassDB::bind_method(D_METHOD("get_max_elements"), &MemoryBufferResource::get_max_elements);

    ClassDB::bind_method(D_METHOD("set_alignment_mode", "mode"), &MemoryBufferResource::set_alignment_mode);
    ClassDB::bind_method(D_METHOD("get_alignment_mode"), &MemoryBufferResource::get_alignment_mode);

    ClassDB::bind_method(D_METHOD("set_alignment", "alignment"), &MemoryBufferResource::set_alignment);
    ClassDB::bind_method(D_METHOD("get_alignment"), &MemoryBufferResource::get_alignment);

    ClassDB::bind_method(D_METHOD("set_needs_gpu_compute", "needs_gpu"), &MemoryBufferResource::set_needs_gpu_compute);
    ClassDB::bind_method(D_METHOD("get_needs_gpu_compute"), &MemoryBufferResource::get_needs_gpu_compute);

    // Layout-Specific Parameters
    ClassDB::bind_method(D_METHOD("set_elements_per_tile", "count"), &MemoryBufferResource::set_elements_per_tile);
    ClassDB::bind_method(D_METHOD("get_elements_per_tile"), &MemoryBufferResource::get_elements_per_tile);

    ClassDB::bind_method(D_METHOD("set_page_size_bytes", "bytes"), &MemoryBufferResource::set_page_size_bytes);
    ClassDB::bind_method(D_METHOD("get_page_size_bytes"), &MemoryBufferResource::get_page_size_bytes);

    ClassDB::bind_method(D_METHOD("set_sparse_universe_size", "size"), &MemoryBufferResource::set_sparse_universe_size);
    ClassDB::bind_method(D_METHOD("get_sparse_universe_size"), &MemoryBufferResource::get_sparse_universe_size);

    ClassDB::bind_method(D_METHOD("get_element_stride"), &MemoryBufferResource::get_element_stride);
    ClassDB::bind_method(D_METHOD("get_tile_count"), &MemoryBufferResource::get_tile_count);
    ClassDB::bind_method(D_METHOD("get_page_count"), &MemoryBufferResource::get_page_count);

    ClassDB::bind_method(D_METHOD("set_enable_shadowing", "enable"), &MemoryBufferResource::set_enable_shadowing);
    ClassDB::bind_method(D_METHOD("get_enable_shadowing"), &MemoryBufferResource::get_enable_shadowing);

    ClassDB::bind_method(D_METHOD("set_selection_mode", "mode"), &MemoryBufferResource::set_selection_mode);
    ClassDB::bind_method(D_METHOD("get_selection_mode"), &MemoryBufferResource::get_selection_mode);

    // Columns & Modification
    ClassDB::bind_method(D_METHOD("set_columns", "columns"), &MemoryBufferResource::set_columns);
    ClassDB::bind_method(D_METHOD("get_columns"), &MemoryBufferResource::get_columns);
    ClassDB::bind_method(D_METHOD("add_column", "name", "data_type", "custom_size"), &MemoryBufferResource::add_column, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("remove_column", "index"), &MemoryBufferResource::remove_column);
    ClassDB::bind_method(D_METHOD("clear_columns"), &MemoryBufferResource::clear_columns);

    // Sizing & Computations
    ClassDB::bind_method(D_METHOD("calculate_raw_data_bytes"), &MemoryBufferResource::calculate_raw_data_bytes);
    ClassDB::bind_method(D_METHOD("calculate_unpadded_data_bytes"), &MemoryBufferResource::calculate_unpadded_data_bytes);
    ClassDB::bind_method(D_METHOD("calculate_padding_bytes"), &MemoryBufferResource::calculate_padding_bytes);
    ClassDB::bind_method(D_METHOD("calculate_shadow_bytes"), &MemoryBufferResource::calculate_shadow_bytes);
    ClassDB::bind_method(D_METHOD("calculate_projected_footprint_bytes"), &MemoryBufferResource::calculate_projected_footprint_bytes);
    
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "buffer_name"), "set_buffer_name", "get_buffer_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "layout_type", PROPERTY_HINT_ENUM, "Flat:1,AOS:2,SOA:4,Sparse Set:8,Tiled SOA:16,Ring:32,Paged:64"), "set_layout_type", "get_layout_type");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alignment_mode", PROPERTY_HINT_ENUM, "Tight:0,STD430:1,STD140:2"), "set_alignment_mode", "get_alignment_mode");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alignment"), "set_alignment", "get_alignment");
    
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_elements"), "set_max_elements", "get_max_elements");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "needs_gpu_compute"), "set_needs_gpu_compute", "get_needs_gpu_compute");
    
    ADD_GROUP("Layout Specific", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "elements_per_tile"), "set_elements_per_tile", "get_elements_per_tile");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "page_size_bytes"), "set_page_size_bytes", "get_page_size_bytes");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "sparse_universe_size"), "set_sparse_universe_size", "get_sparse_universe_size");

    ADD_GROUP("Shadow Configuration", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_shadowing"), "set_enable_shadowing", "get_enable_shadowing");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "selection_mode", PROPERTY_HINT_ENUM, "Dense:0,Sparse:1"), "set_selection_mode", "get_selection_mode");
    
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "columns"), "set_columns", "get_columns");
}

void MemoryBufferResource::set_buffer_name(const godot::StringName& p_name) { 
    if (buffer_name == p_name) return;
    buffer_name = p_name; 
    emit_changed();
}
godot::StringName MemoryBufferResource::get_buffer_name() const { return buffer_name; }

void MemoryBufferResource::set_layout_type(int p_type) {
    BufferLayoutType new_layout = static_cast<BufferLayoutType>(p_type);
    if (new_layout == layout_type) return; 
    layout_type = new_layout; 
    emit_changed();
}
int MemoryBufferResource::get_layout_type() const { return static_cast<int>(layout_type); }

void MemoryBufferResource::set_max_elements(int p_elements) {
    if (p_elements == max_elements) return; 
    max_elements = p_elements; 
    emit_changed();
}
int MemoryBufferResource::get_max_elements() const { return max_elements; }

void MemoryBufferResource::set_elements_per_tile(int p_count) {
    uint32_t val = static_cast<uint32_t>(godot::Math::max(1, p_count));
    if (elements_per_tile == val) return;
    elements_per_tile = val;
    emit_changed();
}
int MemoryBufferResource::get_elements_per_tile() const { return static_cast<int>(elements_per_tile); }

void MemoryBufferResource::set_page_size_bytes(int p_bytes) {
    uint32_t val = static_cast<uint32_t>(godot::Math::max(64, p_bytes));
    if (page_size_bytes == val) return;
    page_size_bytes = val;
    emit_changed();
}
int MemoryBufferResource::get_page_size_bytes() const { return static_cast<int>(page_size_bytes); }

void MemoryBufferResource::set_sparse_universe_size(int64_t p_size) {
    if (sparse_universe_size == p_size || p_size <= 0) return;
    sparse_universe_size = p_size;
    emit_changed();
}
int64_t MemoryBufferResource::get_sparse_universe_size() const { return sparse_universe_size; }

int MemoryBufferResource::get_element_stride() const {
    if (layout_type == BufferLayoutType::AOS) {
        size_t current_offset = 0;
        uint32_t max_align = 1;
        for (int i = 0; i < columns.size(); ++i) {
            godot::Dictionary col = columns[i];
            uint32_t col_size = col.has("size") ? static_cast<uint32_t>(col["size"]) : 4;
            uint32_t col_align = col.has("alignment") ? static_cast<uint32_t>(col["alignment"]) : col_size;
            max_align = godot::Math::max(max_align, col_align);
            current_offset = ideam::core::MemoryUtilities::align_to(current_offset, col_align);
            current_offset += col_size;
        }
        return static_cast<int>(ideam::core::MemoryUtilities::align_to(current_offset, max_align));
    }
    
    int stride = 0;
    for (int i = 0; i < columns.size(); ++i) {
        godot::Dictionary col = columns[i];
        stride += col.has("size") ? static_cast<int>(col["size"]) : 4;
    }
    return stride;
}

int MemoryBufferResource::get_tile_count() const {
    if (layout_type != BufferLayoutType::TILED_SOA || elements_per_tile == 0) return 0;
    return static_cast<int>((max_elements + elements_per_tile - 1) / elements_per_tile);
}

int MemoryBufferResource::get_page_count() const {
    if (page_size_bytes == 0) return 0;
    int raw_bytes = calculate_raw_data_bytes();
    return static_cast<int>((raw_bytes + page_size_bytes - 1) / page_size_bytes);
}

void MemoryBufferResource::set_alignment_mode(int p_mode) {
    BufferAlignmentMode mode = static_cast<BufferAlignmentMode>(p_mode);
    if (alignment_mode == mode) return;
    alignment_mode = mode;
    // Re-evaluate footprint, alignment, and size attributes across all columns
    set_columns(columns);
}
int MemoryBufferResource::get_alignment_mode() const { return static_cast<int>(alignment_mode); }

void MemoryBufferResource::set_alignment(int p_align) {
    if (p_align == alignment) return; 
    alignment = p_align;
    emit_changed();
}
int MemoryBufferResource::get_alignment() const { return alignment; }

void MemoryBufferResource::set_needs_gpu_compute(bool p_needs_gpu) {
    if (p_needs_gpu == needs_gpu_compute) return; 
    needs_gpu_compute = p_needs_gpu;
    emit_changed();
}
bool MemoryBufferResource::get_needs_gpu_compute() const { return needs_gpu_compute; }

void MemoryBufferResource::set_enable_shadowing(bool p_shadow) {
    if (p_shadow == enable_shadowing) return; 
    enable_shadowing = p_shadow;
    emit_changed();
}
bool MemoryBufferResource::get_enable_shadowing() const { return enable_shadowing; }

void MemoryBufferResource::set_selection_mode(int p_mode) { 
    if (p_mode == selection_mode) return;
    selection_mode = static_cast<SelectionMode>(p_mode);
    emit_changed();
}
int MemoryBufferResource::get_selection_mode() const { return selection_mode; }

void MemoryBufferResource::set_columns(const godot::TypedArray<godot::Dictionary>& p_columns) {
    godot::TypedArray<godot::Dictionary> clamped_cols;
    int limit = godot::Math::min(static_cast<int>(p_columns.size()), static_cast<int>(ideam::core::MAX_BUFFER_COLUMNS));
    
    for (int i = 0; i < limit; ++i) {
        godot::Dictionary col = p_columns[i].duplicate();
        col["id"] = i;
        
        DataType dt = col.has("data_type") ? static_cast<DataType>(static_cast<int64_t>(col["data_type"])) : DataType::FLOAT32;
        int custom_size = col.has("custom_size") ? static_cast<int>(col["custom_size"]) : 0;
        
        // Recalculate size and alignment based on the active alignment mode
        col["size"] = ideam::core::MemoryUtilities::get_type_byte_size(dt, alignment_mode, custom_size);
        col["alignment"] = ideam::core::MemoryUtilities::get_type_alignment(dt, alignment_mode, custom_size);
        clamped_cols.push_back(col);
    }

    columns = clamped_cols;
    emit_changed();
}
godot::TypedArray<godot::Dictionary> MemoryBufferResource::get_columns() const { return columns; }

void MemoryBufferResource::add_column(const godot::StringName& p_name, int64_t p_data_type, int p_custom_size) {
    if (columns.size() >= static_cast<int>(ideam::core::MAX_BUFFER_COLUMNS)) {
        godot::UtilityFunctions::push_warning("MemoryBufferResource: Exceeded MAX_BUFFER_COLUMNS (16).");
        return;
    }

    DataType dt = static_cast<DataType>(p_data_type);
    uint64_t dt_val = static_cast<uint64_t>(dt);
    
    // Enforce single leaf DataType
    if (dt_val == 0 || (dt_val & (dt_val - 1)) != 0) {
        dt = DataType::FLOAT32;
    }

    godot::Dictionary col;
    col["id"] = columns.size();
    col["name"] = p_name;
    col["data_type"] = static_cast<int64_t>(dt);
    col["custom_size"] = p_custom_size;
    col["size"] = ideam::core::MemoryUtilities::get_type_byte_size(dt, alignment_mode, p_custom_size);
    col["alignment"] = ideam::core::MemoryUtilities::get_type_alignment(dt, alignment_mode, p_custom_size);

    columns.push_back(col);
    emit_changed();
}

void MemoryBufferResource::remove_column(int p_index) {
    if (p_index < 0 || p_index >= columns.size()) return;
    columns.remove_at(p_index);
    
    // Re-index remaining columns
    for (int i = 0; i < columns.size(); ++i) {
        godot::Dictionary col = columns[i];
        col["id"] = i;
    }
    emit_changed();
}

void MemoryBufferResource::clear_columns() {
    if (columns.is_empty()) return;
    columns.clear();
    emit_changed();
}

int MemoryBufferResource::calculate_raw_data_bytes() const {
    if (max_elements <= 0) return 0;

    switch (layout_type) {
        case BufferLayoutType::AOS: {
            size_t current_offset = 0;
            uint32_t max_align = 1;

            for (int i = 0; i < columns.size(); ++i) {
                godot::Dictionary col = columns[i];
                uint32_t col_size = col.has("size") ? static_cast<uint32_t>(col["size"]) : 4;
                uint32_t col_align = col.has("alignment") ? static_cast<uint32_t>(col["alignment"]) : col_size;
                
                max_align = godot::Math::max(max_align, col_align);
                current_offset = ideam::core::MemoryUtilities::align_to(current_offset, col_align);
                current_offset += col_size;
            }

            size_t element_stride = ideam::core::MemoryUtilities::align_to(current_offset, max_align);
            return static_cast<int>(element_stride * max_elements);
        }

        case BufferLayoutType::SOA: {
            size_t current_offset = 0;
            for (int i = 0; i < columns.size(); ++i) {
                godot::Dictionary col = columns[i];
                uint32_t col_size = col.has("size") ? static_cast<uint32_t>(col["size"]) : 4;
                
                current_offset = ideam::core::MemoryUtilities::align_to(current_offset, 64);
                current_offset += (max_elements * col_size);
            }
            return static_cast<int>(current_offset);
        }

        case BufferLayoutType::TILED_SOA: {
            if (elements_per_tile == 0) return 0;

            uint32_t tile_size_bytes = 0;
            for (int i = 0; i < columns.size(); ++i) {
                godot::Dictionary col = columns[i];
                uint32_t col_size = col.has("size") ? static_cast<uint32_t>(col["size"]) : 4;
                uint32_t col_align = col.has("alignment") ? static_cast<uint32_t>(col["alignment"]) : col_size;

                tile_size_bytes = static_cast<uint32_t>(ideam::core::MemoryUtilities::align_to(tile_size_bytes, col_align));
                tile_size_bytes += (elements_per_tile * col_size);
            }

            size_t tile_stride = ideam::core::MemoryUtilities::align_to(tile_size_bytes, 64);
            size_t total_tiles = (max_elements + elements_per_tile - 1) / elements_per_tile;
            return static_cast<int>(total_tiles * tile_stride);
        }

        case BufferLayoutType::SPARSE_SET: {
            // 1. Sparse Index Array (Maps Entity ID -> Dense Index)
            size_t sparse_bytes = ideam::core::MemoryUtilities::align_to(sparse_universe_size * sizeof(uint32_t), 64);
            
            // 2. Dense Packed Arrays
            size_t dense_bytes = 0;
            for (int i = 0; i < columns.size(); ++i) {
                godot::Dictionary col = columns[i];
                uint32_t col_size = col.has("size") ? static_cast<uint32_t>(col["size"]) : 4;
                dense_bytes = ideam::core::MemoryUtilities::align_to(dense_bytes, 64);
                dense_bytes += (max_elements * col_size);
            }
            return static_cast<int>(sparse_bytes + dense_bytes);
        }

        case BufferLayoutType::PAGED: {
            if (page_size_bytes == 0) return 0;

            size_t element_size = 0;
            for (int i = 0; i < columns.size(); ++i) {
                godot::Dictionary col = columns[i];
                element_size += col.has("size") ? static_cast<uint32_t>(col["size"]) : 4;
            }
            if (element_size == 0) element_size = 4;

            size_t raw_total = element_size * max_elements;
            size_t total_pages = (raw_total + page_size_bytes - 1) / page_size_bytes;
            return static_cast<int>(total_pages * page_size_bytes);
        }

        case BufferLayoutType::RING:
        case BufferLayoutType::FLAT:
        default: {
            size_t element_size = 0;
            for (int i = 0; i < columns.size(); ++i) {
                godot::Dictionary col = columns[i];
                element_size += col.has("size") ? static_cast<uint32_t>(col["size"]) : 1;
            }
            if (element_size == 0) element_size = 1;
            return static_cast<int>(element_size * max_elements);
        }
    }
}

int MemoryBufferResource::calculate_unpadded_data_bytes() const {
    if (max_elements <= 0) return 0;

    int unpadded_stride = 0;
    for (int i = 0; i < columns.size(); ++i) {
        godot::Dictionary col = columns[i];
        DataType dt = col.has("data_type") ? static_cast<DataType>(static_cast<int64_t>(col["data_type"])) : DataType::FLOAT32;
        int custom_size = col.has("custom_size") ? static_cast<int>(col["custom_size"]) : 0;
        unpadded_stride += ideam::core::MemoryUtilities::get_type_byte_size(dt, BufferAlignmentMode::TIGHT, custom_size);
    }

    int unpadded_total = unpadded_stride * max_elements;
    if (layout_type == BufferLayoutType::SPARSE_SET && sparse_universe_size > 0) {
        unpadded_total += static_cast<int>(sparse_universe_size * sizeof(uint32_t));
    }
    return unpadded_total;
}

int MemoryBufferResource::calculate_padding_bytes() const {
    if (max_elements <= 0) return 0;
    int unpadded = calculate_unpadded_data_bytes();
    int footprint = calculate_projected_footprint_bytes();
    int shadow = calculate_shadow_bytes();
    int padding = footprint - unpadded - shadow;
    return godot::Math::max(0, padding);
}

int MemoryBufferResource::calculate_shadow_bytes() const {
    if (!enable_shadowing || max_elements <= 0) return 0;

    size_t selection_data_size = (selection_mode == SELECTION_DENSE) ? 
        (((max_elements + 63) / 64) * sizeof(uint64_t)) : (max_elements * sizeof(int64_t));

    size_t meta_soa_size = (max_elements * sizeof(uint32_t)) + 
                           (max_elements * sizeof(int64_t)) + 
                           (max_elements * sizeof(uint8_t));

    return static_cast<int>(selection_data_size + meta_soa_size + 256);
}

int MemoryBufferResource::calculate_projected_footprint_bytes() const {
    if (max_elements <= 0) return 0;

    size_t data_bytes = static_cast<size_t>(calculate_raw_data_bytes());
    size_t block_align = alignment > 0 ? static_cast<size_t>(alignment) : 64;
    size_t aligned_footprint = ideam::core::MemoryUtilities::align_to(data_bytes, block_align);

    if (enable_shadowing) {
        aligned_footprint += static_cast<size_t>(calculate_shadow_bytes());
        aligned_footprint = ideam::core::MemoryUtilities::align_to(aligned_footprint, block_align);
    }

    return static_cast<int>(aligned_footprint);
}

} // namespace ideam::godot_ext