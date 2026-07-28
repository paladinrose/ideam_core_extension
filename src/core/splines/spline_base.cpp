#include "spline_base.h"
#include <cmath>

namespace ideam::core {

template<size_t N, typename T>
void SplineBase<N, T>::set_cap_points() {
    size_t l = points.size();
    if (l > 1) {
        VecType point_diff = points[1].point - points[0].point;
        start_cap = points[0].point - point_diff;

        if (l >= 3) {
            // Loop connection detection
            // Note: Use GLM length checks for floating point comparisons
            if (glm::distance(points[0].point, points[l - 1].point) < static_cast<T>(0.0001)) {
                start_cap = points[l - 2].point;
                end_cap = points[1].point;
            } else {
                point_diff = points[l - 1].point - points[l - 2].point;
                end_cap = points[l - 1].point + point_diff;
            }
        } else {
            end_cap = points[1].point + point_diff;
        }
    }
}

template<size_t N, typename T>
void SplineBase<N, T>::update_totals() {
    total_weight = static_cast<T>(0.0);
    for (const auto& sec : sections) {
        total_weight += sec.weight;
    }
    // Defer length calculation until requested, as it is computationally expensive.
    is_dirty = false;
}

template<size_t N, typename T>
void SplineBase<N, T>::set_wrap(bool p_wrap) {
    if (p_wrap != is_wrap) {
        if (p_wrap) {
            if (points.size() > 1 && glm::distance(points.front().point, points.back().point) >= static_cast<T>(0.0001)) {
                points.push_back(points.front());
            }
        } else {
            if (points.size() > 1 && glm::distance(points.front().point, points.back().point) < static_cast<T>(0.0001)) {
                points.pop_back();
            }
        }
        
        // Sync sections length
        if (points.empty()) {
            sections.clear();
        } else {
            sections.resize(points.size() - 1, SectionType{default_curve_type, default_weight, static_cast<T>(0.0)});
        }

        is_wrap = p_wrap;
        set_cap_points();
        update_totals();
    }
}

template<size_t N, typename T>
void SplineBase<N, T>::add_point(const VecType& p_point, CurveType p_type, T p_weight) {
    CurveType final_type = (p_type == CurveType::NONE) ? default_curve_type : p_type;
    T final_weight = (p_weight < static_cast<T>(0.0)) ? default_weight : p_weight;

    PointType new_point;
    new_point.point = p_point;
    
    points.push_back(new_point);

    if (points.size() > 1) {
        sections.push_back(SectionType{final_type, final_weight, static_cast<T>(0.0)});
    }

    set_cap_points();
    update_totals();
}

template<size_t N, typename T>
void SplineBase<N, T>::remove_point(size_t p_index) {
    if (p_index < points.size()) {
        points.erase(points.begin() + p_index);
        
        // Remove corresponding section (safeguarded bounds)
        if (p_index < sections.size()) {
            sections.erase(sections.begin() + p_index);
        } else if (!sections.empty()) {
            sections.pop_back();
        }

        set_cap_points();
        update_totals();
    }
}

template<size_t N, typename T>
void SplineBase<N, T>::clear() {
    points.clear();
    sections.clear();
    update_totals();
}

template<size_t N, typename T>
size_t SplineBase<N, T>::get_section_at_percent(T p_percent, bool p_weighted, T& r_local_percent) const noexcept {
    if (sections.empty()) {
        r_local_percent = static_cast<T>(0.0);
        return 0;
    }

    T t = p_percent;
    if (is_wrap) {
        t = t - std::floor(t);
    } else {
        t = std::clamp(t, static_cast<T>(0.0), static_cast<T>(1.0));
    }

    if (p_weighted) {
        T current_weight = static_cast<T>(0.0);
        T percent_as_weight = t * total_weight;

        for (size_t i = 0; i < sections.size(); ++i) {
            T next_weight = current_weight + sections[i].weight;
            if (current_weight <= percent_as_weight && next_weight > percent_as_weight) {
                r_local_percent = (percent_as_weight - current_weight) / sections[i].weight;
                return i;
            }
            current_weight = next_weight;
        }
    } else {
        T current_distance = static_cast<T>(0.0);
        T percent_as_distance = t * total_length;

        for (size_t i = 0; i < sections.size(); ++i) {
            T next_distance = current_distance + sections[i].length;
            if (current_distance <= percent_as_distance && next_distance > percent_as_distance) {
                r_local_percent = (percent_as_distance - current_distance) / (sections[i].length > 0 ? sections[i].length : static_cast<T>(1.0));
                return i;
            }
            current_distance = next_distance;
        }
    }
    
    r_local_percent = static_cast<T>(1.0);
    return sections.size() - 1;
}

template<size_t N, typename T>
typename SplineBase<N, T>::VecType SplineBase<N, T>::get_point(T p_percent, bool p_weighted) const {
    if (points.empty()) return VecType(static_cast<T>(0));
    if (points.size() == 1) return points[0].point;

    T local_t = static_cast<T>(0.0);
    size_t section_idx = get_section_at_percent(p_percent, p_weighted, local_t);
    const SectionType& sec = sections[section_idx];

    // Thread-safe, stateless control point resolution
    VecType a, b, c, d;
    size_t l = sections.size();

    // 1. Resolve logical points
    if (section_idx == 0 && l > 1) {
        a = start_cap; b = points[0].point; c = points[1].point; d = points[2].point;
    } else if (section_idx == 0) {
        a = start_cap; b = points[0].point; c = points[1].point; d = end_cap;
    } else if (section_idx == l - 1) {
        a = points[l - 2].point; b = points[l - 1].point; c = points[l].point; d = end_cap;
    } else {
        a = points[section_idx - 1].point;
        b = points[section_idx].point;
        c = points[section_idx + 1].point;
        d = points[section_idx + 2].point;
    }

    // 2. Override with specific handles if applicable
    if (points[section_idx].use_back_handle) {
        a = points[section_idx].point + points[section_idx].back_handle;
    }
    if (points[section_idx + 1].use_fore_handle) {
        d = points[section_idx + 1].point + points[section_idx + 1].fore_handle;
    }

    // 3. Evaluate statically
    switch (sec.curve_type) {
        case CurveType::CATMULL_ROM:
            return SplineMath<N, T>::evaluate_catmull_rom(local_t, a, b, c, d);
        case CurveType::CUBIC_BEZIER:
            return SplineMath<N, T>::evaluate_cubic_bezier(local_t, a, b, c, d);
        case CurveType::LINEAR:
            return glm::mix(points[section_idx].point, points[section_idx + 1].point, local_t);
        case CurveType::STEP:
            return points[section_idx].point;
        default:
            return points[section_idx].point;
    }
}

template<size_t N, typename T>
T SplineBase<N, T>::get_total_spline_length(T p_sample_percent, bool p_force_recalculate) {
    if (!is_dirty && !p_force_recalculate && total_length > static_cast<T>(0.0)) {
        return total_length;
    }

    T s_length = static_cast<T>(0.0);
    T current_weight = static_cast<T>(0.0);
    T sp = static_cast<T>(0.0);

    for (size_t j = 0; j < sections.size(); ++j) {
        sections[j].length = static_cast<T>(0.0);

        if (sections[j].curve_type == CurveType::LINEAR) {
            sections[j].length = glm::distance(points[j].point, points[j + 1].point);
            s_length += sections[j].length;
        } 
        else if (sections[j].curve_type == CurveType::CATMULL_ROM || sections[j].curve_type == CurveType::CUBIC_BEZIER) {
            VecType a = points[j].point;
            current_weight += sections[j].weight;
            
            // Protect against div-by-zero on empty/zero-weighted splines
            T isp = (total_weight > static_cast<T>(0.0)) ? (current_weight / total_weight) : static_cast<T>(1.0);

            for (T i = p_sample_percent; i <= static_cast<T>(1.0); i += p_sample_percent) {
                T mapped_t = glm::mix(sp, isp, i);
                VecType b = get_point(mapped_t, true); // Evaluate using weights
                T d = glm::distance(a, b);
                
                sections[j].length += d;
                s_length += d;
                a = b;
            }
            sp = isp;
        }
    }

    total_length = s_length;
    is_dirty = false;
    return total_length;
}

// ============================================================================
// Explicit Template Instantiations
// Ensures the compiler generates binary code for exactly the precision and 
// dimensions we care about, avoiding massive header bloat.
// ============================================================================

// 32-Bit Float Splines
template class SplineBase<1, float>;
template class SplineBase<2, float>;
template class SplineBase<3, float>;
template class SplineBase<4, float>;

// 64-Bit Double Splines (Required for Godot's real_t when compiling double precision)
template class SplineBase<1, double>;
template class SplineBase<2, double>;
template class SplineBase<3, double>;
template class SplineBase<4, double>;

} // namespace ideam::core::splines