#ifndef IDEAM_CORE_MEMORY_SPATIAL_ACCESSOR_H
#define IDEAM_CORE_MEMORY_SPATIAL_ACCESSOR_H

#include "array_of_structures_accessor.h"
#include <array>

namespace ideam::core {

/**
 * SpatialAccessor<T, N>
 * Combines AoS memory mapping with N-dimensional grid navigation.
 * N represents the number of dimensions (e.g., 2 for 2D, 3 for 3D).
 */
template<typename T, size_t N>
class SpatialAccessor : public ArrayOfStructuresAccessor<T> {
protected:
    std::array<int64_t, N> dimensions{};
    std::array<int64_t, N> coord_cells{};        // Logical row jumps
    std::array<int64_t, N> coord_byte_deltas{};   // Physical memory jumps

    /**
     * _update_spatial_metadata
     * Bakes the navigation constants based on current dimensions and buffer stride.
     */
    void _update_spatial_metadata() {
        if (this->element_stride == 0) return;

        // 1. Calculate Logical Jumps (Row-count jumps per dimension)
        coord_cells[N - 1] = 1;
        for (int i = static_cast<int>(N) - 2; i >= 0; --i) {
            coord_cells[i] = coord_cells[i + 1] * dimensions[i + 1];
        }

        // 2. Calculate Byte Deltas (Physical memory jumps per dimension)
        for (size_t i = 0; i < N; ++i) {
            coord_byte_deltas[i] = coord_cells[i] * this->element_stride;
        }
    }

public:
    SpatialAccessor(uint32_t p_member_id) 
        : ArrayOfStructuresAccessor<T>(p_member_id) {}

    virtual ~SpatialAccessor() override = default;

    /**
     * set_dimensions
     * Updates the grid topology and refreshes the internal delta bakes.
     */
    void set_dimensions(const std::array<int64_t, N>& p_dims) {
        dimensions = p_dims;
        _update_spatial_metadata();
    }

    /**
     * resolve_all
     * Inherited override. Ensures metadata is refreshed if the buffer reallocates.
     */
    virtual void resolve_all(uint8_t* p_buffer_base, const MemoryBufferSelection* p_selection) override {
        ArrayOfStructuresAccessor<T>::resolve_all(p_buffer_base, p_selection);
        _update_spatial_metadata();
    }

    // --- Spatial API ---

    /**
     * at
     * Direct multidimensional access: acc.at({x, y, z})
     */
    [[nodiscard]] inline T& at(const std::array<int64_t, N>& p_coords) {
        int64_t linear_idx = 0;
        for (size_t i = 0; i < N; ++i) {
            linear_idx += p_coords[i] * coord_cells[i];
        }
        return this->operator[](linear_idx);
    }

    [[nodiscard]] inline const T& at(const std::array<int64_t, N>& p_coords) const {
        int64_t linear_idx = 0;
        for (size_t i = 0; i < N; ++i) {
            linear_idx += p_coords[i] * coord_cells[i];
        }
        return this->operator[](linear_idx);
    }

    /**
     * shift_ptr
     * Moves a pointer through the grid using pre-calculated byte deltas.
     * Extremely useful for kernel-style neighbor plucking.
     */
    [[nodiscard]] inline T* shift_ptr(T* p_current, size_t p_dim, int64_t p_steps) const {
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(p_current);
        return reinterpret_cast<T*>(byte_ptr + (p_steps * coord_byte_deltas[p_dim]));
    }

    /**
     * is_move_valid
     * Boundary validation for spatial movement to prevent linear wrap-around errors.
     */
    [[nodiscard]] inline bool is_move_valid(int64_t p_linear_idx, size_t p_dim, int64_t p_steps) const noexcept {
        const int64_t dim_size = dimensions[p_dim];
        const int64_t current_coord = (p_linear_idx / coord_cells[p_dim]) % dim_size;
        const int64_t target_coord = current_coord + p_steps;
        return target_coord >= 0 && target_coord < dim_size;
    }

    // --- Metadata Accessors ---

    [[nodiscard]] inline const std::array<int64_t, N>& get_dimensions() const { return dimensions; }
    [[nodiscard]] inline const std::array<int64_t, N>& get_coord_cells() const { return coord_cells; }
    [[nodiscard]] inline const std::array<int64_t, N>& get_byte_deltas() const { return coord_byte_deltas; }
    [[nodiscard]] static constexpr size_t get_rank() { return N; }
};

} // namespace ideam::core

#endif // IDEAM_CORE_MEMORY_SPATIAL_ACCESSOR_H