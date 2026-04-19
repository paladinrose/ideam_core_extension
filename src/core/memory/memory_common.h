#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace ideam::core {

static constexpr uint32_t INVALID_ID = 0xFFFFFFFF;

/**
 * DataType
 * Standardized COMMON for the simulation backend.
 * Underlying int64_t for seamless casting to Godot's Variant if needed.
 * Includes CUSTOM hook for plugin-specific or composite structures.
 */
enum class DataType : uint64_t {
    NONE       = 0,
    
    // --- Concrete Primitives (The "Leaf" Bits) ---
    BOOL       = 1ULL << 0, 
    BYTE       = 1ULL << 1, 
    INT32      = 1ULL << 2, 
    INT64      = 1ULL << 3, 
    FLOAT32    = 1ULL << 4, 
    FLOAT64    = 1ULL << 5,

    VECTOR2    = 1ULL << 6,  // Float32
    VECTOR3    = 1ULL << 7, 
    VECTOR4    = 1ULL << 8,

    VECTOR2I   = 1ULL << 9,  // Int32
    VECTOR3I   = 1ULL << 10, 
    VECTOR4I   = 1ULL << 11,

    VECTOR2D   = 1ULL << 12, // Float64
    VECTOR3D   = 1ULL << 13, 
    VECTOR4D   = 1ULL << 14,

    COLOR      = 1ULL << 15,
    CUSTOM     = 1ULL << 63,

    // --- Dimension Masks (Logic-Centric) ---
    ANY_VECTOR2 = VECTOR2 | VECTOR2I | VECTOR2D,
    ANY_VECTOR3 = VECTOR3 | VECTOR3I | VECTOR3D,
    ANY_VECTOR4 = VECTOR4 | VECTOR4I | VECTOR4D | COLOR,

    // --- Precision Masks (Hardware/SIMD-Centric) ---
    ANY_32BIT_FLOAT = FLOAT32 | VECTOR2  | VECTOR3  | VECTOR4,
    ANY_64BIT_FLOAT = FLOAT64 | VECTOR2D | VECTOR3D | VECTOR4D,
    ANY_32BIT_INT   = INT32   | VECTOR2I | VECTOR3I | VECTOR4I,
    
    // --- Godot-Specific Semantic Masks ---
    // Useful for mapping Godot's 'Variant::Type' to our DOD world
    GODOT_FLOAT_TYPES  = ANY_64BIT_FLOAT, // Godot 4 defaults to doubles
    GODOT_INT_TYPES    = INT64, 
    GODOT_VECTOR_TYPES = ANY_VECTOR2 | ANY_VECTOR3 | ANY_VECTOR4,

    // --- Final Utility Aggregates ---
    ANY_NUMERIC = ANY_32BIT_FLOAT | ANY_64BIT_FLOAT | ANY_32BIT_INT | INT64 | BYTE,
    ANY         = (1ULL << 16) - 1 
};

constexpr DataType operator|(DataType a, DataType b) noexcept {
    return static_cast<DataType>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

constexpr DataType operator&(DataType a, DataType b) noexcept {
    return static_cast<DataType>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

/**
 * BufferAccessMode
 * Defines the read/write permissions for a MemoryGrant.
 * Used by the MemoryManager to detect and prevent data races.
 */
enum class BufferAccessMode : uint8_t {
    NONE = 0,       // No access permitted.
    READ = 1,       // Shared access; multiple readers allowed.
    WRITE = 2,      // Exclusive access; no other readers or writers allowed.
    READ_WRITE = 3  // Exclusive access with intent to both read and modify.
};

/**
 * BufferAlignmentMode
 * Defines memory packing rules for SIMD and GPU compatibility.
 */
enum class BufferAlignmentMode : uint8_t {
    TIGHT,   // 1-byte alignment (AoS optimization)
    STD430,  // GPU-friendly (4/8/16 byte rules, no 16-byte rounding for arrays)
    STD140   // Strict GPU (16-byte chunks, arrays/structs rounded to 16)
};

/**
 * BufferLifecycleState
 * Tracks the structural stability of a buffer.
 */
enum class BufferLifecycleState : uint8_t {
    IDLE,
    LOCKED,    // Structural changes (re-sizing) forbidden
    MIGRATING  // Buffer is being moved in the master block
};

/**
 * MemoryUtilities
 * Compile-time and inline utilities for memory math.
 */
struct MemoryUtilities {
    
    /**
     * get_type_byte_size
     * Returns the byte footprint of a type based on the alignment mode.
     * @param p_type The DataType enum.
     * @param p_mode The alignment mode (TIGHT, STD430, STD140).
     * @param p_custom_size Optional size override for DataType::CUSTOM.
     */
    [[nodiscard]] static constexpr int get_type_byte_size(DataType p_type, BufferAlignmentMode p_mode, int p_custom_size = 0) {
        if (p_type == DataType::CUSTOM) {
            return p_custom_size;
        }

        switch (p_type) {
            case DataType::BOOL:     return (p_mode == BufferAlignmentMode::TIGHT) ? 1 : 4;
            case DataType::BYTE:     return 1;
            case DataType::INT32: 
            case DataType::FLOAT32:  return 4;
            case DataType::INT64: 
            case DataType::FLOAT64:  return 8;
            case DataType::VECTOR2I:
            case DataType::VECTOR2:  return 8;
            case DataType::VECTOR3I: 
            case DataType::VECTOR3:  return (p_mode == BufferAlignmentMode::STD140) ? 16 : 12; 
            case DataType::VECTOR4: 
            case DataType::VECTOR4I: 
            case DataType::COLOR: 
            case DataType::VECTOR2D: return 16;
            case DataType::VECTOR3D: return 24;
            case DataType::VECTOR4D: return 32;
            default: return 0;
        }
    }

    /**
     * get_type_alignment
     * Returns the alignment requirement (in bytes) for a type.
     */
    [[nodiscard]] static constexpr int get_type_alignment(DataType p_type, BufferAlignmentMode p_mode, int p_custom_align = 0) {
        if (p_mode == BufferAlignmentMode::TIGHT) {
            return 1;
        }
        
        if (p_type == DataType::CUSTOM) {
            return p_custom_align > 0 ? p_custom_align : 4; // Default to 4-byte for custom if not specified
        }

        switch (p_type) {
            case DataType::BYTE:     return 1;
            case DataType::BOOL: 
            case DataType::INT32: 
            case DataType::FLOAT32:  return 4;
            case DataType::INT64: 
            case DataType::FLOAT64: 
            case DataType::VECTOR2: 
            case DataType::VECTOR2I: return 8; 
            case DataType::VECTOR2D: return 16;
            case DataType::VECTOR3: 
            case DataType::VECTOR3I:
            case DataType::VECTOR4: 
            case DataType::VECTOR4I: 
            case DataType::COLOR:    return 16;
            case DataType::VECTOR3D: 
            case DataType::VECTOR4D: return 32;
            default: return 4;
        }
    }

    /**
     * align_to
     * Rounds an offset up to the nearest alignment boundary.
     */
    [[nodiscard]] static constexpr size_t align_to(size_t p_offset, size_t p_alignment) {
        if (p_alignment == 0) return p_offset;
        return (p_offset + p_alignment - 1) & ~(p_alignment - 1);
    }

    /**
     * is_trivially_copyable_v
     * Validation helper for CUSTOM types to ensure DOD safety.
     */
    template<typename T>
    static constexpr bool is_dod_safe() {
        return std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;
    }
};

} // namespace ideam::core

 // IDEAM_CORE_MEMORY_COMMON_H