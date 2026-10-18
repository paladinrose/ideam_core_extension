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
 * ImageChannelExtractTransformLogic<T_Comp>
 * Strides across an interleaved godot::Image, isolating a single component channel
 * (0=R, 1=G, 2=B, 3=A) and blitting it linearly into a destination column/buffer grant.
 * Useful for extracting collision masks, height fields, or simulation inputs from authored assets.
 */
template <typename T_Comp = uint8_t>
struct alignas(64) ImageChannelExtractTransformLogic {
    static_assert(std::is_trivially_copyable_v<T_Comp>, "T_Comp must be trivially copyable for DOD blitting.");

    using ValueType       = T_Comp;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T_Comp, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::SOA | BufferLayoutType::FLAT;
    static constexpr DataType required_types              = DataType::BYTE | DataType::FLOAT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions          = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Image Channel Extract";

    // --- Configuration State (64-byte Cache-Aligned) ---
    uint32_t target_column_buffer_id = INVALID_ID;
    uint32_t target_column_id        = INVALID_ID;
    uint8_t selected_channel         = 0; // 0=R, 1=G, 2=B, 3=A
    uint8_t source_channel_count     = 0; // 0 = Auto-detect from image bytes/dimensions
    uint8_t reserved_internal[6]     = {0};

    godot::Ref<godot::Image> source_image;

    // Explicit tail padding to lock the struct to exactly 64 bytes (1 cache line)
    uint8_t reserved_padding[40] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "target_column_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary ch_prop;
        ch_prop["name"] = "selected_channel";
        ch_prop["type"] = godot::Variant::INT;
        ch_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        ch_prop["hint_string"] = "Red (0),Green (1),Blue (2),Alpha (3)";
        props.push_back(ch_prop);

        godot::Dictionary count_prop;
        count_prop["name"] = "source_channel_count";
        count_prop["type"] = godot::Variant::INT;
        count_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        count_prop["hint_string"] = "0,4,1,Auto (0)";
        props.push_back(count_prop);

        godot::Dictionary img_prop;
        img_prop["name"] = "source_image";
        img_prop["type"] = godot::Variant::OBJECT;
        img_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        img_prop["hint_string"] = "Texture2D";
        props.push_back(img_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_column_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_column_buffer_id")) {
            target_column_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_column_buffer_id"]));
        } else if (p_props.has("buffer_id")) {
            target_column_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("target_column_id")) {
            target_column_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_column_id"]));
        } else if (p_props.has("column_id")) {
            target_column_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["column_id"]));
        }

        if (p_props.has("selected_channel")) {
            selected_channel = static_cast<uint8_t>(static_cast<int64_t>(p_props["selected_channel"]));
        } else if (p_props.has("channel")) {
            selected_channel = static_cast<uint8_t>(static_cast<int64_t>(p_props["channel"]));
        }

        if (p_props.has("source_channel_count")) {
            source_channel_count = static_cast<uint8_t>(static_cast<int64_t>(p_props["source_channel_count"]));
        } else if (p_props.has("channel_count")) {
            source_channel_count = static_cast<uint8_t>(static_cast<int64_t>(p_props["channel_count"]));
        }

        godot::Variant tex_var;
        if (p_props.has("source_image")) {
            tex_var = p_props["source_image"];
        } else if (p_props.has("source_texture")) {
            tex_var = p_props["source_texture"];
        } else if (p_props.has("image")) {
            tex_var = p_props["image"];
        }

        if (tex_var.get_type() == godot::Variant::OBJECT) {
            godot::Ref<godot::Texture2D> tex = tex_var;
            if (tex.is_valid()) {
                source_image = tex->get_image();
            } else {
                // Fallback in case raw Images are passed directly in code/tests
                source_image = tex_var;
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
        if (!source_image.is_valid() || source_image->is_empty()) return;

        const GrantPartPOD* part = nullptr;
        if (p_context.grant) {
            for (uint32_t p = 0; p < p_context.grant->part_count; ++p) {
                const auto& gp = p_context.grant->parts[p];
                if (gp.buffer_id == target_column_buffer_id) {
                    if (target_column_id == INVALID_ID || gp.column_id == target_column_id) {
                        part = &gp;
                        break;
                    }
                }
            }
        }
        if (!part) {
            part = p_context.get_grant_part(target_column_buffer_id);
        }
        if (!part || !part->raw_base_ptr) return;

        const int32_t img_w = source_image->get_width();
        const int32_t img_h = source_image->get_height();
        if (img_w <= 0 || img_h <= 0) return;

        const size_t pixel_count = static_cast<size_t>(img_w) * static_cast<size_t>(img_h);
        const size_t img_bytes = static_cast<size_t>(source_image->get_data_size());
        if (img_bytes == 0) return;

        // Auto-detect channel count from image data size if not explicitly configured
        size_t channels = source_channel_count;
        if (channels == 0) {
            const size_t bytes_per_pixel = img_bytes / pixel_count;
            channels = bytes_per_pixel / sizeof(T_Comp);
            if (channels == 0) channels = 1;
        }

        if (selected_channel >= channels) return;

        const T_Comp* src_raw = reinterpret_cast<const T_Comp*>(source_image->ptr());
        if (!src_raw) return;

        size_t available_elements = part->capacity_bytes / sizeof(T_Comp);
        if (part->selection.mode == SelectionMode::RANGE && part->selection.element_count > 0) {
            available_elements = static_cast<size_t>(part->selection.element_count);
        }

        const size_t count = std::min(pixel_count, available_elements);

        if (part->selection.mode == SelectionMode::SPARSE && part->selection.data.indices) {
            const int64_t sel_count = std::min(static_cast<size_t>(part->selection.element_count), count);
            for (int64_t k = 0; k < sel_count; ++k) {
                const int64_t idx = part->selection.data.indices[k];
                if (idx >= 0 && static_cast<size_t>(idx) < pixel_count) {
                    _write_view(p_view, static_cast<size_t>(k), src_raw[(static_cast<size_t>(idx) * channels) + selected_channel]);
                }
            }
        } else if (part->selection.mode == SelectionMode::DENSE && part->selection.data.bitset) {
            const uint64_t* bitset = part->selection.data.bitset;
            for (size_t i = 0; i < count; ++i) {
                if (bitset[i >> 6] & (1ULL << (i & 63))) {
                    _write_view(p_view, i, src_raw[(i * channels) + selected_channel]);
                }
            }
        } else if (part->selection.mode == SelectionMode::RANGE) {
            const size_t start = static_cast<size_t>(part->selection.start_index);
            const size_t src_offset = (start + count <= pixel_count) ? start : 0;
            for (size_t k = 0; k < count; ++k) {
                _write_view(p_view, static_cast<size_t>(k), src_raw[((src_offset + k) * channels) + selected_channel]);
            }
        } else {
            for (size_t i = 0; i < count; ++i) {
                _write_view(p_view, i, src_raw[(i * channels) + selected_channel]);
            }
        }
    }

    private:
    template <typename T_View, typename T_Val>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _write_view(T_View& p_view, size_t p_index, T_Val p_val) noexcept {
        if constexpr (std::is_pointer_v<std::decay_t<decltype(p_view[p_index])>>) {
            *reinterpret_cast<T_Val*>(p_view[p_index]) = p_val;
        } else {
            p_view[p_index] = p_val;
        }
    }
};

static_assert(sizeof(ImageChannelExtractTransformLogic<>) == 64, "ImageChannelExtractTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core