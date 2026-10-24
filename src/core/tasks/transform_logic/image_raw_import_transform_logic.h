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
 * ImageRawImportTransformLogic
 * Bulk linear blit from a godot::Image into a MemoryBuffer grant.
 * Ingests external initialization textures, masks, or maps directly into DOD storage.
 */
struct alignas(64) ImageRawImportTransformLogic {
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

    static constexpr std::string_view display_name = "Image Raw Import";

    // --- Configuration State ---
    uint32_t buffer_id = INVALID_ID;
    bool one_shot = false;
    mutable bool already_imported = false;
    godot::Ref<godot::Image> source_image;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary img_prop;
        img_prop["name"] = "source_image";
        img_prop["type"] = godot::Variant::OBJECT;
        img_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        img_prop["hint_string"] = "Texture2D";
        props.push_back(img_prop);

        godot::Dictionary oneshot_prop;
        oneshot_prop["name"] = "one_shot";
        oneshot_prop["type"] = godot::Variant::BOOL;
        oneshot_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(oneshot_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("buffer_id")) {
            buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }
        if (p_props.has("one_shot")) {
            one_shot = static_cast<bool>(p_props["one_shot"]);
        }
        already_imported = false;
        
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
        if (one_shot && already_imported) return;
        if (!source_image.is_valid() || source_image->is_empty()) return;

        const GrantPartPOD* part = p_context.get_grant_part(buffer_id);
        if (!part || !part->raw_base_ptr) return;

        const size_t image_bytes = static_cast<size_t>(source_image->get_data_size());
        if (image_bytes == 0) return;

        size_t available_bytes = part->capacity_bytes;
        uint8_t* dst_ptr = part->raw_base_ptr;

        if (part->selection.mode == SelectionMode::RANGE && part->selection.element_count > 0) {
            dst_ptr += (part->selection.start_index * part->element_stride);
            available_bytes = static_cast<size_t>(part->selection.element_count * part->element_stride);
        }

        const size_t copy_bytes = std::min(image_bytes, available_bytes);
        const uint8_t* src_ptr = source_image->ptr();

        if (src_ptr && dst_ptr && copy_bytes > 0) {
            std::memcpy(dst_ptr, src_ptr, copy_bytes);
            already_imported = true;
        }
    }
};

static_assert(sizeof(ImageRawImportTransformLogic) <= 64, "ImageRawImportTransformLogic exceeded cache line boundary.");

} // namespace ideam::core