#ifndef IDEAM_CORE_COLOR_CULL_LOGIC_H
#define IDEAM_CORE_COLOR_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <godot_cpp/variant/color.hpp>
#include <algorithm>

namespace ideam::core {

/**
 * ColorCullLogic<T>
 * Performs RGBA, HSV, or Distance-based filtering on color properties.
 * T: The color storage type (usually godot::Color or a 4-float POD).
 */
template <typename T>
struct ColorCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;

    enum class ColorMode : uint8_t {
        CHANNELS_RGBA,
        SEMANTIC_HSV,
        DISTANCE_RGB
    };

    enum class Comparison : uint8_t { 
        EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL 
    };

    enum ChannelMask : uint8_t {
        R_H = 1 << 0,
        G_S = 1 << 1,
        B_V = 1 << 2,
        A   = 1 << 3
    };

    // --- Configuration Data ---
    uint32_t column_id   = 0; 
    ColorMode mode       = ColorMode::CHANNELS_RGBA;
    Comparison op        = Comparison::GREATER;
    uint8_t mask         = R_H;
    float threshold      = 0.5f;
    godot::Color target  = godot::Color(0, 0, 0, 1);

    /**
     * execute_cull
     * Dispatches to dense or sparse paths. Optimized via templated comparison logic.
     */
    template <typename T_View>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view);
        } else {
            _cull_sparse(r_selection, p_view);
        }
    }

private:
    /**
     * _evaluate_comp
     * Templated comparison to allow the compiler to optimize the check.
     */
    template <Comparison C>
    [[nodiscard]] FORCE_INLINE static bool _evaluate_comp(float p_val, float p_thresh) {
        if constexpr (C == Comparison::EQUAL) return p_val == p_thresh;
        if constexpr (C == Comparison::NOT_EQUAL) return p_val != p_thresh;
        if constexpr (C == Comparison::LESS) return p_val < p_thresh;
        if constexpr (C == Comparison::LESS_EQUAL) return p_val <= p_thresh;
        if constexpr (C == Comparison::GREATER) return p_val > p_thresh;
        if constexpr (C == Comparison::GREATER_EQUAL) return p_val >= p_thresh;
        return false;
    }

    /**
     * _evaluate_logic
     * Full color evaluation based on the configured mode.
     */
    template <Comparison C>
    [[nodiscard]] FORCE_INLINE bool _evaluate_logic(const T& p_color) const {
        // Cast to float array for raw access
        const float* v = reinterpret_cast<const float*>(&p_color);

        if (mode == ColorMode::DISTANCE_RGB) {
            float dr = v[0] - target.r;
            float dg = v[1] - target.g;
            float db = v[2] - target.b;
            return _evaluate_comp<C>(dr * dr + dg * dg + db * db, threshold * threshold);
        } 
        
        if (mode == ColorMode::SEMANTIC_HSV) {
            float hsv[4];
            _rgb_to_hsv(v, hsv);
            if ((mask & R_H) && !_evaluate_comp<C>(hsv[0], threshold)) return false;
            if ((mask & G_S) && !_evaluate_comp<C>(hsv[1], threshold)) return false;
            if ((mask & B_V) && !_evaluate_comp<C>(hsv[2], threshold)) return false;
            if ((mask & A)   && !_evaluate_comp<C>(hsv[3], threshold)) return false;
            return true;
        }

        // Default: CHANNELS_RGBA
        if ((mask & R_H) && !_evaluate_comp<C>(v[0], threshold)) return false;
        if ((mask & G_S) && !_evaluate_comp<C>(v[1], threshold)) return false;
        if ((mask & B_V) && !_evaluate_comp<C>(v[2], threshold)) return false;
        if ((mask & A)   && !_evaluate_comp<C>(v[3], threshold)) return false;
        return true;
    }

    /**
     * _rgb_to_hsv
     * Local conversion logic ported from legacy color_step.cpp
     */
    FORCE_INLINE static void _rgb_to_hsv(const float* p_rgb, float* r_hsv) {
        const float r = p_rgb[0], g = p_rgb[1], b = p_rgb[2];
        const float max_c = std::max({r, g, b});
        const float min_c = std::min({r, g, b});
        const float delta = max_c - min_c;

        r_hsv[2] = max_c; // V
        r_hsv[1] = (max_c > 0.0001f) ? (delta / max_c) : 0.0f; // S

        if (delta < 0.0001f) {
            r_hsv[0] = 0.0f;
        } else {
            if (r >= max_c) r_hsv[0] = (g - b) / delta;
            else if (g >= max_c) r_hsv[0] = 2.0f + (b - r) / delta;
            else r_hsv[0] = 4.0f + (r - g) / delta;
            r_hsv[0] /= 6.0f;
            if (r_hsv[0] < 0.0f) r_hsv[0] += 1.0f;
        }
        r_hsv[3] = p_rgb[3]; // A
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        // Optimized switch-dispatch to keep inner loops branchless
        switch (op) {
            case Comparison::EQUAL:         _loop_dense<Comparison::EQUAL>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::NOT_EQUAL:     _loop_dense<Comparison::NOT_EQUAL>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::LESS:          _loop_dense<Comparison::LESS>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::LESS_EQUAL:    _loop_dense<Comparison::LESS_EQUAL>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::GREATER:       _loop_dense<Comparison::GREATER>(bitset, count, p_view, r_selection.element_count); break;
            case Comparison::GREATER_EQUAL: _loop_dense<Comparison::GREATER_EQUAL>(bitset, count, p_view, r_selection.element_count); break;
        }
    }

    template <Comparison C, typename T_View>
    FORCE_INLINE void _loop_dense(uint64_t* p_bitset, int64_t p_cap, const T_View& p_view, int64_t& r_count) const {
        for (int64_t i = 0; i < p_cap; ++i) {
            if (p_bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate_logic<C>(p_view[i])) {
                    p_bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t* indices = r_selection.data.indices;
        int64_t write_ptr = 0;
        const int64_t count = r_selection.element_count;

        switch (op) {
            case Comparison::EQUAL:         _loop_sparse<Comparison::EQUAL>(indices, count, p_view, write_ptr); break;
            case Comparison::NOT_EQUAL:     _loop_sparse<Comparison::NOT_EQUAL>(indices, count, p_view, write_ptr); break;
            case Comparison::LESS:          _loop_sparse<Comparison::LESS>(indices, count, p_view, write_ptr); break;
            case Comparison::LESS_EQUAL:    _loop_sparse<Comparison::LESS_EQUAL>(indices, count, p_view, write_ptr); break;
            case Comparison::GREATER:       _loop_sparse<Comparison::GREATER>(indices, count, p_view, write_ptr); break;
            case Comparison::GREATER_EQUAL: _loop_sparse<Comparison::GREATER_EQUAL>(indices, count, p_view, write_ptr); break;
        }
        r_selection.element_count = write_ptr;
    }

    template <Comparison C, typename T_View>
    FORCE_INLINE void _loop_sparse(int64_t* p_indices, int64_t p_count, const T_View& p_view, int64_t& r_write_ptr) const {
        for (int64_t i = 0; i < p_count; ++i) {
            int64_t idx = p_indices[i];
            if (_evaluate_logic<C>(p_view[idx])) {
                p_indices[r_write_ptr++] = idx;
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_COLOR_CULL_LOGIC_H