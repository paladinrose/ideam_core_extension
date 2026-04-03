#ifndef IDEAM_CORE_STOCHASTIC_CULL_LOGIC_H
#define IDEAM_CORE_STOCHASTIC_CULL_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit> 
#include <concepts> 

namespace ideam::core {

/**
 * StochasticCullLogic<T>
 * A probabilistic filter that prunes elements based on a random roll.
 */
template <typename T = float>
struct StochasticCullLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr LogicRequirement requirements = LogicRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    uint32_t target_buffer_id = 0;
    uint32_t column_id       = 0;
    float global_probability = 0.5f; 
    uint32_t seed_base       = 0x811c9dc5; 

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute_cull(MemoryBufferSelectionPOD& r_selection, 
                      const TaskContextPOD& p_context, 
                      const T_View& p_view) const {
        
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection, p_view);
            else _cull_sparse(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_view, p_context);
        }
    }

    template<typename T_View, typename T_Strategy>
    void execute_sim(const TaskContextPOD& p_context, const T_View& p_view) const { /* No-op */ }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(uint32_t& r_rng_state, float prob) const {
        r_rng_state ^= r_rng_state << 13;
        r_rng_state ^= r_rng_state >> 17;
        r_rng_state ^= r_rng_state << 5;
        
        uint32_t mantissa = (r_rng_state & 0x007FFFFF) | 0x3F800000;
        float random_val = std::bit_cast<float>(mantissa) - 1.0f; 
        
        return random_val <= prob; 
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        uint32_t rng_state = seed_base ^ (p_view.get_version() * 0x27D4EB2D);

        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            float prob = global_probability;
            if constexpr (!std::is_void_v<T> && !std::is_same_v<T, uint8_t>) {
                prob = static_cast<float>(p_view[i]);
            }

            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(rng_state, prob)) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            } else {
                _evaluate(rng_state, 0.0f); // Keep RNG state deterministic
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t* indices = r_selection.data.indices;
        int64_t write_ptr = 0;
        uint32_t base_rng_state = seed_base ^ (p_view.get_version() * 0x27D4EB2D);

        for (int64_t i = 0; i < r_selection.element_count; ++i) {
            const int64_t idx = indices[i];
            uint32_t local_state = base_rng_state ^ static_cast<uint32_t>(idx * 0x9E3779B9);
            
            float prob = global_probability;
            if constexpr (!std::is_void_v<T> && !std::is_same_v<T, uint8_t>) {
                prob = static_cast<float>(p_view[idx]);
            }

            if (_evaluate(local_state, prob)) {
                indices[write_ptr++] = idx;
            }
        }
        r_selection.element_count = write_ptr;
    }

    template <typename T_View>
    void _add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        uint32_t base_rng_state = seed_base ^ (p_view.get_version() * 0x27D4EB2D);
        const int64_t words = (r_selection.capacity + 63) >> 6;

        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                // Deterministic jump for the global index
                uint32_t local_state = base_rng_state ^ static_cast<uint32_t>(global_index * 0x9E3779B9);

                float prob = global_probability;
                if constexpr (!std::is_void_v<T> && !std::is_same_v<T, uint8_t>) {
                    prob = static_cast<float>(p_view[global_index]);
                }

                if (_evaluate(local_state, prob)) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                
                mask &= (mask - 1); 
            }
        }
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_STOCHASTIC_CULL_LOGIC_H