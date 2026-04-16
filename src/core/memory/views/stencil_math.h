namespace ideam::core::stencil_math {
    // Calculates a dense bounding box (Moore neighborhood)
    template <size_t Dimensions, size_t Radius>
    consteval size_t moore_size() {
        size_t size = 1;
        for (size_t i = 0; i < Dimensions; ++i) size *= (2 * Radius + 1);
        return size;
    }

    // Calculates a cross/diamond (Von Neumann neighborhood)
    template <size_t Dimensions, size_t Radius>
    consteval size_t von_neumann_size() {
        // e.g., 2D radius 1 = 5 points. 3D radius 1 = 7 points.
        return 1 + (2 * Radius * Dimensions);
    }
}