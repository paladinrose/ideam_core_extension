#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "metadata_logic_traits.h"

#include <type_traits>
#include <cmath>
#include <array>

namespace ideam::core {

enum class GroupMaskOp : uint8_t {
    SET,
    ADD,
    REMOVE
};

/**
 * GroupMaskMetadataLogic<T, N>
 * Evaluates elements against a set of target values and modifies their shadow group_mask.
 */
template <typename T, size_t N = 1>
struct GroupMaskMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr MetadataRequirement requirements = MetadataRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType supported_types = DataType::ANY;
    static constexpr size_t transient_workspace_bytes = 0;

    struct Mapping {
        T target_value;
        uint32_t bit_flag;
    };

    uint32_t target_buffer_id = 0;
    std::array<Mapping, N> mappings;
    GroupMaskOp op = GroupMaskOp::SET;
    float tolerance = 0.0001f;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <typename T_View, typename T_Strategy>
    void execute_metadata(MemoryBufferSelectionPOD& r_selection, const TaskContextPOD& p_context, const T_View& p_view) const {
        if (!r_selection.group_masks || r_selection.element_count == 0) return;

        if (r_selection.mode == SelectionMode::DENSE) {
            _dispatch_dense(r_selection, p_view);
        } else if (r_selection.mode == SelectionMode::SPARSE) {
            _dispatch_sparse(r_selection, p_view);
        } else if (r_selection.mode == SelectionMode::RANGE) {
            _dispatch_range(r_selection, p_view);
        }
    }

private:
    [[nodiscard]] inline bool _matches(const T& p_val, const T& p_target) const noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            return std::abs(p_val - p_target) <= tolerance;
        } else if constexpr (requires { p_val.distance_squared_to(p_target); }) {
            return p_val.distance_squared_to(p_target) <= (tolerance * tolerance);
        } else {
            return p_val == p_target;
        }
    }

    [[nodiscard]] inline uint32_t _get_flags(const T& p_val) const noexcept {
        if constexpr (N == 1) {
            return _matches(p_val, mappings[0].target_value) ? mappings[0].bit_flag : 0;
        } else {
            uint32_t accumulated_flags = 0;
            for (size_t i = 0; i < N; ++i) {
                if (_matches(p_val, mappings[i].target_value)) accumulated_flags |= mappings[i].bit_flag;
            }
            return accumulated_flags;
        }
    }

    template <typename T_View>
    inline void _dispatch_dense(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        switch (op) {
            case GroupMaskOp::SET:    _loop_dense<GroupMaskOp::SET>(r_sel, p_view); break;
            case GroupMaskOp::ADD:    _loop_dense<GroupMaskOp::ADD>(r_sel, p_view); break;
            case GroupMaskOp::REMOVE: _loop_dense<GroupMaskOp::REMOVE>(r_sel, p_view); break;
        }
    }

    template <typename T_View>
    inline void _dispatch_sparse(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        switch (op) {
            case GroupMaskOp::SET:    _loop_sparse<GroupMaskOp::SET>(r_sel, p_view); break;
            case GroupMaskOp::ADD:    _loop_sparse<GroupMaskOp::ADD>(r_sel, p_view); break;
            case GroupMaskOp::REMOVE: _loop_sparse<GroupMaskOp::REMOVE>(r_sel, p_view); break;
        }
    }

    template <typename T_View>
    inline void _dispatch_range(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        switch (op) {
            case GroupMaskOp::SET:    _loop_range<GroupMaskOp::SET>(r_sel, p_view); break;
            case GroupMaskOp::ADD:    _loop_range<GroupMaskOp::ADD>(r_sel, p_view); break;
            case GroupMaskOp::REMOVE: _loop_range<GroupMaskOp::REMOVE>(r_sel, p_view); break;
        }
    }

    template <GroupMaskOp O, typename T_View>
    inline void _loop_dense(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        const uint64_t* bitset = r_sel.data.bitset;
        uint32_t* masks = r_sel.group_masks;
        const int64_t cap = r_sel.capacity;

        for (int64_t i = 0; i < cap; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                const uint32_t flags = _get_flags(p_view[i]);
                if (flags != 0) {
                    if constexpr (O == GroupMaskOp::SET)         masks[i] = flags;
                    else if constexpr (O == GroupMaskOp::ADD)    masks[i] |= flags;
                    else if constexpr (O == GroupMaskOp::REMOVE) masks[i] &= ~flags;
                }
            }
        }
    }

    template <GroupMaskOp O, typename T_View>
    inline void _loop_sparse(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        const int64_t* indices = r_sel.data.indices;
        uint32_t* masks = r_sel.group_masks;
        const int64_t count = r_sel.element_count;

        for (int64_t i = 0; i < count; ++i) {
            const int64_t idx = indices[i];
            const uint32_t flags = _get_flags(p_view[idx]);
            if (flags != 0) {
                if constexpr (O == GroupMaskOp::SET)         masks[idx] = flags;
                else if constexpr (O == GroupMaskOp::ADD)    masks[idx] |= flags;
                else if constexpr (O == GroupMaskOp::REMOVE) masks[idx] &= ~flags;
            }
        }
    }

    template <GroupMaskOp O, typename T_View>
    inline void _loop_range(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        uint32_t* masks = r_sel.group_masks;
        const int64_t end = r_sel.start_index + r_sel.element_count;

        for (int64_t i = r_sel.start_index; i < end; ++i) {
            const uint32_t flags = _get_flags(p_view[i]);
            if (flags != 0) {
                if constexpr (O == GroupMaskOp::SET)         masks[i] = flags;
                else if constexpr (O == GroupMaskOp::ADD)    masks[i] |= flags;
                else if constexpr (O == GroupMaskOp::REMOVE) masks[i] &= ~flags;
            }
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_GROUP_MASK_METADATA_LOGIC_H