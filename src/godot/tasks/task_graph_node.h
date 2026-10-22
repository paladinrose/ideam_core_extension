#pragma once

#include "../memory/memory_graph_node.h"
#include "../controls/runtime_inspector.h"
#include "task_resource.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/foldable_container.hpp>
#include <godot_cpp/classes/aspect_ratio_container.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>

namespace ideam::godot_ext {

struct BufferOptionBinding {
    godot::StringName property_name;
    godot::OptionButton* button = nullptr;
    godot::PackedInt32Array buffer_ids;
};

class TaskGraphNode : public MemoryGraphNode {
    GDCLASS(TaskGraphNode, MemoryGraphNode)

public:
    enum TransientWorkspaceState {
        WORKSPACE_HIDDEN,
        WORKSPACE_ACTIVE,
        WORKSPACE_ERROR
    };

private:
    godot::Label* task_type_label = nullptr;
    TransientWorkspaceState workspace_state = WORKSPACE_HIDDEN;

    godot::Ref<godot::Texture2D> _get_badge_icon_for_workspace(TransientWorkspaceState p_state) const;
    std::vector<BufferOptionBinding> buffer_option_bindings;

    void _on_buffer_option_selected(int p_index, godot::StringName p_prop_name, godot::OptionButton* p_btn);

    // --- Image Preview UI Infrastructure ---
    godot::FoldableContainer* preview_foldable = nullptr;
    godot::AspectRatioContainer* preview_aspect = nullptr;
    godot::TextureRect* preview_rect = nullptr;
    godot::Ref<godot::ImageTexture> preview_texture;

    godot::StringName active_image_property_name;
    bool has_image_pipeline = false;
    bool is_target_image = false;

    godot::SpinBox* preview_size_spin = nullptr;
    godot::Button* preview_refresh_btn = nullptr;

    void _on_preview_size_changed(double p_value);
    void _on_refresh_button_pressed();
    void _on_preview_folding_changed(bool p_folded);
    void _update_preview_from_resource();

protected:
    godot::ScrollContainer* custom_parameters_scroll = nullptr;
    godot::VBoxContainer* custom_parameters_container = nullptr;
    
    RuntimeInspector* logic_inspector = nullptr;
    godot::TextureRect* task_type_badge = nullptr;
    godot::TextureRect* workspace_badge = nullptr;
    
    static void _bind_methods();
    
    virtual void _build_ui() override;
    void _notification(int p_what);
    virtual void _update_theme_properties() override;
    
    virtual void _rebuild_dynamic_ui();
    virtual void _update_matrix_guardrails();
    virtual uint64_t _calculate_flat_index() const;

    void _reify_property_schema(godot::Array& r_properties, uint32_t p_current_type_id);
    void _rebuild_logic_inspector(const godot::Array& p_properties);

    void _on_custom_param_changed(const godot::StringName& p_param_name, const godot::Variant& p_value);
    void _on_logic_option_selected(int p_index);
    
public:
    TaskGraphNode();
    virtual ~TaskGraphNode() override = default;

    virtual void receive_memory_grant(const godot::Ref<MemoryGrantResource>& p_grant) override;
    
    godot::Ref<TaskResource> get_task_node_resource() const;
    uint32_t get_task_type() const;
    godot::StringName get_task_name() const;

    void set_workspace_state(TransientWorkspaceState p_state);
    virtual void receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names) override;

    // Zero-allocation preview texture update
    void update_preview_image(const godot::Ref<godot::Image>& p_image);
    void request_snapshot();
    
};

} // namespace ideam::godot_ext