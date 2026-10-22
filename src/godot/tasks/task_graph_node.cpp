#include "task_graph_node.h"
#include "task_graph_resource.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

using namespace godot;

namespace ideam::godot_ext {

TaskGraphNode::TaskGraphNode() {
}

void TaskGraphNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_task_node_resource"), &TaskGraphNode::get_task_node_resource);
    ClassDB::bind_method(D_METHOD("get_task_type"), &TaskGraphNode::get_task_type);
    ClassDB::bind_method(D_METHOD("get_task_name"), &TaskGraphNode::get_task_name);
    
    ClassDB::bind_method(D_METHOD("_on_custom_param_changed", "param_name", "value"), &TaskGraphNode::_on_custom_param_changed);
    ClassDB::bind_method(D_METHOD("_on_buffer_option_selected", "index", "prop_name", "btn"), &TaskGraphNode::_on_buffer_option_selected);
    ClassDB::bind_method(D_METHOD("_on_logic_option_selected", "index"), &TaskGraphNode::_on_logic_option_selected);

    ClassDB::bind_method(D_METHOD("_on_preview_folding_changed", "folded"), &TaskGraphNode::_on_preview_folding_changed);
    ClassDB::bind_method(D_METHOD("update_preview_image", "image"), &TaskGraphNode::update_preview_image);

    ClassDB::bind_method(D_METHOD("_on_preview_size_changed", "value"), &TaskGraphNode::_on_preview_size_changed);
    ClassDB::bind_method(D_METHOD("_on_refresh_button_pressed"), &TaskGraphNode::_on_refresh_button_pressed);
    ClassDB::bind_method(D_METHOD("request_snapshot"), &TaskGraphNode::request_snapshot);
}

void TaskGraphNode::receive_memory_grant(const godot::Ref<MemoryGrantResource>& p_grant) {
    MemoryGraphNode::receive_memory_grant(p_grant);
    _rebuild_dynamic_ui();
}

Ref<TaskResource> TaskGraphNode::get_task_node_resource() const {
    return Object::cast_to<TaskResource>(get_node_resource().ptr());
}

uint32_t TaskGraphNode::get_task_type() const {
    Ref<TaskResource> res = get_task_node_resource();
    return res.is_valid() ? static_cast<uint32_t>(res->get_task_type()) : 0;
}

StringName TaskGraphNode::get_task_name() const {
    Ref<TaskResource> res = get_task_node_resource();
    return res.is_valid() ? res->get_task_name() : StringName();
}

