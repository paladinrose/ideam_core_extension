#include "spline_1d.h"
#include <cmath>
#include <algorithm>

namespace ideam::core {

template <typename T>
std::vector<T> Spline1D<T>::get_percents_at_value(T p_value, int p_samples) const {
    std::vector<T> percents;

    if (this->points.size() < 2) {
        return percents;
    }

    for (size_t i = 0; i < this->points.size() - 1; ++i) {
        T a = this->points[i].point.x;
        T c = this->points[i + 1].point.x;
        T b = static_cast<T>(-1.0);

        // Check if the value falls within the bounds of this segment's endpoints
        if ((p_value >= a && p_value <= c) || (p_value <= a && p_value >= c)) {
            
            // Dummy variable required by get_section_at_percent, 
            // though here we calculate the percent manually.
            T a_per = static_cast<T>(i) / static_cast<T>(this->points.size() - 1); 
            T c_per = static_cast<T>(i + 1) / static_cast<T>(this->points.size() - 1);
            T b_per = static_cast<T>(-1.0);

            // Binary search the segment
            for (int j = 0; j < p_samples; ++j) {
                b_per = (a_per + c_per) * static_cast<T>(0.5);
                b = this->get_point(b_per).x;

                if (p_value > b) {
                    if (c > a) { a = b; a_per = b_per; } 
                    else       { c = b; c_per = b_per; }
                } 
                else if (p_value < b) {
                    if (c > a) { c = b; c_per = b_per; } 
                    else       { a = b; a_per = b_per; }
                } 
                else {
                    percents.push_back(b_per);
                    break;
                }
            }

            // If we exhausted samples without a perfect match, pick the closest bound
            if (p_value != b) {
                T diff_c = std::abs(c - p_value);
                T diff_a = std::abs(a - p_value);
                
                if (diff_c < diff_a) {
                    percents.push_back(c_per);
                } else {
                    percents.push_back(a_per);
                }
            }
        }
    }

    // Ensure we don't return duplicate hits at exact segment boundaries
    auto last = std::unique(percents.begin(), percents.end(), [](T l, T r) {
        return std::abs(l - r) < static_cast<T>(0.0001);
    });
    percents.erase(last, percents.end());

    return percents;
}

template <typename T>
T Spline1D<T>::get_min_value() const noexcept {
    if (this->points.empty()) return static_cast<T>(0.0);
    
    T min_val = this->points[0].point.x;
    for (size_t i = 1; i < this->points.size(); ++i) {
        if (this->points[i].point.x < min_val) {
            min_val = this->points[i].point.x;
        }
    }
    return min_val;
}

template <typename T>
T Spline1D<T>::get_max_value() const noexcept {
    if (this->points.empty()) return static_cast<T>(0.0);
    
    T max_val = this->points[0].point.x;
    for (size_t i = 1; i < this->points.size(); ++i) {
        if (this->points[i].point.x > max_val) {
            max_val = this->points[i].point.x;
        }
    }
    return max_val;
}

template <typename T>
Spline1D<T> Spline1D<T>::create_zero_to_one() {
    Spline1D<T> spline;
    spline.add_point(VecType(static_cast<T>(0.0)), CurveType::LINEAR);
    spline.add_point(VecType(static_cast<T>(1.0)), CurveType::LINEAR);
    return spline;
}

template <typename T>
Spline1D<T> Spline1D<T>::create_one_to_zero() {
    Spline1D<T> spline;
    spline.add_point(VecType(static_cast<T>(1.0)), CurveType::LINEAR);
    spline.add_point(VecType(static_cast<T>(0.0)), CurveType::LINEAR);
    return spline;
}

template <typename T>
Spline1D<T> Spline1D<T>::create_equidistant(size_t p_count, CurveType p_type) {
    Spline1D<T> spline;
    if (p_count < 2) p_count = 2;

    T inv = static_cast<T>(1.0) / static_cast<T>(p_count - 1);
    for (size_t i = 0; i < p_count; ++i) {
        spline.add_point(VecType(inv * static_cast<T>(i)), p_type);
    }
    return spline;
}

// ============================================================================
// Explicit Template Instantiations
// ============================================================================

template class Spline1D<float>;
template class Spline1D<double>;

} // namespace ideam::core