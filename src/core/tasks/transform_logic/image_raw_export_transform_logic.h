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

namespace ideam::core {

/**
 * ImageRawExportTransformLogic
 * Bulk linear blit from a MemoryBuffer grant into a godot::Image.
 * Suitable for whole-frame copies where buffer layout matches image pixel format.
 */
struct alignas(64) ImageRawExportTransformLogic {
    using ValueType       = uint8_t;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<uint8_t, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::BYTE | DataType::ANY_NUMERIC | DataType::COLOR;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions          = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Image Raw Export";

    // --- Configuration State ---
    uint32_t buffer_id = INVALID_ID;
    godot::Ref<godot::Image> target_image;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

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

        const GrantPartPOD* part = p_context.get_grant_part(buffer_id);
        if (!part || !part->raw_base_ptr) return;

        const size_t image_bytes = static_cast<size_t>(target_image->get_data_size());
        if (image_bytes == 0) return;

        size_t available_bytes = part->capacity_bytes;
        const uint8_t* src_ptr = part->raw_base_ptr;

        if (part->selection.mode == SelectionMode::RANGE && part->selection.element_count > 0) {
            src_ptr += (part->selection.start_index * part->element_stride);
            available_bytes = static_cast<size_t>(part->selection.element_count * part->element_stride);
        }

        const size_t copy_bytes = std::min(image_bytes, available_bytes);
        uint8_t* dst_ptr = target_image->ptrw();

        if (src_ptr && dst_ptr && copy_bytes > 0) {
            std::memcpy(dst_ptr, src_ptr, copy_bytes);
        }
    }
};

static_assert(sizeof(ImageRawExportTransformLogic) <= 64, "ImageRawExportTransformLogic exceeded cache line boundary.");

} // namespace ideam::core