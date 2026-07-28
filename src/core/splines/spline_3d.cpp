#include "spline_3d.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

namespace ideam::core {

template <typename T>
std::vector<typename Spline3D<T>::VecType> Spline3D<T>::get_smoothed_points(int p_max_subdivisions) const {
    if (p_max_subdivisions < 3) p_max_subdivisions = 3;
    
    std::vector<VecType> smooth_points;
    if (this->points.empty()) return smooth_points;

    for (size_t i = 0; i < this->points.size() - 1; ++i) {
        smooth_points.push_back(this->points[i].point);

        const auto& sec = this->sections[i];
        if (sec.curve_type == CurveType::CATMULL_ROM || sec.curve_type == CurveType::CUBIC_BEZIER) {
            
            T pa = static_cast<T>(i) / static_cast<T>(this->points.size() - 1);
            T pb = static_cast<T>(i + 1) / static_cast<T>(this->points.size() - 1);
            
            T per_step = (pb - pa) / static_cast<T>(p_max_subdivisions);

            for (int j = 1; j < p_max_subdivisions; ++j) {
                smooth_points.push_back(this->get_point(pa + (per_step * static_cast<T>(j)), false));
            }
        }
    }
    smooth_points.push_back(this->points.back().point);
    return smooth_points;
}

template <typename T>
void Spline3D<T>::get_min_max(VecType& r_min, VecType& r_max, bool p_use_smoothed) const {
    if (this->points.empty()) {
        r_min = VecType(static_cast<T>(0.0));
        r_max = VecType(static_cast<T>(0.0));
        return;
    }

    if (p_use_smoothed) {
        std::vector<VecType> pts = get_smoothed_points();
        r_min = pts[0];
        r_max = pts[0];
        for (const auto& p : pts) {
            r_min = glm::min(r_min, p);
            r_max = glm::max(r_max, p);
        }
    } else {
        r_min = this->points[0].point;
        r_max = this->points[0].point;
        for (const auto& pt : this->points) {
            r_min = glm::min(r_min, pt.point);
            r_max = glm::max(r_max, pt.point);
        }
    }
}

template <typename T>
void Spline3D<T>::rotate_spline(const VecType& p_euler_angles) {
    if (this->points.empty()) return;

    VecType min_bounds, max_bounds;
    get_min_max(min_bounds, max_bounds, false);
    VecType center = (min_bounds + max_bounds) * static_cast<T>(0.5);

    // Build transformation matrix[cite: 1]
    glm::tmat4x4<T, glm::defaultp> transform(static_cast<T>(1.0));
    transform = glm::translate(transform, center);
    
    // Apply euler rotations
    transform = glm::rotate(transform, p_euler_angles.y, VecType(static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0)));
    transform = glm::rotate(transform, p_euler_angles.x, VecType(static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0)));
    transform = glm::rotate(transform, p_euler_angles.z, VecType(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0)));
    
    transform = glm::translate(transform, -center);

    // Apply to all control points and handles
    for (auto& pt : this->points) {
        glm::tvec4<T, glm::defaultp> p(pt.point.x, pt.point.y, pt.point.z, static_cast<T>(1.0));
        p = transform * p;
        pt.point = VecType(p.x, p.y, p.z);
        
        // Transform handles relative to rotation (strip translation)
        if (pt.use_back_handle || pt.use_fore_handle) {
            glm::tmat4x4<T, glm::defaultp> rot_only(static_cast<T>(1.0));
            rot_only = glm::rotate(rot_only, p_euler_angles.y, VecType(static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0)));
            rot_only = glm::rotate(rot_only, p_euler_angles.x, VecType(static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0)));
            rot_only = glm::rotate(rot_only, p_euler_angles.z, VecType(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0)));

            if (pt.use_back_handle) {
                glm::tvec4<T, glm::defaultp> bh(pt.back_handle.x, pt.back_handle.y, pt.back_handle.z, static_cast<T>(1.0));
                bh = rot_only * bh;
                pt.back_handle = VecType(bh.x, bh.y, bh.z);
            }
            if (pt.use_fore_handle) {
                glm::tvec4<T, glm::defaultp> fh(pt.fore_handle.x, pt.fore_handle.y, pt.fore_handle.z, static_cast<T>(1.0));
                fh = rot_only * fh;
                pt.fore_handle = VecType(fh.x, fh.y, fh.z);
            }
        }
    }
    this->is_dirty = true;
}

