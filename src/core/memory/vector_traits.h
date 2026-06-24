#pragma once

#include <limits>
#include <type_traits>
#include <algorithm>

// Concepts to detect fields at compile-time
template<typename T>
concept HasXAndY = requires(T v) { v.x; v.y; };

template<typename T>
concept HasRAndG = requires(T v) { v.r; v.g; };

// Primary templates to extract scalar types
template<typename T>
constexpr auto get_scalar_max() {
    if constexpr (HasXAndY<T>) {
        return std::numeric_limits<decltype(std::declval<T>().x)>::max();
    } else if constexpr (HasRAndG<T>) {
        return std::numeric_limits<decltype(std::declval<T>().r)>::max();
    } else {
        return std::numeric_limits<T>::max();
    }
}

template<typename T>
constexpr auto get_scalar_lowest() {
    if constexpr (HasXAndY<T>) {
        return std::numeric_limits<decltype(std::declval<T>().x)>::lowest();
    } else if constexpr (HasRAndG<T>) {
        return std::numeric_limits<decltype(std::declval<T>().r)>::lowest();
    } else {
        return std::numeric_limits<T>::lowest();
    }
}

// Functions that return a fully initialized Vector/Color filled with max/lowest values
template<typename T>
constexpr T get_max_bound() {
    T obj{};
    auto max_val = get_scalar_max<T>();
    
    if constexpr (HasXAndY<T>) {
        obj.x = max_val;
        obj.y = max_val;
        if constexpr (requires { obj.z; }) obj.z = max_val;
        if constexpr (requires { obj.w; }) obj.w = max_val;
    } else if constexpr (HasRAndG<T>) {
        obj.r = max_val;
        obj.g = max_val;
        obj.b = max_val;
        obj.a = max_val;
    } else {
        obj = max_val;
    }
    return obj;
}

template<typename T>
constexpr T get_lowest_bound() {
    T obj{};
    auto lowest_val = get_scalar_lowest<T>();
    
    if constexpr (HasXAndY<T>) {
        obj.x = lowest_val;
        obj.y = lowest_val;
        if constexpr (requires { obj.z; }) obj.z = lowest_val;
        if constexpr (requires { obj.w; }) obj.w = lowest_val;
    } else if constexpr (HasRAndG<T>) {
        obj.r = lowest_val;
        obj.g = lowest_val;
        obj.b = lowest_val;
        obj.a = lowest_val;
    } else {
        obj = lowest_val;
    }
    return obj;
}

template<typename T>
[[nodiscard]] constexpr T component_min(const T& a, const T& b) noexcept {
    if constexpr (HasXAndY<T>) {
        T obj{};
        obj.x = std::min(a.x, b.x);
        obj.y = std::min(a.y, b.y);
        if constexpr (requires { obj.z; }) obj.z = std::min(a.z, b.z);
        if constexpr (requires { obj.w; }) obj.w = std::min(a.w, b.w);
        return obj;
    } else if constexpr (HasRAndG<T>) {
        T obj{};
        obj.r = std::min(a.r, b.r);
        obj.g = std::min(a.g, b.g);
        obj.b = std::min(a.b, b.b);
        obj.a = std::min(a.a, b.a);
        return obj;
    } else {
        return std::min(a, b);
    }
}

// Functions that return the component-wise maximum of two instances
template<typename T>
[[nodiscard]] constexpr T component_max(const T& a, const T& b) noexcept {
    if constexpr (HasXAndY<T>) {
        T obj{};
        obj.x = std::max(a.x, b.x);
        obj.y = std::max(a.y, b.y);
        if constexpr (requires { obj.z; }) obj.z = std::max(a.z, b.z);
        if constexpr (requires { obj.w; }) obj.w = std::max(a.w, b.w);
        return obj;
    } else if constexpr (HasRAndG<T>) {
        T obj{};
        obj.r = std::max(a.r, b.r);
        obj.g = std::max(a.g, b.g);
        obj.b = std::max(a.b, b.b);
        obj.a = std::max(a.a, b.a);
        return obj;
    } else {
        return std::max(a, b);
    }
}

// Safely evaluates if any component in the type is non-zero
template<typename T>
[[nodiscard]] constexpr bool is_not_zero(const T& v) noexcept {
    if constexpr (HasXAndY<T>) {
        bool nz = (v.x != 0) || (v.y != 0);
        if constexpr (requires { v.z; }) nz = nz || (v.z != 0);
        if constexpr (requires { v.w; }) nz = nz || (v.w != 0);
        return nz;
    } else if constexpr (HasRAndG<T>) {
        return (v.r != 0) || (v.g != 0) || (v.b != 0) || (v.a != 0);
    } else {
        return v != 0;
    }
}

// Increments all available components by 1 (Critical for generic COUNT_NON_ZERO)
template<typename T>
constexpr void increment_all_components(T& v) noexcept {
    if constexpr (HasXAndY<T>) {
        v.x += 1; v.y += 1;
        if constexpr (requires { v.z; }) v.z += 1;
        if constexpr (requires { v.w; }) v.w += 1;
    } else if constexpr (HasRAndG<T>) {
        v.r += 1; v.g += 1; v.b += 1; v.a += 1;
    } else {
        v += 1;
    }
}

// Divides all components by a scalar value (Critical for AVERAGE)
template<typename T>
constexpr void divide_components_by_scalar(T& v, double scalar) noexcept {
    if constexpr (HasXAndY<T>) {
        v.x /= scalar; v.y /= scalar;
        if constexpr (requires { v.z; }) v.z /= scalar;
        if constexpr (requires { v.w; }) v.w /= scalar;
    } else if constexpr (HasRAndG<T>) {
        v.r /= scalar; v.g /= scalar; v.b /= scalar; v.a /= scalar;
    } else {
        v /= scalar;
    }
}

// Adds two generic types together, component by component
template<typename T>
constexpr void add_components(T& dest, const T& src) noexcept {
    if constexpr (HasXAndY<T>) {
        dest.x += src.x; dest.y += src.y;
        if constexpr (requires { dest.z; }) dest.z += src.z;
        if constexpr (requires { dest.w; }) dest.w += src.w;
    } else if constexpr (HasRAndG<T>) {
        dest.r += src.r; dest.g += src.g; dest.b += src.b; dest.a += src.a;
    } else {
        dest += src;
    }
}