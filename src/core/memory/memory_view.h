#ifndef IDEAM_CORE_MEMORY_VIEW_H
#define IDEAM_CORE_MEMORY_VIEW_H

#include "memory_common.h"
#include <type_traits>
#include <cstdint>

namespace ideam::core {

/**
 * Strategy Policies
 * Define the "How" of memory resolution. 
 */

// --- 1. Flat Strategy ---
struct FlatStrategy {
    static constexpr bool is_spatial = false;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) {
        return p_base + p_index; 
    }
};

// --- 2. SoA Strategy ---
struct SoAStrategy {
    static constexpr bool is_spatial = false;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) {
        return p_base + p_index; 
    }
};

// --- 3. AoS Strategy ---
struct AoSStrategy {
    static constexpr bool is_spatial = false;

    template<typename T>
    static inline T* resolve(T* p_base, size_t p_index, size_t p_stride, size_t p_capacity_bytes = 0) {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_base);
        return reinterpret_cast<T*>(byte_ptr + (p_index * p_stride));
    }
};

// --- 4. Spatial 2D Strategy ---
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
};

// --- 5. Spatial 3D Strategy ---
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
};

// --- 6. Spatial 4D Strategy ---
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
        return reinterpret_cast<T*>(byte_ptr + (tile_idx * tile_stride_bytes) + p_stride + (local_idx * sizeof(T)));
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
};

/**
 * MemoryView<T, Strategy>
 */
template<typename T, typename Strategy = FlatStrategy>
struct MemoryView {
    // --- 8-Byte Block ---
    T* head_ptr = nullptr;
    const MemoryGrantPOD* grant = nullptr;

    // --- 4-Byte Block ---
    uint32_t grant_part_index = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // --- Strategy Policy ---
    // [[no_unique_address]] ensures that if Strategy is empty (Flat/SoA/AoS/Ring),
    // it consumes 0 bytes of extra space.
    [[no_unique_address]] Strategy strategy;

    [[nodiscard]] inline bool is_valid() const {
        if (!grant || !grant->active) return false;
        const auto& part = grant->parts[grant_part_index];
        if (part.buffer_version_at_issue != baked_buffer_version) return false;
        if (*grant->global_manager_version_ptr != baked_manager_version) return false;
        return true;
    }

    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T& operator[](size_t p_index) const {
        const auto& part = grant->parts[grant_part_index];
        if constexpr (std::is_empty_v<Strategy>) {
            return *Strategy::template resolve<T>(head_ptr, p_index, part.element_stride, part.capacity_bytes);
        } else {
            return *strategy.template resolve<T>(head_ptr, p_index, part.element_stride, part.capacity_bytes);
        }
    }

    // --- Spatial Accessors ---

    template<typename S = Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline typename std::enable_if<S::is_spatial, T&>::type 
    at(int64_t p_x, int64_t p_y) const {
        return *strategy.resolve_2d(head_ptr, p_x, p_y, grant->parts[grant_part_index].element_stride);
    }

    template<typename S = Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline typename std::enable_if<S::is_spatial, T&>::type 
    at(int64_t p_x, int64_t p_y, int64_t p_z) const {
        return *strategy.resolve_3d(head_ptr, p_x, p_y, p_z, grant->parts[grant_part_index].element_stride);
    }

    template<typename S = Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline typename std::enable_if<S::is_spatial, T&>::type 
    at(int64_t p_x, int64_t p_y, int64_t p_z, int64_t p_w) const {
        return *strategy.resolve_4d(head_ptr, p_x, p_y, p_z, p_w, grant->parts[grant_part_index].element_stride);
    }

    // --- Utility ---

    [[nodiscard]] inline T* get_raw(size_t p_index) const {
        const auto& part = grant->parts[grant_part_index];
        if constexpr (std::is_empty_v<Strategy>) {
            return Strategy::template resolve<T>(head_ptr, p_index, part.element_stride, part.capacity_bytes);
        } else {
            return strategy.template resolve<T>(head_ptr, p_index, part.element_stride, part.capacity_bytes);
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_VIEW_H