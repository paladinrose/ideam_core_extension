#ifndef IDEAM_CORE_ATOMIC_VIEW_H
#define IDEAM_CORE_ATOMIC_VIEW_H

#include "strategies.h"
#include "../memory_grant_pod.h"
#include "view_traits.h"
#include <atomic>
#include <cstdint>
#include <type_traits>

namespace ideam::core {

/**
 * AtomicView<T, Strategy>
 * A thread-safe accessor for shared buffers.
 * Access is strictly bound to the MemoryBufferSelectionPOD.
 * Only supports types valid for std::atomic (trivially copyable).
 */
template<typename T, typename Strategy = FlatStrategy>
struct AtomicView {
    static_assert(std::is_trivially_copyable_v<T>, "AtomicView requires trivially copyable types.");

    // --- 8-Byte Block ---
    T* head_ptr = nullptr;
    const MemoryGrantPOD* grant = nullptr;

    // --- 4-Byte Block ---
    uint32_t grant_part_index = 0;
    uint32_t baked_buffer_version = 0;
    uint32_t baked_manager_version = 0;

    // --- Strategy Policy ---
    [[no_unique_address]] Strategy strategy;

    // --- Capability Traits ---
    static constexpr ViewCapability capabilities = 
        ViewCapability::LINEAR_ACCESS | 
        ViewCapability::RANDOM_ACCESS | 
        (Strategy::is_spatial ? ViewCapability::SPATIAL_ACCESS : ViewCapability::NONE);

    static constexpr bool is_spatial = Strategy::is_spatial;
    static constexpr bool is_simd = false;
    static constexpr uint32_t lane_width = 1;

    /**
     * is_valid
     * Reactive version check.
     */
    [[nodiscard]] inline bool is_valid() const {
        if (!grant || !grant->active) return false;
        if (grant_part_index >= grant->part_count) return false;

        const auto& part = grant->parts[grant_part_index];
        if (part.buffer_version_at_issue != baked_buffer_version) return false;
        if (grant->global_manager_version_ptr && *grant->global_manager_version_ptr != baked_manager_version) return false;
        
        return part.selection.is_valid();
    }

    /**
     * operator[]
     * Selection-Relative Linear Access.
     * Provides a standard reference to the atomic element for linear processing.
     */
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline std::atomic<T>& operator[](size_t p_selection_index) const {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;

        if (p_selection_index >= static_cast<size_t>(selection.element_count)) {
            return *reinterpret_cast<std::atomic<T>*>(head_ptr);
        }

        size_t actual_buffer_index = 0;
        switch (selection.mode) {
            case SelectionMode::SPARSE: {
                actual_buffer_index = static_cast<size_t>(selection.data.indices[p_selection_index]);
                break;
            }
            case SelectionMode::DENSE: {
                if (!selection.is_selected(static_cast<int64_t>(p_selection_index))) {
                    return *reinterpret_cast<std::atomic<T>*>(head_ptr);
                }
                actual_buffer_index = p_selection_index;
                break;
            }
            case SelectionMode::RANGE: {
                actual_buffer_index = static_cast<size_t>(selection.start_index) + p_selection_index;
                break;
            }
        }

        T* resolved = nullptr;
        if constexpr (std::is_empty_v<Strategy>) {
            resolved = Strategy::template resolve<T>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
        } else {
            resolved = strategy.template resolve<T>(head_ptr, actual_buffer_index, part.element_stride, part.capacity_bytes);
        }
        return *reinterpret_cast<std::atomic<T>*>(resolved);
    }

    /**
     * add
     * Atomically adds a value to the element at the specified coordinates/index.
     */
    template<typename... Args>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void add(T p_value, Args... p_coords) const {
        auto* atom = _get_atomic(p_coords...);
        if (atom) atom->fetch_add(p_value, std::memory_order_relaxed);
    }

    /**
     * store
     * Atomic set operation.
     */
    template<typename... Args>
    inline void store(T p_value, Args... p_coords) const {
        auto* atom = _get_atomic(p_coords...);
        if (atom) atom->store(p_value, std::memory_order_release);
    }

    /**
     * load
     * Atomic get operation.
     */
    template<typename... Args>
    inline T load(Args... p_coords) const {
        auto* atom = _get_atomic(p_coords...);
        return atom ? atom->load(std::memory_order_acquire) : T{};
    }

    /**
     * compare_exchange
     * Strong atomic swap/check.
     */
    template<typename... Args>
    inline bool compare_exchange(T& r_expected, T p_desired, Args... p_coords) const {
        auto* atom = _get_atomic(p_coords...);
        return atom ? atom->compare_exchange_strong(r_expected, p_desired) : false;
    }

private:
    /**
     * _get_atomic
     * Internal helper to resolve strategy-based pointer and verify selection.
     * Returns nullptr if the requested coordinate is outside the selection grant.
     */
    template<typename... Args>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline std::atomic<T>* _get_atomic(Args... p_coords) const {
        const auto& part = grant->parts[grant_part_index];
        const auto& selection = part.selection;
        T* ptr = nullptr;
        int64_t flat_idx = 0;

        if constexpr (sizeof...(Args) == 1) {
            flat_idx = static_cast<int64_t>((p_coords)...);
            ptr = strategy.resolve(head_ptr, static_cast<size_t>(flat_idx), part.element_stride, part.capacity_bytes);
        } else if constexpr (sizeof...(Args) == 2) {
            flat_idx = strategy.get_index_2d(p_coords..., part.element_stride);
            ptr = strategy.resolve_2d(head_ptr, p_coords..., part.element_stride);
        } else if constexpr (sizeof...(Args) == 3) {
            flat_idx = strategy.get_index_3d(p_coords..., part.element_stride);
            ptr = strategy.resolve_3d(head_ptr, p_coords..., part.element_stride);
        } else if constexpr (sizeof...(Args) == 4) {
            flat_idx = strategy.get_index_4d(p_coords..., part.element_stride);
            ptr = strategy.resolve_4d(head_ptr, p_coords..., part.element_stride);
        }

        if (!selection.is_selected(flat_idx)) {
            return nullptr;
        }

        return reinterpret_cast<std::atomic<T>*>(ptr);
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_ATOMIC_VIEW_H