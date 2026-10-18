#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <algorithm>
#include <cstring>
#include <type_traits>
#include <string_view>

namespace ideam::core {

/**
 * ImagePaletteLutTransformLogic<T_Index, T_Pixel>
 * Maps discrete state indices from a simulation buffer to colors via a palette LUT,
 * blitting the resolved pixels into a godot::Image.
 * Used for decoupling simulation state (such as Conway's Game of Life dead/alive cells)
 * from visual representation.
 */
template <typename T_Index = uint8_t, typename T_Pixel = uint32_t>
struct alignas(64) ImagePaletteLutTransformLogic {
    static_assert(std::is_trivially_copyable_v<T_Pixel>, "T_Pixel must be trivially copyable for raw blitting.");

    using ValueType       = T_Index;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T_Index, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::BOOL | DataType::BYTE | DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions          = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Image Palette LUT";

    // --- Configuration State (64-byte Cache-Aligned) ---
    uint32_t buffer_id           = INVALID_ID;
    uint32_t reserved_internal   = 0;
    godot::Ref<godot::Image> target_image;
    godot::PackedColorArray palette;

    // Explicit tail padding to lock the struct to exactly 64 bytes (1 cache line)
    uint8_t reserved_padding[40] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary pal_prop;
        pal_prop["name"] = "palette";
        pal_prop["type"] = godot::Variant::PACKED_COLOR_ARRAY;
        pal_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(pal_prop);

        godot::Dictionary img_prop;
        img_prop["name"] = "target_image";
        img_prop["type"] = godot::Variant::OBJECT;
        img_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        img_prop["hint_string"] = "Texture2D";
        props.push_back(img_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("buffer_id")) {
            buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        } else if (p_props.has("source_buffer_id")) {
            buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        }

        if (p_props.has("palette")) {
            palette = p_props["palette"];
        }

        godot::Variant tex_var;
        if (p_props.has("target_image")) {
            tex_var = p_props["target_image"];
        } else if (p_props.has("target_texture")) {
            tex_var = p_props["target_texture"];
        } else if (p_props.has("image")) {
            tex_var = p_props["image"];
        }

        if (tex_var.get_type() == godot::Variant::OBJECT) {
            godot::Ref<godot::Texture2D> tex = tex_var;
            if (tex.is_valid()) {
                target_image = tex->get_image();
            } else {
                target_image = tex_var;
            }
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if (!target_image.is_valid() || target_image->is_empty() || palette.is_empty()) return;

        const GrantPartPOD* part = p_context.get_grant_part(buffer_id);
        if (!part || !part->raw_base_ptr) return;

        const int32_t img_w = target_image->get_width();
        const int32_t img_h = target_image->get_height();
        if (img_w <= 0 || img_h <= 0) return;

        const size_t pixel_count = static_cast<size_t>(img_w) * static_cast<size_t>(img_h);
        const size_t img_bytes = static_cast<size_t>(target_image->get_data_size());
        if (img_bytes < pixel_count * sizeof(T_Pixel)) return;

        T_Pixel* dst_pixels = reinterpret_cast<T_Pixel*>(target_image->ptrw());
        if (!dst_pixels) return;

        // Build a cache of up to 256 colors on the stack to bypass Variant/Color overhead in the hot loop
        constexpr size_t MAX_STACK_LUT = 256;
        T_Pixel stack_lut[MAX_STACK_LUT];

        const int64_t pal_size = palette.size();
        const godot::Color* pal_raw = palette.ptr();
        const size_t lut_count = std::min<size_t>(static_cast<size_t>(pal_size), MAX_STACK_LUT);

        for (size_t k = 0; k < lut_count; ++k) {
            if constexpr (std::is_same_v<T_Pixel, uint32_t>) {
                stack_lut[k] = pal_raw[k].to_rgba32();
            } else if constexpr (std::is_same_v<T_Pixel, godot::Color>) {
                stack_lut[k] = pal_raw[k];
            } else if constexpr (std::is_same_v<T_Pixel, uint8_t>) {
                stack_lut[k] = static_cast<uint8_t>(pal_raw[k].get_v() * 255.0f + 0.5f);
            } else {
                stack_lut[k] = static_cast<T_Pixel>(pal_raw[k].to_rgba32());
            }
        }

        auto resolve_pixel = [&](T_Index raw_val) noexcept -> T_Pixel {
            size_t idx = 0;
            if constexpr (std::is_same_v<T_Index, bool>) {
                idx = raw_val ? 1 : 0;
            } else {
                idx = static_cast<size_t>(raw_val);
            }
            if (idx >= lut_count) idx = lut_count - 1;
            return stack_lut[idx];
        };

        size_t available_elements = part->capacity_bytes / sizeof(T_Index);
        if (part->selection.mode == SelectionMode::RANGE && part->selection.element_count > 0) {
            available_elements = static_cast<size_t>(part->selection.element_count);
        }

        const size_t count = std::min(pixel_count, available_elements);

        if (part->selection.mode == SelectionMode::SPARSE && part->selection.data.indices) {
            const int64_t sel_count = std::min(static_cast<size_t>(part->selection.element_count), count);
            for (int64_t k = 0; k < sel_count; ++k) {
                const int64_t idx = part->selection.data.indices[k];
                if (idx >= 0 && static_cast<size_t>(idx) < pixel_count) {
                    dst_pixels[idx] = resolve_pixel(_read_view(p_view, static_cast<size_t>(k)));
                }
            }
        } else if (part->selection.mode == SelectionMode::DENSE && part->selection.data.bitset) {
            const uint64_t* bitset = part->selection.data.bitset;
            for (size_t i = 0; i < count; ++i) {
                if (bitset[i >> 6] & (1ULL << (i & 63))) {
                    dst_pixels[i] = resolve_pixel(_read_view(p_view, i));
                }
            }
        } else if (part->selection.mode == SelectionMode::RANGE) {
            const size_t start = static_cast<size_t>(part->selection.start_index);
            const size_t dst_offset = (start + count <= pixel_count) ? start : 0;
            for (size_t k = 0; k < count; ++k) {
                dst_pixels[dst_offset + k] = resolve_pixel(_read_view(p_view, static_cast<size_t>(k)));
            }
        } else {
            for (size_t i = 0; i < count; ++i) {
                dst_pixels[i] = resolve_pixel(_read_view(p_view, i));
            }
        }
    }

    private:
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline T_Index _read_view(const T_View& p_view, size_t p_index) noexcept {
        if constexpr (std::is_pointer_v<std::decay_t<decltype(p_view[p_index])>>) {
            return *reinterpret_cast<const T_Index*>(p_view[p_index]);
        } else if constexpr (requires { static_cast<T_Index>(p_view[p_index]); }) {
            return static_cast<T_Index>(p_view[p_index]);
        } else {
            return T_Index{};
        }
    }
};

static_assert(sizeof(ImagePaletteLutTransformLogic<>) == 128, "ImagePaletteLutTransformLogic must be exactly 128 bytes!");

} // namespace ideam::core