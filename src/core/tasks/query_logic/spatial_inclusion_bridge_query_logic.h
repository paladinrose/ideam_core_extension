#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <bit>
#include <cmath>

namespace ideam::core {

/**
 * SpatialInclusionBridgeQueryLogic<T_Coord, T_Strategy>
 * Target: Physical Objects (Entities, Mass). Source: Field/Grid.
 * Evaluates physical objects against an active Grid Selection.
 */
template <typename T_Coord, typename T_Strategy>
struct SpatialInclusionBridgeQueryLogic {
    using ValueType       = T_Coord; 
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SingleElementView<T_Coord, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS | ViewCapability::SPATIAL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY_VECTOR2 | DataType::ANY_VECTOR3 | DataType::VECTOR4D;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;

    static constexpr size_t transient_workspace_bytes     = 0;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    static constexpr std::string_view display_name = "Spatial Inclusion Bridge";
    
    const MemoryBufferSelectionPOD* source_selection = nullptr; // The active Field grid
    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;

    T_Coord grid_origin{};
    T_Coord cell_size{}; 
    uint32_t grid_width = 1;
    uint32_t grid_height = 1;
    uint32_t grid_depth = 1;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary col_prop;
        col_prop["name"] = "column_id";
        col_prop["type"] = godot::Variant::INT;
        col_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(col_prop);
        
        godot::Dictionary origin_prop;
        origin_prop["name"] = "grid_origin";
        origin_prop["type"] = "T_Coord"; 
        origin_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(origin_prop);

        godot::Dictionary cell_prop;
        cell_prop["name"] = "cell_size";
        cell_prop["type"] = "T_Coord"; 
        cell_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cell_prop);

        godot::Dictionary gw_prop;
        gw_prop["name"] = "grid_width";
        gw_prop["type"] = godot::Variant::INT;
        gw_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(gw_prop);

        godot::Dictionary gh_prop;
        gh_prop["name"] = "grid_height";
        gh_prop["type"] = godot::Variant::INT;
        gh_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(gh_prop);

        godot::Dictionary gd_prop;
        gd_prop["name"] = "grid_depth";
        gd_prop["type"] = godot::Variant::INT;
        gd_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(gd_prop);
        
        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("column_id")) {
            column_id = static_cast<uint32_t>(p_props["column_id"]);
        }
        if (p_props.has("grid_origin")) {
            grid_origin = static_cast<T_Coord>(p_props["grid_origin"]);
        }
        if (p_props.has("cell_size")) {
            cell_size = static_cast<T_Coord>(p_props["cell_size"]);
        }
        if (p_props.has("grid_width")) {
            grid_width = static_cast<uint32_t>(p_props["grid_width"]);
        }
        if (p_props.has("grid_height")) {
            grid_height = static_cast<uint32_t>(p_props["grid_height"]);
        }
        if (p_props.has("grid_depth")) {
            grid_depth = static_cast<uint32_t>(p_props["grid_depth"]);
        }
    }
    
    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if (!source_selection || source_selection->mode != SelectionMode::DENSE) return;

        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection, p_view);
            else _cull_sparse(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_view, p_context);
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
    inline T_Coord _read_view(const T_View& p_view, int64_t idx) const {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T_Coord*>(p_view[idx]);
        } else if constexpr (requires { static_cast<T_Coord>(p_view[idx]); }) {
            return static_cast<T_Coord>(p_view[idx]);
        } else {
            return T_Coord{}; 
        }
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(int64_t index, const T_View& p_view) const {
        // CORRECTED: Safe extraction
        T_Coord pos = _read_view(p_view, index);
        const T_Strategy& strategy = p_view.strategy;
        
        int64_t grid_idx = _get_grid_index(pos);
        if (grid_idx < 0 || grid_idx >= source_selection->capacity) return false;

        return (source_selection->data.bitset[grid_idx >> 6] & (1ULL << (grid_idx & 63))) != 0;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(i, p_view)) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t write_ptr = 0;
        int64_t* indices = r_selection.data.indices;
        for (int64_t i = 0; i < r_selection.element_count; ++i) {
            if (_evaluate(indices[i], p_view)) indices[write_ptr++] = indices[i];
        }
        r_selection.element_count = write_ptr;
    }

    template <typename T_View>
    void _add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                if (_evaluate(global_index, p_view)) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                mask &= (mask - 1); 
            }
        }
    }
    
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline int64_t _get_grid_index(const T_Coord& p_pos) const {
        // If T_Coord has a 'z' component, it's 3D.
        if constexpr (requires { p_pos.z; }) { 
            float cx = static_cast<float>(cell_size.x);
            float cy = static_cast<float>(cell_size.y);
            float cz = static_cast<float>(cell_size.z);
            if (std::abs(cx) < 0.0001f || std::abs(cy) < 0.0001f || std::abs(cz) < 0.0001f) return -1;
            
            float lx = static_cast<float>(p_pos.x - grid_origin.x);
            float ly = static_cast<float>(p_pos.y - grid_origin.y);
            float lz = static_cast<float>(p_pos.z - grid_origin.z);
            
            int32_t x = static_cast<int32_t>(std::floor(lx / cx));
            int32_t y = static_cast<int32_t>(std::floor(ly / cy));
            int32_t z = static_cast<int32_t>(std::floor(lz / cz));
            
            if (x < 0 || x >= static_cast<int32_t>(grid_width) || 
                y < 0 || y >= static_cast<int32_t>(grid_height) || 
                z < 0 || z >= static_cast<int32_t>(grid_depth)) return -1;
                
            return x + (y * grid_width) + (z * grid_width * grid_height);
            
        } else { // It's 2D
            float cx = static_cast<float>(cell_size.x);
            float cy = static_cast<float>(cell_size.y);
            if (std::abs(cx) < 0.0001f || std::abs(cy) < 0.0001f) return -1;
            
            float lx = static_cast<float>(p_pos.x - grid_origin.x);
            float ly = static_cast<float>(p_pos.y - grid_origin.y);
            
            int32_t x = static_cast<int32_t>(std::floor(lx / cx));
            int32_t y = static_cast<int32_t>(std::floor(ly / cy));
            
            if (x < 0 || x >= static_cast<int32_t>(grid_width) || 
                y < 0 || y >= static_cast<int32_t>(grid_height)) return -1;
                
            return x + (y * grid_width);
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_SPATIAL_INCLUSION_BRIDGE_QUERY_LOGIC_H