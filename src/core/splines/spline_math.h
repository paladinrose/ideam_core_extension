#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <span>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

namespace ideam::core {

enum class CurveType : uint8_t {
    LINEAR = 0,
    CATMULL_ROM,
    CUBIC_BEZIER,
    STEP,
    NONE
};

// Pure, stateless math operations for SIMD/DOD kernels and OOP evaluation
template<size_t N, typename T>
struct SplineMath {
    [[nodiscard]] static constexpr glm::vec<N, T> evaluate_catmull_rom(T t, const glm::vec<N, T>& p0, const glm::vec<N, T>& p1, const glm::vec<N, T>& p2, const glm::vec<N, T>& p3) noexcept {
        const T t2 = t * t;
        const T t3 = t2 * t;
        
        return static_cast<T>(0.5) * (
            (-p0 + static_cast<T>(3.0) * p1 - static_cast<T>(3.0) * p2 + p3) * t3 +
            (static_cast<T>(2.0) * p0 - static_cast<T>(5.0) * p1 + static_cast<T>(4.0) * p2 - p3) * t2 +
            (-p0 + p2) * t +
            (static_cast<T>(2.0) * p1)
        );
    }

    [[nodiscard]] static constexpr glm::vec<N, T> evaluate_cubic_bezier(T t, const glm::vec<N, T>& p0, const glm::vec<N, T>& p1, const glm::vec<N, T>& p2, const glm::vec<N, T>& p3) noexcept {
        const T u = static_cast<T>(1.0) - t;
        const T u2 = u * u;
        const T u3 = u2 * u;
        const T t2 = t * t;
        const T t3 = t2 * t;

        return (u3 * p0) + (static_cast<T>(3.0) * u2 * t * p1) + (static_cast<T>(3.0) * u * t2 * p2) + (t3 * p3);
    }
};
} // namespace ideam::core