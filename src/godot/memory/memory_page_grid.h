#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/templates/vector.hpp>

#include "memory_buffer_resource.h"
#include "../../core/memory/memory_common.h"

namespace ideam::godot_ext {

class MemoryPageGrid : public godot::Control {
    GDCLASS(MemoryPageGrid, godot::Control)

public:
    enum PageState {
        PAGE_STATE_COMMITTED_FULL = 0,
        PAGE_STATE_COMMITTED_PARTIAL = 1,
        PAGE_STATE_UNCOMMITTED = 2
    };

    enum AccessStrategy {
        ACCESS_LINEAR_SCAN = 0,
        ACCESS_RANDOM_SCATTER = 1
    };

    struct VirtualPageEntry {
        int vpn = 0;
        int pfn = -1; // -1 if uncommitted
        PageState state = PAGE_STATE_UNCOMMITTED;
        size_t used_bytes = 0;
        size_t slack_bytes = 0;
        godot::Rect2 rect;
    };

    struct PhysicalFrameEntry {
        int pfn = 0;
        int vpn = -1;
        PageState state = PAGE_STATE_UNCOMMITTED;
        size_t used_bytes = 0;
        size_t slack_bytes = 0;
        godot::Rect2 rect;
    };

private:
    godot::Ref<MemoryBufferResource> active_buffer;

    int page_size = 4096;
    int target_columns = 16;
    bool enforce_square_cells = false;
    AccessStrategy access_strategy = ACCESS_LINEAR_SCAN;
    int virtual_headroom_pages = 2; // Uncommitted virtual address space reserve

    godot::Vector<VirtualPageEntry> virtual_pages;
    godot::Vector<PhysicalFrameEntry> physical_frames;

    godot::Rect2 directory_tier_rect;
    godot::Rect2 physical_tier_rect;
    float routing_channel_y = 0.0f;
    float routing_channel_h = 42.0f;

    // Interactive Hover Tracking
    int hovered_layer = -1; // 0 = Directory (VPN), 1 = Physical (PFN)
    int hovered_vpn = -1;
    int hovered_pfn = -1;

    void _clear_connections();
    void _recalculate_layout();
    void _draw_crosshatch(const godot::Rect2& p_rect, const godot::Color& p_line_color, float p_spacing = 6.0f);
    void _draw_bezier_link(const godot::Vector2& p_start, const godot::Vector2& p_end, const godot::Color& p_color, float p_width = 2.0f);
    void _draw_access_strategy_overlay(const godot::Vector2& p_size);

protected:
    static void _bind_methods();
    void _notification(int p_what);
    
public:
    MemoryPageGrid();
    ~MemoryPageGrid();

    virtual void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    
    void setup_from_resource(godot::Ref<MemoryBufferResource> p_buffer);
    void _on_buffer_changed();

    void set_page_size(int p_size);
    int get_page_size() const;

    void set_target_columns(int p_columns);
    int get_target_columns() const;

    void set_enforce_square_cells(bool p_enforce);
    bool get_enforce_square_cells() const;

    void set_access_strategy(int p_strategy);
    int get_access_strategy() const;

    void set_virtual_headroom_pages(int p_count);
    int get_virtual_headroom_pages() const;

    virtual godot::Vector2 _get_minimum_size() const override;
    virtual godot::String _get_tooltip(const godot::Vector2& p_at_position) const override;
};

} // namespace ideam::godot_ext