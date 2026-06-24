#pragma once

#include <limits>
#include <type_traits>

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