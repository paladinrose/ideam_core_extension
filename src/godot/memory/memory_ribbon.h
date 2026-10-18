#pragma once

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/classes/color_rect.hpp>

#include "memory_manager_resource.h"
#include "memory_ribbon_button.h"
#include "memory_block_button.h"
#include "block_grant_badge.h"

namespace ideam::godot_ext {

class MemoryRibbon : public godot::BoxContainer {
    GDCLASS(MemoryRibbon, godot::BoxContainer)

public:
    enum BlockType {
        BLOCK_BUFFER = 0,
        BLOCK_MANAGED = 1,
        BLOCK_TRANSIENT = 2
    };

private:
    godot::Ref<MemoryManagerResource> memory_manager;
    godot::PackedInt32Array selected_buffer_ids;
    godot::ColorRect* drop_spacer = nullptr;
    godot::TypedArray<MemoryBufferResource> connected_schemas;

    void _clear_ribbon();
    void _build_ribbon();
    int _get_drop_target_index(const godot::Vector2& p_at_position) const;

protected:
    static void _bind_methods();
    
public:
    MemoryRibbon();
    ~MemoryRibbon();

    virtual void _notification(int p_what);

    virtual void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;

    // Drag and Drop Virtual Overrides
    virtual bool _can_drop_data(const godot::Vector2& p_at_position, const godot::Variant& p_data) const override;
    virtual void _drop_data(const godot::Vector2& p_at_position, const godot::Variant& p_data) override;

    void sync_with_resource(godot::Ref<MemoryManagerResource> p_manager);
    
    void set_is_vertical(bool p_vertical);
    bool get_is_vertical() const;

    // Visual State Management
    void highlight_grant_buffers(const godot::PackedInt32Array& p_active_buffer_ids);
    void clear_dimming();
    void set_selected_buffers(const godot::PackedInt32Array& p_indices);
    void clear_selection();

    void set_cut_buffers(const godot::PackedInt32Array& p_indices);
    void clear_cut_buffers();

    // Signal Handlers
    void _on_resource_changed();
    void _on_block_selected(int p_buffer_id, bool p_shift_pressed, bool p_ctrl_pressed);
    void _on_block_context_menu_requested(const godot::Vector2& p_global_pos, int p_buffer_id);
    void _on_block_navigated(int p_buffer_id, int p_direction);
    void _on_block_select_all_requested();
    void _on_block_invert_selection_requested();
    void _on_block_copy_requested();
    void _on_block_cut_requested();
    void _on_block_paste_requested();
    void _on_block_cancel_requested();
    void _on_block_drag_initiated(int p_buffer_id);
    void _on_block_delete_requested(bool p_bypass_confirm);
    void _on_block_tooltip_requested(int p_buffer_id, godot::Dictionary p_out_data);
    
    void _on_grant_badge_pressed(int p_grant_index);
    void _on_grant_badge_mouse_entered(int p_grant_index);
    void _on_grant_badge_mouse_exited(int p_grant_index);

    void _on_non_buffer_block_selected(int p_block_type, int p_index, bool p_shift_pressed, bool p_ctrl_pressed);
    void _on_non_buffer_block_context_menu_requested(const godot::Vector2& p_global_pos, int p_block_type, int p_index);
    void _on_non_buffer_block_tooltip_requested(int p_block_type, int p_index, godot::Dictionary p_out_data);
};

} // namespace ideam::godot_ext