void TaskGraphNode::_build_ui() {
    MemoryGraphNode::_build_ui();
    set_resizable(true);
    
    task_type_badge = memnew(godot::TextureRect);
    task_type_badge->set_name("TaskTypeBadge");
    add_badge(task_type_badge);

    workspace_badge = memnew(godot::TextureRect);
    workspace_badge->set_name("WorkspaceBadge");
    add_badge(workspace_badge);

    Ref<TaskResource> task_res = get_task_node_resource();
    if (task_res.is_null()) return;

    if (!task_res->is_connected("snapshot_updated", Callable(this, "update_preview_image"))) {
        task_res->connect("snapshot_updated", Callable(this, "update_preview_image"));
    }
    
    // 1. Header Setup
    task_type_label = memnew(Label);
    String label_text = String("Logic: ") + get_task_name();
    task_type_label->set_text(label_text);
    add_child(task_type_label);

    // 2. Collapsible Image Preview Container Setup
    preview_foldable = memnew(FoldableContainer);
    preview_foldable->set_name("ImagePreviewSection");
    preview_foldable->set_title("Preview");
    preview_foldable->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    preview_foldable->set_visible(false);

    // Toolbar for max resolution and manual snapshot trigger
    HBoxContainer* preview_toolbar = memnew(HBoxContainer);
    preview_toolbar->set_name("PreviewToolbar");
    preview_toolbar->set_h_size_flags(Control::SIZE_EXPAND_FILL);

    Label* size_label = memnew(Label);
    size_label->set_text("Max Res:");
    preview_toolbar->add_child(size_label);

    preview_size_spin = memnew(SpinBox);
    preview_size_spin->set_min(64);
    preview_size_spin->set_max(2048);
    preview_size_spin->set_step(64);
    preview_size_spin->set_value(task_res->get_max_preview_size());
    preview_size_spin->set_suffix("px");
    preview_size_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    preview_size_spin->connect("value_changed", Callable(this, "_on_preview_size_changed"));
    preview_toolbar->add_child(preview_size_spin);

    preview_refresh_btn = memnew(Button);
    preview_refresh_btn->set_text("Capture");
    preview_refresh_btn->set_tooltip_text("Queue an image snapshot on next execution");
    preview_refresh_btn->connect("pressed", Callable(this, "_on_refresh_button_pressed"));
    preview_toolbar->add_child(preview_refresh_btn);

    preview_foldable->add_child(preview_toolbar);

    preview_aspect = memnew(AspectRatioContainer);
    preview_aspect->set_name("PreviewAspect");
    preview_aspect->set_ratio(1.0f);
    preview_aspect->set_stretch_mode(AspectRatioContainer::STRETCH_FIT);
    preview_aspect->set_custom_minimum_size(Vector2(128, 128));
    preview_aspect->set_h_size_flags(Control::SIZE_EXPAND_FILL);

    Ref<StyleBoxFlat> preview_bg = memnew(StyleBoxFlat);
    preview_bg->set_bg_color(Color(0.12f, 0.12f, 0.12f, 1.0f));
    preview_bg->set_corner_radius_all(4);
    preview_aspect->add_theme_stylebox_override("panel", preview_bg);

    preview_rect = memnew(TextureRect);
    preview_rect->set_name("PreviewRect");
    preview_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    preview_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    preview_rect->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    preview_rect->set_v_size_flags(Control::SIZE_EXPAND_FILL);

    preview_aspect->add_child(preview_rect);
    preview_foldable->add_child(preview_aspect);
    add_child(preview_foldable);

    int preview_slot = preview_foldable->get_index();
    set_slot_enabled_left(preview_slot, false);
    set_slot_enabled_right(preview_slot, false);

    preview_foldable->connect("folding_changed", Callable(this, "_on_preview_folding_changed"));

    // 3. Dynamic UI Container & Inspector Setup
    custom_parameters_scroll = memnew(ScrollContainer);
    custom_parameters_scroll->set_custom_minimum_size(Vector2(0, 25));
    custom_parameters_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    custom_parameters_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(custom_parameters_scroll);

    custom_parameters_container = memnew(VBoxContainer);
    custom_parameters_container->set_name("CustomParameters");
    custom_parameters_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    custom_parameters_scroll->add_child(custom_parameters_container);

    logic_inspector = memnew(RuntimeInspector);
    logic_inspector->set_name("LogicInspector");
    logic_inspector->set_custom_minimum_size(Vector2(0, 25));
    logic_inspector->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    logic_inspector->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(logic_inspector);
    logic_inspector->connect("property_changed", Callable(this, "_on_custom_param_changed"));

    _rebuild_dynamic_ui();
}

void TaskGraphNode::_notification(int p_what) {
    MemoryGraphNode::_notification(p_what);
}

void TaskGraphNode::_update_theme_properties() {
    MemoryGraphNode::_update_theme_properties();

    if (workspace_badge) {
        workspace_badge->set_texture(_get_badge_icon_for_workspace(workspace_state));
        Color badge_color = get_theme_color(workspace_state == WORKSPACE_ACTIVE ? "transient_active_color" : "transient_error_color", "GraphNode");
        workspace_badge->set_modulate(badge_color); 
        workspace_badge->set_visible(workspace_state != WORKSPACE_HIDDEN);
    }

    for (const auto& binding : buffer_option_bindings) {
        if (binding.button) {
            binding.button->queue_redraw();
        }
    }
}