template <typename T>
void Spline3D<T>::scale_spline(const VecType& p_scale) {
    if (this->points.empty()) return;

    VecType min_bounds, max_bounds;
    get_min_max(min_bounds, max_bounds, false);
    VecType center = (min_bounds + max_bounds) * static_cast<T>(0.5);

    // Scale from center[cite: 1]
    for (auto& pt : this->points) {
        pt.point = ((pt.point - center) * p_scale) + center;

        if (pt.use_back_handle) {
            pt.back_handle *= p_scale;
        }
        if (pt.use_fore_handle) {
            pt.fore_handle *= p_scale;
        }
    }
    this->is_dirty = true;
}

template <typename T>
typename Spline3D<T>::VecType Spline3D<T>::get_closest_point_on_spline(const VecType& p_point, int p_samples, int* r_closest_anchor_before) const {
    if (this->points.empty()) return VecType(static_cast<T>(0.0));
    if (this->points.size() == 1) return this->points[0].point;

    // 1. Find the closest absolute anchor point[cite: 1]
    int closest_anchor = 0;
    T min_dist = glm::distance(p_point, this->points[0].point);

    for (size_t i = 1; i < this->points.size(); ++i) {
        T d = glm::distance(p_point, this->points[i].point);
        if (d < min_dist) {
            min_dist = d;
            closest_anchor = static_cast<int>(i);
        }
    }

    // 2. Determine which neighboring section (before or after) is closer[cite: 1]
    int a_section = std::max(0, closest_anchor - 1);
    int b_section = std::min(static_cast<int>(this->points.size() - 1), closest_anchor + 1);

    T a_dist = glm::distance(p_point, this->points[a_section].point);
    T b_dist = glm::distance(p_point, this->points[b_section].point);

    int target_section_start = 0;
    VecType a_point, b_point;
    T a_percent, b_percent;

    if (a_dist < b_dist) {
        target_section_start = a_section;
        a_point = this->points[a_section].point;
        b_point = this->points[closest_anchor].point;
        a_percent = static_cast<T>(a_section) / static_cast<T>(this->points.size() - 1);
        b_percent = static_cast<T>(closest_anchor) / static_cast<T>(this->points.size() - 1);
        b_dist = min_dist;
    } else {
        target_section_start = closest_anchor;
        a_point = this->points[closest_anchor].point;
        b_point = this->points[b_section].point;
        a_percent = static_cast<T>(closest_anchor) / static_cast<T>(this->points.size() - 1);
        b_percent = static_cast<T>(b_section) / static_cast<T>(this->points.size() - 1);
        a_dist = min_dist;
    }

    if (r_closest_anchor_before) {
        *r_closest_anchor_before = target_section_start;
    }

    // 3. Iteratively resample halfway points to converge on the exact nearest coordinate[cite: 1]
    T half_percent = (a_percent + b_percent) * static_cast<T>(0.5);
    VecType halfway = this->get_point(half_percent, false);
    T half_dist = glm::distance(p_point, halfway);

    for (int i = 0; i < p_samples; ++i) {
        if (a_dist < b_dist) {
            b_point = halfway;
            b_percent = half_percent;
            b_dist = half_dist;
        } else {
            a_point = halfway;
            a_percent = half_percent;
            a_dist = half_dist;
        }

        half_percent = (a_percent + b_percent) * static_cast<T>(0.5);
        halfway = this->get_point(half_percent, false);
        half_dist = glm::distance(p_point, halfway);
    }

    // Return the closest of the converging points[cite: 1]
    if (a_dist < b_dist && a_dist < half_dist) return a_point;
    if (b_dist < a_dist && b_dist < half_dist) return b_point;
    return halfway;
}

// Explicit Instantiations
template class Spline3D<float>;
template class Spline3D<double>;

} // namespace ideam::core