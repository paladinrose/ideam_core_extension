#pragma once

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "memory_manager_resource.h"
#include "memory_block_button.h"

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
    int selected_buffer_index = -1;

    void _clear_ribbon();
    void _build_ribbon();
    int _get_drop_target_index(const godot::Vector2& p_at_position) const;

protected:
    static void _bind_methods();

    // Drag and Drop Virtual Overrides
    virtual bool _can_drop_data(const godot::Vector2& p_at_position, const godot::Variant& p_data) const override;
    virtual void _drop_data(const godot::Vector2& p_at_position, const godot::Variant& p_data) override;

public:
    MemoryRibbon();
    ~MemoryRibbon();

    void sync_with_resource(godot::Ref<MemoryManagerResource> p_manager);
    
    void set_is_vertical(bool p_vertical);
    bool get_is_vertical() const;

    // Visual State Management
    void highlight_grant_buffers(const godot::PackedInt32Array& p_active_buffer_ids);
    void clear_dimming();
    void set_selected_buffer_index(int p_index);
    void clear_selection();

    // Signal Handlers
    void _on_resource_changed();
    void _on_block_selected(int p_buffer_id, bool p_shift_pressed, bool p_ctrl_pressed);
    void _on_block_context_menu_requested(const godot::Vector2& p_global_pos, int p_buffer_id);
    void _on_block_navigated(int p_buffer_id, int p_direction);
    void _on_non_buffer_block_pressed(int p_block_type, int p_index);
};

} // namespace ideam::godot_ext