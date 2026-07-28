#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <span>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

#include "spline_math.h"

namespace ideam::core {

enum class PointSearchType : uint8_t {
    ANY = 0,
    BEFORE,
    AFTER
};

template<typename T>
struct SplineSection {
    CurveType curve_type{CurveType::LINEAR};
    T weight{static_cast<T>(1.0)};
    T length{static_cast<T>(0.0)};
};

// AoS Layout for the OOP side.
// DOD systems will likely use SoA (separate arrays for points, back_handles, etc.)
template<size_t N, typename T>
struct SplinePoint {
    glm::vec<N, T> point{static_cast<T>(0)};
    glm::vec<N, T> back_handle{static_cast<T>(0)};
    glm::vec<N, T> fore_handle{static_cast<T>(0)};
    bool use_back_handle{false};
    bool use_fore_handle{false};
};

// OOP Spline Container
template<size_t N, typename T = float>
class SplineBase {
public:
    using VecType = glm::vec<N, T>;
    using PointType = SplinePoint<N, T>;
    using SectionType = SplineSection<T>;

protected:
    std::vector<PointType> points;
    std::vector<SectionType> sections;

    VecType start_cap{static_cast<T>(0)};
    VecType end_cap{static_cast<T>(0)};

    bool is_wrap{false};
    bool is_dirty{true};

    CurveType default_curve_type{CurveType::LINEAR};
    T default_weight{static_cast<T>(1.0)};

    T total_weight{static_cast<T>(0.0)};
    T total_length{static_cast<T>(0.0)};

    void set_cap_points();
    void update_totals();

public:
    SplineBase() = default;
    virtual ~SplineBase() = default;

    // --- State Accessors ---
    [[nodiscard]] inline size_t get_length() const noexcept { return is_wrap ? points.size() - 1 : points.size(); }
    [[nodiscard]] inline size_t get_section_count() const noexcept { return sections.size(); }
    
    [[nodiscard]] inline bool get_wrap() const noexcept { return is_wrap; }
    void set_wrap(bool p_wrap);

    // --- Modification ---
    void add_point(const VecType& p_point, CurveType p_type = CurveType::NONE, T p_weight = static_cast<T>(-1.0));
    void remove_point(size_t p_index);
    void clear();

    // --- Data Access ---
    [[nodiscard]] std::span<const PointType> get_points() const noexcept { return points; }
    [[nodiscard]] std::span<const SectionType> get_sections() const noexcept { return sections; }

    // --- Evaluation ---
    [[nodiscard]] size_t get_section_at_percent(T p_percent, bool p_weighted, T& r_local_percent) const noexcept;
    [[nodiscard]] VecType get_point(T p_percent, bool p_weighted = true) const;
    [[nodiscard]] T get_total_spline_length(T p_sample_percent = static_cast<T>(0.1), bool p_force_recalculate = false);
};

} // namespace ideam::core