#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

namespace ideam::core {

/**
 * PackedArrayExportTransformLogic
 * Streams data from an internal DOD buffer directly into a mapped Godot PackedArray.
 */
template<typename T>
struct alignas(64) PackedArrayExportTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    
    // The Primary View maps to the Godot PackedArray memory
    using DefaultView     = SingleElementView<ValueType, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    
    // Supports any standard layout type depending on the PackedArray type (Byte, Float32, Vector3, etc.)
    static constexpr DataType required_types              = DataType::ANY; 
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; 
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Packed Array Export";

    // --- Configuration ---
    uint32_t export_target_buffer_id = INVALID_ID; // The mapped Godot PackedArray
    uint32_t source_buffer_id        = INVALID_ID; // The internal DOD simulation buffer

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary source_prop;
        source_prop["name"] = "source_buffer_id";
        source_prop["type"] = godot::Variant::INT;
        source_prop["hint_string"] = "buffer_option";
        props.push_back(source_prop);

        godot::Dictionary target_prop;
        target_prop["name"] = "export_target_buffer_id";
        target_prop["type"] = godot::Variant::INT;
        target_prop["hint_string"] = "buffer_option";
        props.push_back(target_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const {
        return export_target_buffer_id; // Primary view bound to the Godot array
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(p_props["source_buffer_id"]);
        }
        if (p_props.has("export_target_buffer_id")) {
            export_target_buffer_id = static_cast<uint32_t>(p_props["export_target_buffer_id"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD& context, T_View& export_view) const {
        // 1. Resolve Primary Bounds (The mapped Godot Array)
        const GrantPartPOD* export_part = context.get_grant_part(export_target_buffer_id);
        if (!export_part) return;
        const int64_t count = export_part->selection.element_count;

        // 2. Resolve Secondary Buffer (The DOD Source)
        const GrantPartPOD* source_part = context.get_grant_part(source_buffer_id);
        if (!source_part) return;

        // 3. Instantiate Secondary Lightweight View on the stack
        using SourceView = SingleElementView<ValueType, T_Strategy>;
        SourceView source_view;
        source_view.bind(source_part);

        // 4. Hot Loop: Direct memory streaming
        // The SingleElementView abstracts whether this is Flat, SoA, or AoS memory.
        for (int64_t i = 0; i < count; ++i) {
            ValueType val = _read_view<SourceView>(source_view, i);
            _write_view<T_View>(export_view, i, val);
        }
    }

private:
    template <typename V_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline ValueType _read_view(const V_View& p_view, int64_t idx) const {
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<ValueType>(p_view[idx].read());
        } else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<ValueType>(p_view[idx].load());
        } else {
            if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
                return *reinterpret_cast<const ValueType*>(p_view[idx]);
            } else {
                return static_cast<ValueType>(p_view[idx]);
            }
        }
    }

    template <typename V_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void _write_view(const V_View& p_view, int64_t idx, const ValueType& value) const {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            *reinterpret_cast<ValueType*>(p_view[idx]) = value;
        } else if constexpr (requires { p_view[idx].write(); }) {
            p_view[idx] = value;
        } else if constexpr (requires { p_view[idx].store(value); }) {
            p_view[idx].store(value);
        } else {
            (void)(p_view[idx] = value);
        }
    }
};

} // namespace ideam::core