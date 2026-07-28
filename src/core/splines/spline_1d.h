#pragma once

#include "spline_base.h"
#include <vector>

namespace ideam::core {

/**
 * Spline1D
 * Represents a single-dimensional curve, often used for 
 * animation curves, value easing, or alpha tracks.
 */
template <typename T = float>
class Spline1D : public SplineBase<1, T> {
public:
    using VecType = typename SplineBase<1, T>::VecType;

    Spline1D() = default;
    virtual ~Spline1D() = default;

    // --- 1D Specific Queries ---
    
    /**
     * Solves for t (percent) given a target output value f(t).
     * Uses a binary search root-finding method per segment.
     * @param p_value The output value to search for.
     * @param p_samples The maximum number of binary search iterations per segment.
     * @return A list of normalized percentages (t) where the curve crosses the value.
     */
    [[nodiscard]] std::vector<T> get_percents_at_value(T p_value, int p_samples = 10) const;

    /**
     * @return The absolute minimum value among all anchor points.
     */
    [[nodiscard]] T get_min_value() const noexcept;

    /**
     * @return The absolute maximum value among all anchor points.
     */
    [[nodiscard]] T get_max_value() const noexcept;

    // --- 1D Spline Builders ---

    /**
     * Creates a standard normalized linear spline rising from 0 to 1.
     */
    [[nodiscard]] static Spline1D<T> create_zero_to_one();

    /**
     * Creates a standard normalized linear spline falling from 1 to 0.
     */
    [[nodiscard]] static Spline1D<T> create_one_to_zero();

    /**
     * Creates a spline with 'p_count' points distributed linearly from 0 to 1.
     */
    [[nodiscard]] static Spline1D<T> create_equidistant(size_t p_count, CurveType p_type = CurveType::LINEAR);
};

} // namespace ideam::core