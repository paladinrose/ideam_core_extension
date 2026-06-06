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

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Flat element comparison
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;

    static constexpr size_t transient_workspace_bytes     = 0;

    struct Mapping {
        T target_value;
        uint32_t bit_flag;
    };

    uint32_t target_buffer_id = 0;
    std::array<Mapping, N> mappings;
    GroupMaskOp op = GroupMaskOp::SET;
    float tolerance = 0.0001f;

    static godot::Array get_ui_properties() {
        godot::Array props;

        // 1. Operation Mode
        godot::Dictionary op_prop;
        op_prop["name"] = "op";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Set,Add,Remove";
        props.push_back(op_prop);

        // 2. Tolerance
        godot::Dictionary tol_prop;
        tol_prop["name"] = "tolerance";
        tol_prop["type"] = godot::Variant::FLOAT;
        tol_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(tol_prop);

        // 3. Mappings
        godot::Dictionary mappings_prop;
        mappings_prop["name"] = "mappings";
        mappings_prop["type"] = godot::Variant::ARRAY;
        
        godot::Array struct_props;
        
        godot::Dictionary target_val_prop;
        target_val_prop["name"] = "target_value";
        target_val_prop["type"] = "T"; // Dynamically resolved by RuntimeInspector
        struct_props.push_back(target_val_prop);

        godot::Dictionary bit_flag_prop;
        bit_flag_prop["name"] = "bit_flag";
        bit_flag_prop["type"] = godot::Variant::INT;
        struct_props.push_back(bit_flag_prop);

        mappings_prop["struct_properties"] = struct_props;
        props.push_back(mappings_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("op")) {
            op = static_cast<GroupMaskOp>(static_cast<uint8_t>(static_cast<int64_t>(p_props["op"])));
        }
        
        if (p_props.has("tolerance")) {
            tolerance = static_cast<float>(p_props["tolerance"]);
        }

        if (p_props.has("mappings")) {
            godot::Array arr = p_props["mappings"];
            size_t elements_to_copy = std::min(static_cast<size_t>(arr.size()), N);
            for (size_t i = 0; i < elements_to_copy; ++i) {
                godot::Dictionary element = arr[i];
                if (element.has("target_value")) {
                    mappings[i].target_value = static_cast<T>(element["target_value"]);
                }
                if (element.has("bit_flag")) {
                    mappings[i].bit_flag = static_cast<uint32_t>(static_cast<int64_t>(element["bit_flag"]));
                }
            }
        }
    }

    template <typename T_View, typename T_Strategy>
    void execute_metadata(MemoryBufferSelectionPOD& r_selection,
                          T_View& p_view,
                          const T_Strategy& p_strategy,
                          const TaskContextPOD& p_context) const {
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

    // --- The DOD View Adapter ---
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T _read_view(T_View& p_view, int64_t idx) const {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else if constexpr (requires { static_cast<T>(p_view[idx]); }) {
            return static_cast<T>(p_view[idx]);
        } else {
            return T{}; 
        }
    }

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
    inline void _dispatch_dense(MemoryBufferSelectionPOD& r_sel, T_View& p_view) const {
        switch (op) {
            case GroupMaskOp::SET:    _loop_dense<GroupMaskOp::SET>(r_sel, p_view); break;
            case GroupMaskOp::ADD:    _loop_dense<GroupMaskOp::ADD>(r_sel, p_view); break;
            case GroupMaskOp::REMOVE: _loop_dense<GroupMaskOp::REMOVE>(r_sel, p_view); break;
        }
    }

    template <typename T_View>
    inline void _dispatch_sparse(MemoryBufferSelectionPOD& r_sel, T_View& p_view) const {
        switch (op) {
            case GroupMaskOp::SET:    _loop_sparse<GroupMaskOp::SET>(r_sel, p_view); break;
            case GroupMaskOp::ADD:    _loop_sparse<GroupMaskOp::ADD>(r_sel, p_view); break;
            case GroupMaskOp::REMOVE: _loop_sparse<GroupMaskOp::REMOVE>(r_sel, p_view); break;
        }
    }

    template <typename T_View>
    inline void _dispatch_range(MemoryBufferSelectionPOD& r_sel, T_View& p_view) const {
        switch (op) {
            case GroupMaskOp::SET:    _loop_range<GroupMaskOp::SET>(r_sel, p_view); break;
            case GroupMaskOp::ADD:    _loop_range<GroupMaskOp::ADD>(r_sel, p_view); break;
            case GroupMaskOp::REMOVE: _loop_range<GroupMaskOp::REMOVE>(r_sel, p_view); break;
        }
    }

    template <GroupMaskOp O, typename T_View>
    inline void _loop_dense(MemoryBufferSelectionPOD& r_sel, T_View& p_view) const {
        const uint64_t* bitset = r_sel.data.bitset;
        uint32_t* masks = r_sel.group_masks;
        const int64_t cap = r_sel.capacity;

        for (int64_t i = 0; i < cap; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                const uint32_t flags = _get_flags(_read_view(p_view, i));
                if (flags != 0) {
                    if constexpr (O == GroupMaskOp::SET)         masks[i] = flags;
                    else if constexpr (O == GroupMaskOp::ADD)    masks[i] |= flags;
                    else if constexpr (O == GroupMaskOp::REMOVE) masks[i] &= ~flags;
                }
            }
        }
    }

    template <GroupMaskOp O, typename T_View>
    inline void _loop_sparse(MemoryBufferSelectionPOD& r_sel, T_View& p_view) const {
        const int64_t* indices = r_sel.data.indices;
        uint32_t* masks = r_sel.group_masks;
        const int64_t count = r_sel.element_count;

        for (int64_t i = 0; i < count; ++i) {
            const int64_t idx = indices[i];
            const uint32_t flags = _get_flags(_read_view(p_view, idx));
            if (flags != 0) {
                if constexpr (O == GroupMaskOp::SET)         masks[idx] = flags;
                else if constexpr (O == GroupMaskOp::ADD)    masks[idx] |= flags;
                else if constexpr (O == GroupMaskOp::REMOVE) masks[idx] &= ~flags;
            }
        }
    }

    template <GroupMaskOp O, typename T_View>
    inline void _loop_range(MemoryBufferSelectionPOD& r_sel, T_View& p_view) const {
        uint32_t* masks = r_sel.group_masks;
        const int64_t end = r_sel.start_index + r_sel.element_count;

        for (int64_t i = r_sel.start_index; i < end; ++i) {
            const uint32_t flags = _get_flags(_read_view(p_view, i));
            if (flags != 0) {
                if constexpr (O == GroupMaskOp::SET)         masks[i] = flags;
                else if constexpr (O == GroupMaskOp::ADD)    masks[i] |= flags;
                else if constexpr (O == GroupMaskOp::REMOVE) masks[i] &= ~flags;
            }
        }
    }
};

} // namespace ideam::core