#pragma once

#include "transform_logic/transform_logic_traits.h"
#include "i_native_task.h"
#include "../memory/views/view_traits.h"

#include <godot_cpp/classes/image.hpp>
#include <new>
#include <atomic>
#include <algorithm>

namespace ideam::core {

template<
    IsTransformLogic T_Logic, 
    typename T_View     = typename T_Logic::DefaultView, 
    typename T_Strategy = typename T_Logic::DefaultStrategy
>
#ifdef __cpp_lib_hardware_interference_size
class alignas(std::hardware_destructive_interference_size) TransformTask final : public INativeTask {
#else
class alignas(64) TransformTask final : public INativeTask {
#endif
    
    static_assert(
        TransformLogicValidator::validate<T_Logic, T_View>(),
        "TransformTask instantiation failed: The selected T_View or T_Strategy does not fulfill the hardware, layout, or type requirements of the T_Logic!"
    );

private:
    T_Logic logic;

    mutable godot::Ref<godot::Image> snapshot_image;
    std::atomic<bool> snapshot_requested{false};
    int32_t max_preview_size = 256;

    void _capture_snapshot(const godot::Ref<godot::Image>& p_src) const {
        if (p_src.is_null() || p_src->is_empty()) {
            snapshot_image.unref();
            return;
        }

        const int32_t src_w = p_src->get_width();
        const int32_t src_h = p_src->get_height();
        int32_t target_w = src_w;
        int32_t target_h = src_h;

        // Calculate clamped dimensions while preserving aspect ratio
        if (max_preview_size > 0 && (src_w > max_preview_size || src_h > max_preview_size)) {
            if (src_w >= src_h) {
                target_w = max_preview_size;
                target_h = std::max(1, (src_h * max_preview_size) / src_w);
            } else {
                target_h = max_preview_size;
                target_w = std::max(1, (src_w * max_preview_size) / src_h);
            }
        }

        // Fast Path: Unscaled / 1:1 match -> Reuse existing buffer with blit_rect
        if (target_w == src_w && target_h == src_h) {
            if (snapshot_image.is_null() ||
                snapshot_image->get_width() != src_w ||
                snapshot_image->get_height() != src_h ||
                snapshot_image->get_format() != p_src->get_format()) {
                snapshot_image = godot::Image::create_empty(src_w, src_h, false, p_src->get_format());
            }
            snapshot_image->blit_rect(p_src, godot::Rect2i(0, 0, src_w, src_h), godot::Vector2i(0, 0));
        } 
        // Clamped Path: Downscale into an isolated preview allocation
        else {
            godot::Ref<godot::Image> downscaled = p_src->duplicate();
            downscaled->resize(target_w, target_h, godot::Image::INTERPOLATE_BILINEAR);
            snapshot_image = downscaled;
        }
    }

public:
    using LogicType    = T_Logic;
    using ViewType     = T_View;
    using StrategyType = T_Strategy;

    TransformTask() = default;

    static constexpr DataType supported_types = T_Logic::required_types & ViewTraits<T_View>::supported_types;

    explicit TransformTask(const T_Logic& p_logic) : logic(p_logic) {}
    virtual ~TransformTask() override = default;

    virtual size_t get_transient_requirement(const TaskContextPOD& p_context) const override {
        if constexpr (requires { logic.get_transient_requirement(p_context); }) {
            return logic.get_transient_requirement(p_context);
        } else if constexpr (requires { T_Logic::transient_workspace_bytes; }) {
            return T_Logic::transient_workspace_bytes;
        }
        return 0;
    }
    
    virtual void cull_selections(const TaskContextPOD& p_context, uint8_t p_dirty_mask) override {}

    virtual void apply_properties(const godot::Dictionary& p_props) override { 
        apply_common_buffer_properties(logic, p_props);
        logic.apply_properties(p_props);
        if (p_props.has("max_preview_size")) {
            max_preview_size = static_cast<int32_t>(p_props["max_preview_size"]);
        }
    }

    virtual void prepare(const TaskContextPOD& p_context) override {
        if constexpr (requires { logic.prepare(p_context); }) {
            logic.prepare(p_context);
        }
    }

    virtual void execute(const TaskContextPOD& p_context) override {
        const uint32_t target_id = logic.get_target_buffer_id();
        const GrantPartPOD* part = p_context.get_grant_part(target_id);
        if (!part) return;
        
        T_View view = assemble_view<T_Logic, T_View>(logic, p_context, part);
        logic.template execute<T_View, T_Strategy>(p_context, view);

        // One-shot capture check: thread-safe exchange clears the request atomically
        if (snapshot_requested.exchange(false, std::memory_order_acq_rel)) {
            if constexpr (requires { logic.target_image; }) {
                _capture_snapshot(logic.target_image);
            } else if constexpr (requires { logic.source_image; }) {
                _capture_snapshot(logic.source_image);
            }
        }
    }

    virtual godot::Ref<godot::Image> get_image_snapshot() const override {
        return snapshot_image;
    }

    virtual void request_snapshot() override {
        snapshot_requested.store(true, std::memory_order_release);
    }

    virtual void set_max_preview_size(int32_t p_size) override {
        max_preview_size = p_size;
    }

    virtual int32_t get_max_preview_size() const override {
        return max_preview_size;
    }

    virtual bool is_snapshot_requested() const override {
        return snapshot_requested.load(std::memory_order_acquire);
    }
};

} // namespace ideam::core