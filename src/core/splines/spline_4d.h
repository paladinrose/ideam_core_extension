#pragma once

#include "spline_base.h"
#include <vector>
#include <span>

namespace ideam::core {

/**
 * Spline4D
 * Represents a curve in 4D space. 
 * Often used for animated 3D transforms with a weight track (w), 
 * or traversing complex color gradients (RGBA / HSVA).
 */
template <typename T = float>
class Spline4D : public SplineBase<4, T> {
public:
    using VecType = typename SplineBase<4, T>::VecType;
    using Vec3Type = glm::vec<3, T>;

    Spline4D() = default;
    virtual ~Spline4D() = default;

    // --- 4D Spline Evaluation ---

    /**
     * Subdivides the spline into a dense array of flattened 4D segments[cite: 1].
     */
    [[nodiscard]] std::vector<VecType> get_smoothed_points(int p_max_subdivisions = 10) const;

    // --- Spatial Queries ---

    /**
     * Calculates the Axis-Aligned Bounding Box (AABB) across all 4 dimensions[cite: 1].
     */
    void get_min_max(VecType& r_min, VecType& r_max, bool p_use_smoothed = false) const;

    /**
     * Uses hierarchical sampling to estimate the closest point on the continuous spline 
     * to a given 4D coordinate[cite: 1].
     */
    [[nodiscard]] VecType get_closest_point_on_spline(const VecType& p_point, int p_samples = 10, int* r_closest_anchor_before = nullptr) const;

    // --- Semi-Spatial Transformations ---

    /**
     * Rotates the 3D components (X, Y, Z) of all control points around the 3D center 
     * of the spline's bounding box. The W component is preserved[cite: 1].
     * @param p_euler_angles Rotation in radians (X, Y, Z).
     */
    void rotate_spline(const Vec3Type& p_euler_angles);

    /**
     * Scales the 3D components (X, Y, Z) of all control points outward/inward relative 
     * to the bounding box center. The W component is preserved[cite: 1].
     */
    void scale_spline(const Vec3Type& p_scale);

    // --- 4D Generators ---

    /**
     * Takes an array of RGB(A) colors, converts them to HSV(A), and builds 
     * a smoothly interpolated 4D Catmull-Rom spline[cite: 1].
     */
    [[nodiscard]] static Spline4D<T> create_gradient(std::span<const VecType> p_colors);
};

} // namespace ideam::core