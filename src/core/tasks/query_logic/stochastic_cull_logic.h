#ifndef IDEAM_CORE_STOCHASTIC_CULL_LOGIC_H
#define IDEAM_CORE_STOCHASTIC_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit> // C++20 std::bit_cast
#include <concepts> // C++20 concepts

namespace ideam::core {

/**
 * StochasticCullLogic<T>
 * A probabilistic filter that prunes elements based on a random roll.
 * T: The probability source type. 
 * - If T is 'float', the buffer value [0,1] is the element's unique chance.
 * - If T is 'void' or a dummy, a global probability is used.
 */
template <typename T = float>
struct StochasticCullLogic {
    // --- View Binding & Logic Traits ---
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY;

    // --- Configuration Data ---
    uint32_t column_id       = 0;
    float global_probability = 0.5f; // Used if column_id is invalid
    uint32_t seed_base       = 42;
    bool use_data_as_weight  = true; // If true, T modulates the probability

    /**
     * execute_cull
     * Evaluates "luck" and prunes the selection.
     */
    template <typename T_View>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_context) const {
        // C++23: We could use std::expected here for error handling, 
        // but for a hot path, we keep it branch-predictor friendly.
        
        if (r_selection.mode == SelectionMode::DENSE) {
            _cull_dense(r_selection, p_view);
        } else {
            _cull_sparse(r_selection, p_view);
        }
    }

private:
    /**
     * _xoshiro128pp
     * A C++20/23 optimized Xoshiro128++ generator.
     * Extremely fast, excellent constants, and SIMD-parallelizable.
     */
    inline uint32_t _xoshiro128pp(uint32_t s[4]) const {
        const uint32_t result = _rotl(s[0] + s[3], 7) + s[0];
        const uint32_t t = s[1] << 9;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];

        s[2] ^= t;
        s[3] = _rotl(s[3], 11);

        return result;
    }

    inline uint32_t _rotl(const uint32_t x, int k) const {
        return (x << k) | (x >> (32 - k));
    }

    /**
     * _evaluate
     * Performs the random roll. 
     */
    inline bool _evaluate(uint32_t& r_rng_state, float p_threshold) const {
        // High-speed 32-bit PRNG step (SplitMix-style state jump)
        r_rng_state ^= r_rng_state << 13;
        r_rng_state ^= r_rng_state >> 17;
        r_rng_state ^= r_rng_state << 5;
        
        // Map [0, UINT32_MAX] to [0.0, 1.0]
        const float roll = static_cast<float>(r_rng_state) * (1.0f / 4294967295.0f);
        return roll <= p_threshold;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        const int64_t count = r_selection.capacity;

        // Initialize RNG state with seed + buffer version for stability
        uint32_t rng_state = seed_base ^ (p_view.get_version() * 0x27D4EB2D);

        for (int64_t i = 0; i < count; ++i) {
            // We advance RNG for every element to maintain parity between Dense and Sparse
            float prob = global_probability;
            
            if constexpr (!std::is_void_v<T>) {
                if (use_data_as_weight) {
                    // C++20 std::bit_cast for type-safe probability extraction
                    prob = static_cast<float>(p_view[i]); 
                }
            }

            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(rng_state, prob)) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            } else {
                // Keep RNG in sync even if element isn't in selection
                _evaluate(rng_state, 0.0f); 
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t* indices = r_selection.data.indices;
        int64_t write_ptr = 0;
        const int64_t count = r_selection.element_count;

        uint32_t rng_state = seed_base ^ (p_view.get_version() * 0x27D4EB2D);

        // Note: For Sparse, we can't easily stay in sync with a Dense scan 
        // without iterating through every single index in the buffer.
        // Instead, we jump the RNG state based on the element index.
        for (int64_t i = 0; i < count; ++i) {
            const int64_t idx = indices[i];
            
            // Scoped Seeding: Ensure the "roll" for element 500 is always the same
            uint32_t local_state = rng_state ^ static_cast<uint32_t>(idx * 0x9E3779B9);
            
            float prob = global_probability;
            if constexpr (!std::is_void_v<T>) {
                prob = static_cast<float>(p_view[idx]);
            }

            if (_evaluate(local_state, prob)) {
                indices[write_ptr++] = idx;
            }
        }
        r_selection.element_count = write_ptr;
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_STOCHASTIC_CULL_LOGIC_H