Ref<Texture2D> TaskGraphNode::_get_badge_icon_for_workspace(TransientWorkspaceState p_state) const {
    StringName type_context = "GraphNode";
    switch (p_state) {
        case WORKSPACE_ACTIVE: return get_theme_icon("badge_transient_active", type_context);
        case WORKSPACE_ERROR:  return get_theme_icon("badge_transient_error", type_context);
        case WORKSPACE_HIDDEN: 
        default:               return Ref<Texture2D>();
    }
}

void TaskGraphNode::set_workspace_state(TransientWorkspaceState p_state) {
    if (workspace_state == p_state) return;
    workspace_state = p_state;
    queue_redraw();
}

void TaskGraphNode::_rebuild_dynamic_ui() {
    if (!custom_parameters_container) return;

    for (int i = 0; i < custom_parameters_container->get_child_count(); ++i) {
        Node* child = custom_parameters_container->get_child(i);
        child->queue_free();
    }

    if (logic_inspector) {
        logic_inspector->clear_inspector();
    }
}

void TaskGraphNode::_reify_property_schema(Array& r_properties, uint32_t p_current_type_id) {
    buffer_option_bindings.clear();
    has_image_pipeline = false;
    active_image_property_name = StringName();
    is_target_image = false;

    Ref<TaskResource> task_res = get_task_node_resource();
    Ref<MemoryGrantResource> grant_res;
    if (task_res.is_valid()) {
        grant_res = task_res->get_memory_grant();
    }

    for (int i = 0; i < r_properties.size(); ++i) {
        Dictionary prop = r_properties[i];

        // Resolve dynamic 'T' types
        if (prop.has("type") && prop["type"].get_type() == Variant::STRING && String(prop["type"]) == "T") {
            if (prop.has("type_hints")) {
                Dictionary hints = prop["type_hints"];
                if (hints.has(p_current_type_id)) {
                    Dictionary concrete_hint = hints[p_current_type_id];
                    prop["type"] = concrete_hint.has("type") ? concrete_hint["type"] : Variant::NIL;
                    if (concrete_hint.has("hint")) prop["hint"] = concrete_hint["hint"];
                    if (concrete_hint.has("hint_string")) prop["hint_string"] = concrete_hint["hint_string"];
                } else {
                    prop["type"] = Variant::NIL; 
                }
                prop.erase("type_hints");
            }
        }

        if (prop.has("struct_properties")) {
            Array sub_schema = prop["struct_properties"];
            _reify_property_schema(sub_schema, p_current_type_id);
            prop["struct_properties"] = sub_schema;
        }

        if (prop.has("port_override")) {
            uint64_t mask = prop["port_override"];
            uint8_t slot          = mask & 0xFF;
            bool override_left    = (mask >> 8) & 1;
            bool enable_left      = (mask >> 9) & 1;
            auto layout_left      = static_cast<core::BufferLayoutType>((mask >> 10) & 0xFFFF);
            bool override_right   = (mask >> 26) & 1;
            bool enable_right     = (mask >> 27) & 1;
            auto layout_right     = static_cast<core::BufferLayoutType>((mask >> 28) & 0xFFFF);

            if (override_left) {
                set_slot_enabled_left(slot, enable_left);
                if (enable_left) update_memory_port(slot, true, BitField<core::BufferLayoutType>(static_cast<int64_t>(layout_left)));
            }
            if (override_right) {
                set_slot_enabled_right(slot, enable_right);
                if (enable_right) update_memory_port(slot, false, BitField<core::BufferLayoutType>(static_cast<int64_t>(layout_right)));
            }
        }

        // --- IMAGE RESOURCE INTERCEPTION ---
        if (prop.has("type") && static_cast<int>(prop["type"]) == Variant::OBJECT) {
            String hint_str = prop.has("hint_string") ? String(prop["hint_string"]) : "";
            if (hint_str == "Texture2D" || hint_str == "Image") {
                String name_str = prop["name"];
                has_image_pipeline = true;
                active_image_property_name = prop["name"];
                is_target_image = name_str.contains("target");
            }
        }

        // --- ENUM / DROPDOWN OVERRIDES ---
        bool consume_property = false;

        if (prop.has("hint") && static_cast<int>(prop["hint"]) == PROPERTY_HINT_ENUM) {
            String hint_str = prop["hint_string"];

            if (hint_str == "buffer_option") {
                StringName prop_name = prop["name"];
                OptionButton* opt_btn = memnew(OptionButton);
                custom_parameters_container->add_child(opt_btn);
                
                BufferOptionBinding binding;
                binding.property_name = prop_name;
                binding.button = opt_btn;

                if (grant_res.is_valid()) {
                    TypedArray<GrantPartResource> parts = grant_res->get_configured_parts();
                    binding.buffer_ids.resize(parts.size());
                    for (int j = 0; j < parts.size(); ++j) {
                        Ref<GrantPartResource> part = parts[j];
                        if (part.is_valid()) {
                            binding.buffer_ids.set(j, part->get_buffer_id());
                        }
                    }
                }

                opt_btn->connect("item_selected", Callable(this, "_on_buffer_option_selected").bind(prop_name, opt_btn));
                buffer_option_bindings.push_back(binding);
                consume_property = true;
            } 
            else if (hint_str == "logic_options") {
                OptionButton* opt_btn = memnew(OptionButton);
                custom_parameters_container->add_child(opt_btn);
                
                PackedStringArray labels = task_res->get_logic_variant_labels();
                for (int j = 0; j < labels.size(); ++j) {
                    opt_btn->add_item(labels[j], j);
                }

                int current_selection = task_res->get_logic_id() - task_res->get_base_logic_id();
                if (current_selection >= 0 && current_selection < opt_btn->get_item_count()) {
                    opt_btn->select(current_selection);
                }

                opt_btn->connect("item_selected", Callable(this, "_on_logic_option_selected"));
                consume_property = true;
            }
        }

        if (consume_property) {
            r_properties.remove_at(i);
            i--; 
        } else {
            r_properties[i] = prop;
        }
    }

    // Configure and synchronize the preview control
    if (preview_foldable) {
        preview_foldable->set_visible(has_image_pipeline);
        if (has_image_pipeline) {
            preview_foldable->set_title(is_target_image ? "Preview (Target Output)" : "Preview (Source Input)");
            if (task_res.is_valid()) {
                preview_foldable->set_folded(task_res->is_preview_folded());
                _update_preview_from_resource();
            }
        } else {
            update_preview_image(Ref<Image>());
        }
    }

    if (!buffer_option_bindings.empty()) {
        emit_signal("buffer_names_requested");
    }
}

