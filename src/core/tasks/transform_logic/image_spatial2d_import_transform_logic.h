#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/rect2i.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <algorithm>
#include <cstring>

namespace ideam::core {

/**
 * ImageSpatial2DImportTransformLogic<T, T_Strategy>
 * Pitch-aware strided blit from a godot::Image into a 2D spatial MemoryBuffer.
 * Ingests level masks or authored sub-rects into the playable region of a simulation grid.
 */
template <typename T = uint8_t, typename T_Strategy = Spatial2DStrategy>
struct alignas(64) ImageSpatial2DImportTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::SPATIAL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::ANY;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions          = 2;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Image Spatial Import";

    // --- Configuration State ---
    uint32_t buffer_id = INVALID_ID;
    int32_t buffer_width = 0;
    int64_t buffer_stride_y = 0;

    godot::Rect2i image_rect;
    godot::Vector2i buffer_offset;
    godot::Ref<godot::Image> source_image;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary width_prop;
        width_prop["name"] = "buffer_width";
        width_prop["type"] = godot::Variant::INT;
        width_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(width_prop);

        godot::Dictionary stride_prop;
        stride_prop["name"] = "buffer_stride_y";
        stride_prop["type"] = godot::Variant::INT;
        stride_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(stride_prop);

        godot::Dictionary rect_prop;
        rect_prop["name"] = "image_rect";
        rect_prop["type"] = godot::Variant::RECT2I;
        rect_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rect_prop);

        godot::Dictionary offset_prop;
        offset_prop["name"] = "buffer_offset";
        offset_prop["type"] = godot::Variant::VECTOR2I;
        offset_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(offset_prop);

        godot::Dictionary img_prop;
        img_prop["name"] = "source_image";
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
        }
        if (p_props.has("buffer_width")) {
            buffer_width = static_cast<int32_t>(p_props["buffer_width"]);
        }
        if (p_props.has("buffer_stride_y")) {
            buffer_stride_y = static_cast<int64_t>(p_props["buffer_stride_y"]);
        }
        if (p_props.has("image_rect")) {
            image_rect = p_props["image_rect"];
        }
        if (p_props.has("buffer_offset")) {
            buffer_offset = p_props["buffer_offset"];
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

    /**
     * configure_view
     * Automatically invoked by assemble_view to bind spatial scanline stride on the stack.
     */
    template <typename T_View>
    void configure_view(T_View& r_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        if constexpr (requires { r_view.strategy.stride_y; }) {
            const size_t stride = (p_part && p_part->element_stride > 0) ? p_part->element_stride : sizeof(T);
            if (buffer_stride_y > 0) {
                r_view.strategy.stride_y = buffer_stride_y;
            } else if (buffer_width > 0) {
                r_view.strategy.stride_y = static_cast<int64_t>(buffer_width * stride);
            } else if (r_view.strategy.stride_y == 0 && image_rect.size.x > 0) {
                r_view.strategy.stride_y = static_cast<int64_t>((buffer_offset.x + image_rect.size.x) * stride);
            }
        }
    }

    template <typename T_View, typename T_Strat>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if (!source_image.is_valid() || source_image->is_empty()) return;

        const GrantPartPOD* part = p_context.get_grant_part(buffer_id);
        if (!part || !part->raw_base_ptr) return;

        const int32_t img_w = source_image->get_width();
        const int32_t img_h = source_image->get_height();
        if (img_w <= 0 || img_h <= 0) return;

        const size_t img_data_size = static_cast<size_t>(source_image->get_data_size());
        const size_t pixel_bytes = img_data_size / (static_cast<size_t>(img_w) * static_cast<size_t>(img_h));
        if (pixel_bytes == 0) return;

        int32_t img_x = image_rect.position.x;
        int32_t img_y = image_rect.position.y;
        int32_t copy_w = image_rect.size.x;
        int32_t copy_h = image_rect.size.y;
        int32_t buf_x = buffer_offset.x;
        int32_t buf_y = buffer_offset.y;

        if (copy_w <= 0 || copy_h <= 0) return;

        // Clip copy dimensions against source Image boundaries
        if (img_x < 0) {
            buf_x += -img_x;
            copy_w -= -img_x;
            img_x = 0;
        }
        if (img_y < 0) {
            buf_y += -img_y;
            copy_h -= -img_y;
            img_y = 0;
        }
        if (img_x + copy_w > img_w) {
            copy_w = img_w - img_x;
        }
        if (img_y + copy_h > img_h) {
            copy_h = img_h - img_y;
        }
        if (copy_w <= 0 || copy_h <= 0) return;

        // Clip destination buffer coordinates against origin and width bounds
        if (buf_x < 0) {
            img_x += -buf_x;
            copy_w -= -buf_x;
            buf_x = 0;
        }
        if (buf_y < 0) {
            img_y += -buf_y;
            copy_h -= -buf_y;
            buf_y = 0;
        }
        if (buffer_width > 0 && (buf_x + copy_w) > buffer_width) {
            copy_w = buffer_width - buf_x;
        }
        if (copy_w <= 0 || copy_h <= 0) return;

        const int64_t img_pitch = static_cast<int64_t>(img_w * pixel_bytes);
        const uint8_t* src_raw = source_image->ptr();
        if (!src_raw) return;

        const size_t bytes_to_copy = std::min(sizeof(T), pixel_bytes);
        const bool has_dense_mask = (part->selection.mode == SelectionMode::DENSE && part->selection.data.bitset != nullptr);

        for (int32_t r = 0; r < copy_h; ++r) {
            const int64_t cur_img_y = img_y + r;
            const int64_t cur_buf_y = buf_y + r;
            const uint8_t* src_row = src_raw + (cur_img_y * img_pitch);

            for (int32_t c = 0; c < copy_w; ++c) {
                const int64_t cur_img_x = img_x + c;
                const int64_t cur_buf_x = buf_x + c;

                if (has_dense_mask) {
                    int64_t flat_idx = 0;
                    if constexpr (requires { p_view.strategy.get_index_2d(cur_buf_x, cur_buf_y, part->element_stride); }) {
                        flat_idx = p_view.strategy.get_index_2d(cur_buf_x, cur_buf_y, part->element_stride);
                    }
                    if (!part->selection.is_selected(flat_idx)) {
                        continue;
                    }
                }

                const uint8_t* src_pixel = src_row + (cur_img_x * pixel_bytes);
                T val{};

                if constexpr (sizeof(T) == 1) {
                    val = static_cast<T>(*src_pixel);
                } else if constexpr (sizeof(T) == sizeof(uint32_t)) {
                    if (pixel_bytes == sizeof(uint32_t)) {
                        val = *reinterpret_cast<const T*>(src_pixel);
                    } else {
                        std::memcpy(&val, src_pixel, bytes_to_copy);
                    }
                } else {
                    std::memcpy(&val, src_pixel, bytes_to_copy);
                }

                _write_view(p_view, cur_buf_x, cur_buf_y, val);
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
    static inline void _write_view(T_View& p_view, int64_t p_x, int64_t p_y, T_Val p_val) noexcept {
        decltype(auto) target = p_view[p_x, p_y];
        if constexpr (std::is_pointer_v<std::decay_t<decltype(target)>>) {
            *reinterpret_cast<T_Val*>(target) = p_val;
        } else if constexpr (requires { target.store(p_val); }) {
            target.store(p_val);
        } else if constexpr (requires { target.write(); }) {
            static_cast<void>(target.write() = p_val);
        } else {
            static_cast<void>(target = p_val);
        }
    }
};

static_assert(sizeof(ImageSpatial2DImportTransformLogic<>) <= 64, "ImageSpatial2DImportTransformLogic exceeded cache line boundary.");

} // namespace ideam::core