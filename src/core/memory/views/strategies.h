#ifndef IDEAM_CORE_MEMORY_STRATEGIES_H
#define IDEAM_CORE_MEMORY_STRATEGIES_H

#include "../memory_common.h"
#include <cstdint>
#include <type_traits>

namespace ideam::core {

struct FlatStrategy {
    static constexpr bool is_spatial = false;
    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) {
        return p_base + p_index; 
    }
};

struct SoAStrategy {
    static constexpr bool is_spatial = false;
    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) {
        return p_base + p_index; 
    }
};

struct AoSStrategy {
    static constexpr bool is_spatial = false;
    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_index * p_stride));
    }
};

struct Spatial2DStrategy {
    static constexpr bool is_spatial = true;
    int64_t stride_y = 0; 

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_index * p_stride));
    }

    template<typename T>
    inline T* resolve_2d(T* p_base, int64_t p_x, int64_t p_y, size_t p_stride) const {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_x * p_stride) + (p_y * stride_y));
    }

    inline int64_t get_index_2d(int64_t p_x, int64_t p_y, size_t p_stride) const {
        return ((p_x * static_cast<int64_t>(p_stride)) + (p_y * stride_y)) / static_cast<int64_t>(p_stride);
    }
};

struct Spatial3DStrategy {
    static constexpr bool is_spatial = true;
    int64_t stride_y = 0;
    int64_t stride_z = 0;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_index * p_stride));
    }

    template<typename T>
    inline T* resolve_3d(T* p_base, int64_t p_x, int64_t p_y, int64_t p_z, size_t p_stride) const {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_x * p_stride) + (p_y * stride_y) + (p_z * stride_z));
    }

    inline int64_t get_index_3d(int64_t p_x, int64_t p_y, int64_t p_z, size_t p_stride) const {
        return ((p_x * static_cast<int64_t>(p_stride)) + (p_y * stride_y) + (p_z * stride_z)) / static_cast<int64_t>(p_stride);
    }
};

struct Spatial4DStrategy {
    static constexpr bool is_spatial = true;
    int64_t stride_y = 0;
    int64_t stride_z = 0;
    int64_t stride_w = 0;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_index * p_stride));
    }

    template<typename T>
    inline T* resolve_4d(T* p_base, int64_t p_x, int64_t p_y, int64_t p_z, int64_t p_w, size_t p_stride) const {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_x * p_stride) + (p_y * stride_y) + (p_z * stride_z) + (p_w * stride_w));
    }

    inline int64_t get_index_4d(int64_t p_x, int64_t p_y, int64_t p_z, int64_t p_w, size_t p_stride) const {
        return ((p_x * static_cast<int64_t>(p_stride)) + (p_y * stride_y) + (p_z * stride_z) + (p_w * stride_w)) / static_cast<int64_t>(p_stride);
    }
};

// --- 7. Tiled SoA Strategy ---
struct TiledSoAStrategy {
    static constexpr bool is_spatial = false;
    uint32_t elements_per_tile = 0;
    uint32_t tile_stride_bytes = 0;

    template<typename T>
    inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) const {
        size_t tile_idx = p_index / elements_per_tile;
        size_t local_idx = p_index % elements_per_tile;
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        // p_stride here represents the column offset within the tile
        return reinterpret_cast<T*>(byte_ptr + (tile_idx * tile_stride_bytes) + p_stride + (local_idx * sizeof(T)));
    }

    // Logic: Tiled layouts are still linear in their index space, 
    // but this method allows external systems to confirm the mapping.
    inline int64_t get_index(size_t p_index) const {
        return static_cast<int64_t>(p_index);
    }
};

// --- 8. Ring Strategy ---
struct RingStrategy {
    static constexpr bool is_spatial = false;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes) {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        size_t offset = (p_index * p_stride) % p_capacity_bytes;
        return reinterpret_cast<T*>(byte_ptr + offset);
    }

    // Critical: For Ring buffers, the logical index is often monotonic, 
    // but the selection needs the wrapped buffer index.
    inline int64_t get_wrapped_index(size_t p_logical_index, size_t p_stride, size_t p_capacity_bytes) const {
        return static_cast<int64_t>((p_logical_index * p_stride) % p_capacity_bytes / p_stride);
    }
};

// --- 9. Paged Strategy ---
struct PagedStrategy {
    static constexpr bool is_spatial = false;
    uint32_t page_size_bytes = 0;
    uint32_t page_shift = 0; 
    uint32_t page_mask = 0; 

    template<typename T>
    inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) const {
        uint8_t** table = reinterpret_cast<uint8_t**>(p_base);
        size_t byte_offset = p_index * p_stride;
        size_t page_idx = byte_offset >> page_shift;
        size_t local_offset = byte_offset & page_mask;
        return reinterpret_cast<T*>(table[page_idx] + local_offset);
    }

    // Allows the view to verify if a specific logical index is currently paged-in
    inline int64_t get_index(size_t p_index) const {
        return static_cast<int64_t>(p_index);
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_STRATEGIES_H