void TaskGraphNode::_rebuild_logic_inspector(const Array& p_properties) {
    if (!logic_inspector) return;
    
    Ref<TaskResource> task_res = get_task_node_resource();
    if (task_res.is_null()) return;

    Dictionary state = task_res->get_task_properties();
    
    Variant::Type resolved_t = Variant::NIL;
    uint32_t current_type_id = 0;
    
    if (state.has("type_id")) {
        current_type_id = static_cast<uint32_t>(state["type_id"]);
        resolved_t = static_cast<Variant::Type>(current_type_id);
    }
    
    Array instanced_schema = p_properties.duplicate(true);
    _reify_property_schema(instanced_schema, current_type_id);
    logic_inspector->build_inspector(instanced_schema, state, resolved_t);
}

void TaskGraphNode::_update_matrix_guardrails() {
}

uint64_t TaskGraphNode::_calculate_flat_index() const {
    return 0;
}

void TaskGraphNode::_on_custom_param_changed(const StringName& p_param_name, const Variant& p_value) {
    emit_property_changed(p_param_name, p_value);

    // Keep functional image and live input preview updated
    if (has_image_pipeline && p_param_name == active_image_property_name) {
        Ref<TaskResource> task_res = get_task_node_resource();
        Ref<Texture2D> tex = Object::cast_to<Texture2D>(p_value.operator Object *());

        if (task_res.is_valid()) {
            task_res->set_functional_texture(tex);
        }

        if (!is_target_image && tex.is_valid()) {
            update_preview_image(tex->get_image());
        } else if (!is_target_image) {
            update_preview_image(Ref<Image>());
        }
    }

    _update_matrix_guardrails();
}

