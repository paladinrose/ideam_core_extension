#pragma once

#include "spline_base.h"
#include <vector>

namespace ideam::core {

/**
 * Spline3D
 * Represents a curve in 3D space. Includes spatial transformation 
 * utilities and hierarchical proximity lookups.
 */
template <typename T = float>
class Spline3D : public SplineBase<3, T> {
public:
    using VecType = typename SplineBase<3, T>::VecType;

    Spline3D() = default;
    virtual ~Spline3D() = default;

    // --- 3D Spline Evaluation ---

    /**
     * Subdivides the spline into a dense array of flattened line segments.
     * Required for generating processable paths or drawing debug lines.
     */
    [[nodiscard]] std::vector<VecType> get_smoothed_points(int p_max_subdivisions = 10) const;

    // --- Spatial Queries ---

    /**
     * Calculates the Axis-Aligned Bounding Box (AABB) of the spline.
     * @param r_min The minimum bounds will be written here.
     * @param r_max The maximum bounds will be written here.
     * @param p_use_smoothed If true, evaluates the actual curve rather than just the anchors.
     */
    void get_min_max(VecType& r_min, VecType& r_max, bool p_use_smoothed = false) const;

    /**
     * Uses hierarchical sampling to estimate the closest point on the continuous spline 
     * to a given coordinate in 3D space.
     */
    [[nodiscard]] VecType get_closest_point_on_spline(const VecType& p_point, int p_samples = 10, int* r_closest_anchor_before = nullptr) const;

    // --- Spatial Transformations ---

    /**
     * Rotates all control points around the calculated center of the spline's bounding box[cite: 1].
     * @param p_euler_angles Rotation in radians (X, Y, Z).
     */
    void rotate_spline(const VecType& p_euler_angles);

    /**
     * Scales all control points outward/inward relative to the spline's bounding box center[cite: 1].
     */
    void scale_spline(const VecType& p_scale);
};

} // namespace ideam::core