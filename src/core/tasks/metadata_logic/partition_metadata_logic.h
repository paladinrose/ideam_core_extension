#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "metadata_logic_traits.h"

#include <array>
#include <type_traits>

namespace ideam::core {

/**
 * PartitionMetadataLogic<T, N>
 * Maps element values to partition IDs.
 */
template <typename T, size_t N = 1>
struct PartitionMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr MetadataRequirement requirements = MetadataRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType supported_types = DataType::ANY;
    static constexpr size_t transient_workspace_bytes = 0;

    struct Mapping {
        T source_value;
        int32_t partition_id;
    };

    uint32_t target_buffer_id = 0;
    std::array<Mapping, N> mappings;
    int32_t default_partition = -1;

    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    template <typename T_View, typename T_Strategy>
    void execute_metadata(MemoryBufferSelectionPOD& r_selection, const TaskContextPOD& p_context, const T_View& p_view) const {
        if (!r_selection.partition_ids || r_selection.element_count == 0) return;

        if (r_selection.mode == SelectionMode::DENSE) {
            _dispatch_dense(r_selection, p_view);
        } else if (r_selection.mode == SelectionMode::SPARSE) {
            _dispatch_sparse(r_selection, p_view);
        } else if (r_selection.mode == SelectionMode::RANGE) {
            _dispatch_range(r_selection, p_view);
        }
    }

private:
    [[nodiscard]] inline int32_t _map_value(const T& p_val) const noexcept {
        if constexpr (N == 1) {
            return (p_val == mappings[0].source_value) ? mappings[0].partition_id : default_partition;
        } else {
            for (size_t i = 0; i < N; ++i) {
                if (p_val == mappings[i].source_value) return mappings[i].partition_id;
            }
            return default_partition;
        }
    }

    template <typename T_View>
    inline void _dispatch_dense(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        const uint64_t* bitset = r_sel.data.bitset;
        int64_t* partitions = r_sel.partition_ids;
        const int64_t cap = r_sel.capacity;

        for (int64_t i = 0; i < cap; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                partitions[i] = static_cast<int64_t>(_map_value(p_view[i]));
            }
        }
    }

    template <typename T_View>
    inline void _dispatch_sparse(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        const int64_t* indices = r_sel.data.indices;
        int64_t* partitions = r_sel.partition_ids;
        const int64_t count = r_sel.element_count;

        for (int64_t i = 0; i < count; ++i) {
            const int64_t idx = indices[i];
            partitions[idx] = static_cast<int64_t>(_map_value(p_view[idx]));
        }
    }

    template <typename T_View>
    inline void _dispatch_range(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        int64_t* partitions = r_sel.partition_ids;
        const int64_t end = r_sel.start_index + r_sel.element_count;

        for (int64_t i = r_sel.start_index; i < end; ++i) {
            partitions[i] = static_cast<int64_t>(_map_value(p_view[i]));
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_PARTITION_METADATA_LOGIC_H