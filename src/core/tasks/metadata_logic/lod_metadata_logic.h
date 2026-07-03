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

/**
 * LODMetadataLogic<T, N>
 * Maps element values (like distance to camera) to a Level of Detail (LOD) tier.
 */
template <typename T, size_t N = 1>
struct LODMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY_NUMERIC | DataType::ANY_VECTOR3;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;

    static constexpr size_t transient_workspace_bytes     = 0;

    static constexpr std::string_view display_name = "Level of Detail (LOD)";
    
    struct Mapping {
        T target_value;
        uint8_t lod_level;
    };

    uint32_t target_buffer_id = 0;
    std::array<Mapping, N> mappings;
    uint8_t default_lod = 0;
    float tolerance = 0.0001f;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary default_lod_prop;
        default_lod_prop["name"] = "default_lod";
        default_lod_prop["type"] = godot::Variant::INT;
        props.push_back(default_lod_prop);

        godot::Dictionary mappings_prop;
        mappings_prop["name"] = "mappings";
        mappings_prop["type"] = godot::Variant::ARRAY;
        
        godot::Array struct_props;
        
        godot::Dictionary target_val_prop;
        target_val_prop["name"] = "target_value";
        target_val_prop["type"] = "T"; // Dynamically resolved by RuntimeInspector
        struct_props.push_back(target_val_prop);

        godot::Dictionary lod_level_prop;
        lod_level_prop["name"] = "lod_level";
        lod_level_prop["type"] = godot::Variant::INT;
        struct_props.push_back(lod_level_prop);

        mappings_prop["struct_properties"] = struct_props;
        props.push_back(mappings_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("default_lod")) {
            default_lod = static_cast<uint8_t>(static_cast<int64_t>(p_props["default_lod"]));
        }
        if (p_props.has("mappings")) {
            godot::Array arr = p_props["mappings"];
            size_t elements_to_copy = std::min(static_cast<size_t>(arr.size()), N);
            for (size_t i = 0; i < elements_to_copy; ++i) {
                godot::Dictionary element = arr[i];
                if (element.has("target_value")) {
                    mappings[i].target_value = static_cast<T>(element["target_value"]);
                }
                if (element.has("lod_level")) {
                    mappings[i].lod_level = static_cast<uint8_t>(static_cast<int64_t>(element["lod_level"]));
                }
            }
        }
    }

    template <typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection,
                            const TaskContextPOD& p_context,
                            const T_View& p_view
                            //const T_Strategy& p_strategy,
                            ) const {
        if (!r_selection.lod_levels || r_selection.element_count == 0) return;

        if (r_selection.mode == SelectionMode::DENSE) {
            _dispatch_dense(r_selection, p_view);
        } else if (r_selection.mode == SelectionMode::SPARSE) {
            _dispatch_sparse(r_selection, p_view);
        } else if (r_selection.mode == SelectionMode::RANGE) {
            _dispatch_range(r_selection, p_view);
        }
    }

private:

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

    [[nodiscard]] inline uint8_t _get_lod(const T& p_val) const noexcept {
        if constexpr (N == 1) {
            return _matches(p_val, mappings[0].target_value) ? mappings[0].lod_level : default_lod;
        } else {
            for (size_t i = 0; i < N; ++i) {
                if (_matches(p_val, mappings[i].target_value)) return mappings[i].lod_level;
            }
            return default_lod;
        }
    }

    template <typename T_View>
    inline void _dispatch_dense(MemoryBufferSelectionPOD& r_sel, T_View& p_view) const {
        const uint64_t* bitset = r_sel.data.bitset;
        uint8_t* lods = r_sel.lod_levels;
        const int64_t cap = r_sel.capacity;

        for (int64_t i = 0; i < cap; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                lods[i] = _get_lod(_read_view(p_view, i));
            }
        }
    }

    template <typename T_View>
    inline void _dispatch_sparse(MemoryBufferSelectionPOD& r_sel, T_View& p_view) const {
        const int64_t* indices = r_sel.data.indices;
        uint8_t* lods = r_sel.lod_levels;
        const int64_t count = r_sel.element_count;

        for (int64_t i = 0; i < count; ++i) {
            const int64_t idx = indices[i];
            lods[idx] = _get_lod(_read_view(p_view, idx));
        }
    }

    template <typename T_View>
    inline void _dispatch_range(MemoryBufferSelectionPOD& r_sel, T_View& p_view) const {
        uint8_t* lods = r_sel.lod_levels;
        const int64_t end = r_sel.start_index + r_sel.element_count;

        for (int64_t i = r_sel.start_index; i < end; ++i) {
            lods[i] = _get_lod(_read_view(p_view, i));
        }
    }
};

} // namespace ideam::core