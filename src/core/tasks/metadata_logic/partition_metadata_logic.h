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

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY;
    static constexpr size_t transient_workspace_bytes     = 0;

    struct Mapping {
        T source_value;
        int32_t partition_id;
    };

    uint32_t target_buffer_id = 0;
    std::array<Mapping, N> mappings;
    int32_t default_partition = -1;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary default_part_prop;
        default_part_prop["name"] = "default_partition";
        default_part_prop["type"] = godot::Variant::INT;
        props.push_back(default_part_prop);

        godot::Dictionary mappings_prop;
        mappings_prop["name"] = "mappings";
        mappings_prop["type"] = godot::Variant::ARRAY;
        
        godot::Array struct_props;
        
        godot::Dictionary source_val_prop;
        source_val_prop["name"] = "source_value";
        source_val_prop["type"] = "T"; // Dynamically resolved by RuntimeInspector
        struct_props.push_back(source_val_prop);

        godot::Dictionary part_id_prop;
        part_id_prop["name"] = "partition_id";
        part_id_prop["type"] = godot::Variant::INT;
        struct_props.push_back(part_id_prop);

        mappings_prop["struct_properties"] = struct_props;
        props.push_back(mappings_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("default_partition")) {
            default_partition = p_props["default_partition"];
        }
        if (p_props.has("mappings")) {
            godot::Array arr = p_props["mappings"];
            size_t elements_to_copy = std::min(static_cast<size_t>(arr.size()), N);
            for (size_t i = 0; i < elements_to_copy; ++i) {
                godot::Dictionary element = arr[i];
                if (element.has("source_value")) {
                    mappings[i].source_value = element["source_value"];
                }
                if (element.has("partition_id")) {
                    mappings[i].partition_id = element["partition_id"];
                }
            }
        }
    }

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

    // --- The DOD View Adapter ---
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T _read_view(const T_View& p_view, int64_t idx) const {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else if constexpr (requires { static_cast<T>(p_view[idx]); }) {
            return static_cast<T>(p_view[idx]);
        } else {
            return T{}; 
        }
    }
    
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
                partitions[i] = static_cast<int64_t>(_map_value(_read_view(p_view, i)));
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
            partitions[idx] = static_cast<int64_t>(_map_value(_read_view(p_view, idx)));
        }
    }

    template <typename T_View>
    inline void _dispatch_range(MemoryBufferSelectionPOD& r_sel, const T_View& p_view) const {
        int64_t* partitions = r_sel.partition_ids;
        const int64_t end = r_sel.start_index + r_sel.element_count;

        for (int64_t i = r_sel.start_index; i < end; ++i) {
            partitions[i] = static_cast<int64_t>(_map_value(_read_view(p_view, i)));
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_PARTITION_METADATA_LOGIC_H