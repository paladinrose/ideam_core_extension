#ifndef IDEAM_CORE_MEMORY_STRATEGIES_H
#define IDEAM_CORE_MEMORY_STRATEGIES_H

#include "../memory_common.h"
#include <cstdint>
#include <type_traits>
#include <concepts>

namespace ideam::core {

/**
 * IsMemoryStrategy
 * Concept ensuring a type provides the necessary DOD metadata for View dispatching.
 */
template <typename T>
concept IsMemoryStrategy = requires {
    { T::is_spatial } -> std::convertible_to<bool>;
    { T::dimensions } -> std::convertible_to<size_t>; // Enforces dimensionality trait
};

struct FlatStrategy {
    static constexpr bool is_spatial = false;
    static constexpr size_t dimensions = 1;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) noexcept {
        return p_base + p_index; 
    }
};

struct SoAStrategy {
    static constexpr bool is_spatial = false;
    static constexpr size_t dimensions = 1;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) noexcept {
        return p_base + p_index; 
    }
};

struct AoSStrategy {
    static constexpr bool is_spatial = false;
    static constexpr size_t dimensions = 1;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) noexcept {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_index * p_stride));
    }
};

struct Spatial2DStrategy {
    static constexpr bool is_spatial = true;
    static constexpr size_t dimensions = 2;
    int64_t stride_y = 0; 

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) noexcept {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_index * p_stride));
    }

    template<typename T>
    inline T* resolve_2d(T* p_base, int64_t p_x, int64_t p_y, size_t p_stride) const noexcept {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_x * static_cast<int64_t>(p_stride)) + (p_y * stride_y));
    }

    inline int64_t get_index_2d(int64_t p_x, int64_t p_y, size_t p_stride) const noexcept {
        return ((p_x * static_cast<int64_t>(p_stride)) + (p_y * stride_y)) / static_cast<int64_t>(p_stride);
    }
};

struct Spatial3DStrategy {
    static constexpr bool is_spatial = true;
    static constexpr size_t dimensions = 3;
    int64_t stride_y = 0;
    int64_t stride_z = 0;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) noexcept {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_index * p_stride));
    }

    template<typename T>
    inline T* resolve_3d(T* p_base, int64_t p_x, int64_t p_y, int64_t p_z, size_t p_stride) const noexcept {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_x * static_cast<int64_t>(p_stride)) + (p_y * stride_y) + (p_z * stride_z));
    }

    inline int64_t get_index_3d(int64_t p_x, int64_t p_y, int64_t p_z, size_t p_stride) const noexcept {
        return ((p_x * static_cast<int64_t>(p_stride)) + (p_y * stride_y) + (p_z * stride_z)) / static_cast<int64_t>(p_stride);
    }
};

struct Spatial4DStrategy {
    static constexpr bool is_spatial = true;
    static constexpr size_t dimensions = 4;
    int64_t stride_y = 0;
    int64_t stride_z = 0;
    int64_t stride_w = 0;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) noexcept {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_index * p_stride));
    }

    template<typename T>
    inline T* resolve_4d(T* p_base, int64_t p_x, int64_t p_y, int64_t p_z, int64_t p_w, size_t p_stride) const noexcept {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_x * static_cast<int64_t>(p_stride)) + (p_y * stride_y) + (p_z * stride_z) + (p_w * stride_w));
    }

    inline int64_t get_index_4d(int64_t p_x, int64_t p_y, int64_t p_z, int64_t p_w, size_t p_stride) const noexcept {
        return ((p_x * static_cast<int64_t>(p_stride)) + (p_y * stride_y) + (p_z * stride_z) + (p_w * stride_w)) / static_cast<int64_t>(p_stride);
    }
};

struct TiledSoAStrategy {
    static constexpr bool is_spatial = false;
    static constexpr size_t dimensions = 1;
    uint32_t elements_per_tile = 0;
    uint32_t tile_stride_bytes = 0;

    template<typename T>
    inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) const noexcept {
        size_t tile_idx = p_index / elements_per_tile;
        size_t local_idx = p_index % elements_per_tile;
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (tile_idx * tile_stride_bytes) + p_stride + (local_idx * sizeof(T)));
    }

    inline int64_t get_index(size_t p_index) const noexcept {
        return static_cast<int64_t>(p_index);
    }
};

struct RingStrategy {
    static constexpr bool is_spatial = false;
    static constexpr size_t dimensions = 1;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes) noexcept {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        size_t offset = (p_index * p_stride) % p_capacity_bytes;
        return reinterpret_cast<T*>(byte_ptr + offset);
    }

    inline int64_t get_wrapped_index(size_t p_logical_index, size_t p_stride, size_t p_capacity_bytes) const noexcept {
        return static_cast<int64_t>(((p_logical_index * p_stride) % p_capacity_bytes) / p_stride);
    }
};

struct PagedStrategy {
    static constexpr bool is_spatial = false;
    static constexpr size_t dimensions = 1;
    uint32_t page_size_bytes = 0;
    uint32_t page_shift = 0; 
    uint32_t page_mask = 0; 

    template<typename T>
    inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) const noexcept {
        uint8_t** table = reinterpret_cast<uint8_t**>(p_base);
        size_t byte_offset = p_index * p_stride;
        size_t page_idx = byte_offset >> page_shift;
        size_t local_offset = byte_offset & page_mask;
        return reinterpret_cast<T*>(table[page_idx] + local_offset);
    }

    inline int64_t get_index(size_t p_index) const noexcept {
        return static_cast<int64_t>(p_index);
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_STRATEGIES_H