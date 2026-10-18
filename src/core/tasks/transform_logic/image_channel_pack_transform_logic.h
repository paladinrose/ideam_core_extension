#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <algorithm>
#include <cstring>
#include <type_traits>
#include <string_view>

namespace ideam::core {

/**
 * ImageChannelPackTransformLogic<T_Comp, ChannelCount>
 * Interleaves up to 4 separate DOD column/buffer grants into an interleaved godot::Image.
 * Suitable for packing separate simulation streams (e.g., Velocity X, Velocity Y, Pressure)
 * into unified multi-attribute textures (e.g., RGB / RGBA).
 */
template <typename T_Comp = uint8_t, size_t ChannelCount = 4>
struct alignas(64) ImageChannelPackTransformLogic {
    static_assert(ChannelCount >= 1 && ChannelCount <= 4, "ChannelCount must be between 1 and 4.");
    static_assert(std::is_trivially_copyable_v<T_Comp>, "T_Comp must be trivially copyable for DOD blitting.");

    using ValueType       = T_Comp;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T_Comp, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::SOA | BufferLayoutType::FLAT;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::BYTE;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions          = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Image Channel Pack";

    // --- Configuration State (64-byte Cache-Aligned) ---
    uint32_t channel_buffer_ids[4] = {INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID};
    uint32_t channel_column_ids[4] = {INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID};
    godot::Ref<godot::Image> target_image;

    // Explicit tail padding to lock the struct to exactly 64 bytes (1 cache line)
    uint8_t reserved_padding[24] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        for (size_t c = 0; c < ChannelCount; ++c) {
            godot::Dictionary ch_prop;
            ch_prop["name"] = godot::String("channel_") + godot::String::num_int64(c) + godot::String("_buffer_id");
            ch_prop["type"] = godot::Variant::INT;
            ch_prop["hint_string"] = "buffer_option";
            props.push_back(ch_prop);
        }

        godot::Dictionary img_prop;
        img_prop["name"] = "target_image";
        img_prop["type"] = godot::Variant::OBJECT;
        img_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        img_prop["hint_string"] = "Texture2D";
        props.push_back(img_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        for (size_t c = 0; c < ChannelCount; ++c) {
            if (channel_buffer_ids[c] != INVALID_ID) {
                return channel_buffer_ids[c];
            }
        }
        return INVALID_ID;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("channel_buffer_ids")) {
            godot::Array arr = p_props["channel_buffer_ids"];
            for (int i = 0; i < std::min(static_cast<int>(arr.size()), 4); ++i) {
                channel_buffer_ids[i] = static_cast<uint32_t>(static_cast<int64_t>(arr[i]));
            }
        }

        for (size_t c = 0; c < 4; ++c) {
            const godot::String buf_key = godot::String("channel_") + godot::String::num_int64(c) + godot::String("_buffer_id");
            if (p_props.has(buf_key)) {
                channel_buffer_ids[c] = static_cast<uint32_t>(static_cast<int64_t>(p_props[buf_key]));
            }

            const godot::String col_key = godot::String("channel_") + godot::String::num_int64(c) + godot::String("_column_id");
            if (p_props.has(col_key)) {
                channel_column_ids[c] = static_cast<uint32_t>(static_cast<int64_t>(p_props[col_key]));
            }
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
        if (!target_image.is_valid() || target_image->is_empty()) return;

        const int32_t img_w = target_image->get_width();
        const int32_t img_h = target_image->get_height();
        if (img_w <= 0 || img_h <= 0) return;

        const size_t pixel_count = static_cast<size_t>(img_w) * static_cast<size_t>(img_h);
        const size_t img_bytes = static_cast<size_t>(target_image->get_data_size());
        if (img_bytes < pixel_count * ChannelCount * sizeof(T_Comp)) return;

        T_Comp* dst_ptr = reinterpret_cast<T_Comp*>(target_image->ptrw());
        if (!dst_ptr) return;

        // Resolve up to ChannelCount column/buffer grants on the stack
        const GrantPartPOD* parts[ChannelCount] = {nullptr};
        const T_Comp* channel_ptrs[ChannelCount] = {nullptr};
        size_t min_elements = pixel_count;

        for (size_t c = 0; c < ChannelCount; ++c) {
            if (p_context.grant) {
                // 1. Resolve by buffer_id (and column_id if specified)
                if (channel_buffer_ids[c] != INVALID_ID) {
                    for (uint32_t p = 0; p < p_context.grant->part_count; ++p) {
                        const auto& gp = p_context.grant->parts[p];
                        if (gp.buffer_id == channel_buffer_ids[c]) {
                            if (channel_column_ids[c] == INVALID_ID || gp.column_id == channel_column_ids[c]) {
                                parts[c] = &gp;
                                break;
                            }
                        }
                    }
                }
                // 2. Direct part index fallback
                if (!parts[c] && c < p_context.grant->part_count) {
                    parts[c] = &p_context.grant->parts[c];
                }
            }

            if (parts[c] && parts[c]->raw_base_ptr) {
                const T_Comp* base = reinterpret_cast<const T_Comp*>(parts[c]->raw_base_ptr);
                size_t avail = parts[c]->capacity_bytes / sizeof(T_Comp);

                if (parts[c]->selection.mode == SelectionMode::RANGE && parts[c]->selection.element_count > 0) {
                    base += parts[c]->selection.start_index;
                    avail = static_cast<size_t>(parts[c]->selection.element_count);
                }

                channel_ptrs[c] = base;
                min_elements = std::min(min_elements, avail);
            }
        }

        // Default fill value for unmapped channels (e.g. fully opaque Alpha for channel 3)
        auto get_default_component = [](size_t c) noexcept -> T_Comp {
            if constexpr (std::is_floating_point_v<T_Comp>) {
                return (c == 3) ? 1.0f : 0.0f;
            } else {
                return (c == 3) ? static_cast<T_Comp>(255) : static_cast<T_Comp>(0);
            }
        };

        // Tight loop: interleaves separate channel streams into destination pixels
        for (size_t i = 0; i < min_elements; ++i) {
            const size_t pixel_base = i * ChannelCount;
            for (size_t c = 0; c < ChannelCount; ++c) {
                dst_ptr[pixel_base + c] = channel_ptrs[c] ? channel_ptrs[c][i] : get_default_component(c);
            }
        }
    }
};

static_assert(sizeof(ImageChannelPackTransformLogic<>) == 64, "ImageChannelPackTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core