#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/foldable_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/classes/button.hpp>

#include "memory_buffer_resource.h"
#include "memory_manager_resource.h"
#include "managed_buffer_resource.h"
#include "memory_grant_resource.h"
#include "../../core/memory/memory_common.h"

namespace ideam::godot_ext {

class MemoryBufferLinkagePanel : public godot::VBoxContainer {
    GDCLASS(MemoryBufferLinkagePanel, godot::VBoxContainer)

private:
    godot::Ref<MemoryBufferResource> active_buffer;
    godot::Ref<MemoryManagerResource> manager_resource;

    // Outer Collapsible Wrapper
    godot::FoldableContainer* foldable_container = nullptr;
    godot::VBoxContainer* content_box = nullptr;

    // Card A: GPU Compute Shader Bindings
    godot::PanelContainer* gpu_card = nullptr;
    godot::Label* gpu_status_label = nullptr;
    godot::Label* gpu_rid_label = nullptr;
    godot::Label* gpu_binding_label = nullptr;
    godot::Label* gpu_sync_label = nullptr;

    // Card B: Registered Consumers
    godot::PanelContainer* consumers_card = nullptr;
    godot::VBoxContainer* consumers_list = nullptr;
    godot::Label* consumers_count_label = nullptr;

    // Card C: Active Execution Grants & Locks
    godot::PanelContainer* grants_card = nullptr;
    godot::VBoxContainer* grants_list = nullptr;
    godot::Label* grants_count_label = nullptr;

    void _clear_connections();
    void _rebuild_panel();
    void _build_gpu_card();
    void _build_consumers_card();
    void _build_grants_card();

    int _get_active_buffer_id() const;
    godot::PanelContainer* _create_card_shell(const godot::String& p_title, const godot::Color& p_accent_color);

protected:
    static void _bind_methods();

public:
    MemoryBufferLinkagePanel();
    ~MemoryBufferLinkagePanel();

    void setup(const godot::Ref<MemoryBufferResource>& p_buffer, const godot::Ref<MemoryManagerResource>& p_manager = godot::Ref<MemoryManagerResource>());
    void set_buffer_resource(const godot::Ref<MemoryBufferResource>& p_buffer);
    godot::Ref<MemoryBufferResource> get_buffer_resource() const;

    void set_manager_resource(const godot::Ref<MemoryManagerResource>& p_manager);
    godot::Ref<MemoryManagerResource> get_manager_resource() const;

    void set_folded(bool p_folded);
    bool is_folded() const;

    void refresh();
    void _on_resource_changed();
};

} // namespace ideam::godot_ext