void TaskGraphNode::_on_preview_folding_changed(bool p_folded) {
    Ref<TaskResource> task_res = get_task_node_resource();
    if (task_res.is_valid()) {
        task_res->set_preview_folded(p_folded);
        // Automatically queue a snapshot when the user expands the foldout
        if (!p_folded) {
            request_snapshot();
        }
    }
    reset_size();
    queue_redraw();
}

void TaskGraphNode::_on_preview_size_changed(double p_value) {
    Ref<TaskResource> task_res = get_task_node_resource();
    if (task_res.is_valid()) {
        task_res->set_max_preview_size(static_cast<int32_t>(p_value));
        request_snapshot();
    }
}

void TaskGraphNode::_on_refresh_button_pressed() {
    request_snapshot();
}

void TaskGraphNode::request_snapshot() {
    Ref<TaskResource> task_res = get_task_node_resource();
    if (task_res.is_valid()) {
        task_res->request_snapshot();
    }
}

void TaskGraphNode::_update_preview_from_resource() {
    Ref<TaskResource> task_res = get_task_node_resource();
    if (task_res.is_null()) return;

    Ref<Image> target_img = task_res->get_snapshot_image();
    if (target_img.is_null() && !is_target_image) {
        Ref<Texture2D> func_tex = task_res->get_functional_texture();
        if (func_tex.is_valid()) {
            target_img = func_tex->get_image();
        }
    }
    update_preview_image(target_img);
}

void TaskGraphNode::update_preview_image(const Ref<Image>& p_image) {
    if (!preview_rect) return;

    if (p_image.is_null() || p_image->is_empty()) {
        preview_rect->set_texture(Ref<Texture2D>());
        preview_texture.unref();
        return;
    }

    // Zero-allocation path when dimensions and format match
    if (preview_texture.is_valid() &&
        Vector2i(preview_texture->get_size()) == p_image->get_size() &&
        preview_texture->get_format() == p_image->get_format()) {
        preview_texture->update(p_image);
    } else {
        preview_texture = ImageTexture::create_from_image(p_image);
        preview_rect->set_texture(preview_texture);
    }
}

void TaskGraphNode::_on_buffer_option_selected(int p_index, godot::StringName p_prop_name, godot::OptionButton* p_btn) {
    if (!p_btn) return;
    int selected_buffer_id = p_btn->get_item_id(p_index);
    emit_property_changed(p_prop_name, selected_buffer_id);
}

void TaskGraphNode::receive_buffer_names_list(const godot::TypedArray<godot::StringName>& p_names) {
    MemoryGraphNode::receive_buffer_names_list(p_names); 

    for (const auto& binding : buffer_option_bindings) {
        OptionButton* btn = binding.button;
        if (!btn) continue;

        int current_selected_id = btn->get_selected_id();
        btn->clear();

        for (int i = 0; i < binding.buffer_ids.size(); ++i) {
            int b_id = binding.buffer_ids[i];
            String b_name = "Unknown Buffer";
            if (b_id >= 0 && b_id < p_names.size()) {
                b_name = p_names[b_id];
            }
            btn->add_item(b_name, b_id);
        }

        int new_idx = btn->get_item_index(current_selected_id);
        if (new_idx != -1) {
            btn->select(new_idx);
        } else if (btn->get_item_count() > 0) {
            btn->select(0);
        }
    }
}

void TaskGraphNode::_on_logic_option_selected(int p_index) {
    Ref<TaskResource> task_res = get_task_node_resource();
    if (task_res.is_null()) return;

    uint32_t new_id = task_res->get_base_logic_id() + p_index;
    if (task_res->get_logic_id() == new_id) return;

    task_res->set_logic_id(new_id);
    emit_property_changed(StringName("logic_id"), new_id);
    _rebuild_dynamic_ui(); 
}

} // namespace ideam::godot_ext