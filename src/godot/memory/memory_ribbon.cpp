#include "memory_ribbon.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryRibbon::_bind_methods() {
    ClassDB::bind_method(D_METHOD("sync_with_resource", "manager"), &MemoryRibbon::sync_with_resource);
    ClassDB::bind_method(D_METHOD("set_is_vertical", "vertical"), &MemoryRibbon::set_is_vertical);
    ClassDB::bind_method(D_METHOD("get_is_vertical"), &MemoryRibbon::get_is_vertical);

    ClassDB::bind_method(D_METHOD("highlight_grant_buffers", "active_buffer_ids"), &MemoryRibbon::highlight_grant_buffers);
    ClassDB::bind_method(D_METHOD("clear_dimming"), &MemoryRibbon::clear_dimming);
    ClassDB::bind_method(D_METHOD("set_selected_buffers", "indices"), &MemoryRibbon::set_selected_buffers);
    ClassDB::bind_method(D_METHOD("clear_selection"), &MemoryRibbon::clear_selection);
    ClassDB::bind_method(D_METHOD("set_cut_buffers", "indices"), &MemoryRibbon::set_cut_buffers);
    ClassDB::bind_method(D_METHOD("clear_cut_buffers"), &MemoryRibbon::clear_cut_buffers);

    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &MemoryRibbon::_on_resource_changed);
    ClassDB::bind_method(D_METHOD("_on_block_selected", "buffer_id", "shift_pressed", "ctrl_pressed"), &MemoryRibbon::_on_block_selected);
    ClassDB::bind_method(D_METHOD("_on_block_context_menu_requested", "global_position", "buffer_id"), &MemoryRibbon::_on_block_context_menu_requested);
    ClassDB::bind_method(D_METHOD("_on_block_navigated", "buffer_id", "direction"), &MemoryRibbon::_on_block_navigated);
    ClassDB::bind_method(D_METHOD("_on_block_select_all_requested"), &MemoryRibbon::_on_block_select_all_requested);
    ClassDB::bind_method(D_METHOD("_on_block_invert_selection_requested"), &MemoryRibbon::_on_block_invert_selection_requested);
    ClassDB::bind_method(D_METHOD("_on_block_copy_requested"), &MemoryRibbon::_on_block_copy_requested);
    ClassDB::bind_method(D_METHOD("_on_block_cut_requested"), &MemoryRibbon::_on_block_cut_requested);
    ClassDB::bind_method(D_METHOD("_on_block_paste_requested"), &MemoryRibbon::_on_block_paste_requested);
    ClassDB::bind_method(D_METHOD("_on_block_cancel_requested"), &MemoryRibbon::_on_block_cancel_requested);
    ClassDB::bind_method(D_METHOD("_on_block_drag_initiated", "buffer_id"), &MemoryRibbon::_on_block_drag_initiated);
    ClassDB::bind_method(D_METHOD("_on_block_delete_requested", "bypass_confirm"), &MemoryRibbon::_on_block_delete_requested);
    ClassDB::bind_method(D_METHOD("_on_block_tooltip_requested", "buffer_id", "out_data"), &MemoryRibbon::_on_block_tooltip_requested);

    ClassDB::bind_method(D_METHOD("_on_grant_badge_pressed", "grant_index"), &MemoryRibbon::_on_grant_badge_pressed);
    ClassDB::bind_method(D_METHOD("_on_grant_badge_mouse_entered", "grant_index"), &MemoryRibbon::_on_grant_badge_mouse_entered);
    ClassDB::bind_method(D_METHOD("_on_grant_badge_mouse_exited", "grant_index"), &MemoryRibbon::_on_grant_badge_mouse_exited);

    ClassDB::bind_method(D_METHOD("_on_non_buffer_block_selected", "block_type", "index", "shift_pressed", "ctrl_pressed"), &MemoryRibbon::_on_non_buffer_block_selected);
    ClassDB::bind_method(D_METHOD("_on_non_buffer_block_context_menu_requested", "global_position", "block_type", "index"), &MemoryRibbon::_on_non_buffer_block_context_menu_requested);
    ClassDB::bind_method(D_METHOD("_on_non_buffer_block_tooltip_requested", "block_type", "index", "out_data"), &MemoryRibbon::_on_non_buffer_block_tooltip_requested);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_vertical"), "set_is_vertical", "get_is_vertical");
    
    // Signals
    ADD_SIGNAL(MethodInfo("select_all_requested"));
    ADD_SIGNAL(MethodInfo("invert_selection_requested"));
    ADD_SIGNAL(MethodInfo("copy_requested"));
    ADD_SIGNAL(MethodInfo("cut_requested"));
    ADD_SIGNAL(MethodInfo("paste_requested"));
    ADD_SIGNAL(MethodInfo("cancel_requested"));
    ADD_SIGNAL(MethodInfo("drag_initiated", PropertyInfo(Variant::INT, "buffer_id")));
    ADD_SIGNAL(MethodInfo("drop_paste_requested", PropertyInfo(Variant::INT, "target_index")));
    ADD_SIGNAL(MethodInfo("delete_requested", PropertyInfo(Variant::BOOL, "bypass_confirm")));

    ADD_SIGNAL(MethodInfo("grant_selected", PropertyInfo(Variant::INT, "grant_index")));
    
    ADD_SIGNAL(MethodInfo("inspection_requested", 
        PropertyInfo(Variant::INT, "block_type"), 
        PropertyInfo(Variant::INT, "index"),
        PropertyInfo(Variant::BOOL, "shift_pressed"),
        PropertyInfo(Variant::BOOL, "ctrl_pressed")));

    ADD_SIGNAL(MethodInfo("buffer_context_menu_requested", 
        PropertyInfo(Variant::VECTOR2, "global_position"), 
        PropertyInfo(Variant::INT, "buffer_id")));
}

MemoryRibbon::MemoryRibbon() {
    set_vertical(false);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_mouse_filter(MOUSE_FILTER_STOP);

    drop_spacer = memnew(ColorRect);
    drop_spacer->set_color(Color(0.2f, 0.6f, 1.0f, 1.0f)); // Blue insertion line
    drop_spacer->set_mouse_filter(MOUSE_FILTER_IGNORE);
    drop_spacer->hide();
    add_child(drop_spacer);

}

MemoryRibbon::~MemoryRibbon() {
    _clear_ribbon();
}

void MemoryRibbon::set_is_vertical(bool p_vertical) {
    set_vertical(p_vertical);
}

bool MemoryRibbon::get_is_vertical() const {
    return is_vertical();
}

void MemoryRibbon::sync_with_resource(Ref<MemoryManagerResource> p_manager) {
    if (memory_manager == p_manager) return;

    if (memory_manager.is_valid() && memory_manager->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        memory_manager->disconnect("changed", Callable(this, "_on_resource_changed"));
    }

    memory_manager = p_manager;

    if (memory_manager.is_valid()) {
        memory_manager->connect("changed", Callable(this, "_on_resource_changed"));
    }

    _build_ribbon();
}

void MemoryRibbon::_on_resource_changed() {
    _build_ribbon();
}

void MemoryRibbon::_clear_ribbon() {
    // 1. Disconnect any active buffer schemas
    for (int i = 0; i < connected_schemas.size(); ++i) {
        Ref<MemoryBufferResource> schema = connected_schemas[i];
        if (schema.is_valid() && schema->is_connected("changed", Callable(this, "_on_resource_changed"))) {
            schema->disconnect("changed", Callable(this, "_on_resource_changed"));
        }
    }
    connected_schemas.clear();

    // 2. Clear buttons
    for (int i = get_child_count() - 1; i >= 0; --i) {
        Node* child = get_child(i);
        if (child == drop_spacer) continue; // Do not delete the spacer
        remove_child(child);
        child->queue_free();
    }
}

void MemoryRibbon::_build_ribbon() {
    _clear_ribbon();

    if (!memory_manager.is_valid()) {
        return;
    }

    float total_bytes = static_cast<float>(memory_manager->get_total_projected_footprint_bytes());
    if (total_bytes <= 0.0f) total_bytes = 1.0f;

    const float MIN_STRETCH_RATIO = 0.05f;

    // 1. Map Memory Buffer Resources via MemoryBlockButton
    TypedArray<MemoryBufferResource> schemas = memory_manager->get_buffer_schemas();
    for (int i = 0; i < schemas.size(); ++i) {
        Ref<MemoryBufferResource> schema = schemas[i];
        if (!schema.is_valid()) continue;

        if (!schema->is_connected("changed", Callable(this, "_on_resource_changed"))) {
            schema->connect("changed", Callable(this, "_on_resource_changed"));
        }
        connected_schemas.push_back(schema);

        MemoryBlockButton* block = memnew(MemoryBlockButton);
        block->set_text(schema->get_buffer_name());
        block->set_buffer_id(i);
        block->set_theme_type_variation("MemoryBufferBlock");
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        float buffer_bytes = static_cast<float>(schema->calculate_projected_footprint_bytes());
        float proportion = buffer_bytes / total_bytes;
        block->set_stretch_ratio(MIN_STRETCH_RATIO + proportion);

        // Set non-empty text so Godot's tooltip timer triggers
        block->set_tooltip_text(schema->get_buffer_name());

        // Populate Grant Badges into the button's grant_icon_container
        TypedArray<MemoryGrantResource> active_grants = memory_manager->get_active_emulated_grants();
        for (int g = 0; g < active_grants.size(); ++g) {
            Ref<MemoryGrantResource> grant = active_grants[g];
            if (!grant.is_valid()) {
                continue;
            }

            TypedArray<GrantPartResource> parts = grant->get_configured_parts();
            for (int p = 0; p < parts.size(); ++p) {
                Ref<GrantPartResource> part = parts[p];
                if (part.is_valid() && part->get_buffer_id() == i) {
                    BlockGrantBadge* badge = memnew(BlockGrantBadge);

                    String g_name = grant->get_grant_name().is_empty() 
                        ? (String("Grant ") + String::num_int64(g)) 
                        : String(grant->get_grant_name());
                    String mode_str = (part->get_access_mode() == 1) ? String("WRITE") : String("READ");
                    String tooltip_txt = g_name + String(" [") + mode_str + String("]");

                    if (!grant->is_emulated_valid()) {
                        tooltip_txt += String("\nValidation Warnings:");
                        PackedStringArray errors = grant->get_emulation_errors();
                        for (int e = 0; e < errors.size(); ++e) {
                            tooltip_txt += String("\n - ") + errors[e];
                        }
                    }

                    badge->configure(g, grant->get_grant_name(), part->get_access_mode(), !grant->is_emulated_valid(), tooltip_txt);

                    badge->connect("grant_badge_pressed", Callable(this, "_on_grant_badge_pressed"));
                    badge->connect("grant_badge_mouse_entered", Callable(this, "_on_grant_badge_mouse_entered"));
                    badge->connect("grant_badge_mouse_exited", Callable(this, "_on_grant_badge_mouse_exited"));

                    block->get_grant_icon_container()->add_child(badge);
                }
            }
        }

        // Connect MemoryBlockButton signals
        block->connect("block_selected", Callable(this, "_on_block_selected"));
        block->connect("block_context_menu_requested", Callable(this, "_on_block_context_menu_requested"));
        block->connect("block_navigated", Callable(this, "_on_block_navigated"));
        block->connect("select_all_requested", Callable(this, "_on_block_select_all_requested"));
        block->connect("invert_selection_requested", Callable(this, "_on_block_invert_selection_requested"));
        block->connect("copy_requested", Callable(this, "_on_block_copy_requested"));
        block->connect("cut_requested", Callable(this, "_on_block_cut_requested"));
        block->connect("paste_requested", Callable(this, "_on_block_paste_requested"));
        block->connect("cancel_requested", Callable(this, "_on_block_cancel_requested"));
        block->connect("drag_initiated", Callable(this, "_on_block_drag_initiated"));
        block->connect("delete_requested", Callable(this, "_on_block_delete_requested"));
        block->connect("tooltip_requested", Callable(this, "_on_block_tooltip_requested"));

        if (selected_buffer_ids.has(i)) {
            block->set_selected(true);
        }

        add_child(block);
    }

    // 2. Map Managed Buffer Profiles
    TypedArray<ManagedBufferResource> profiles = memory_manager->get_managed_buffers();
    for (int i = 0; i < profiles.size(); ++i) {
        Ref<ManagedBufferResource> profile = profiles[i];
        if (!profile.is_valid()) continue;

        MemoryRibbonButton* block = memnew(MemoryRibbonButton);
        block->set_block_type(BLOCK_MANAGED);
        block->set_index(i);
        block->set_text(profile->get_consumer_name());
        block->set_theme_type_variation("ManagedBufferBlock");
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        // Required non-empty string to trigger Godot's tooltip timer
        block->set_tooltip_text(profile->get_consumer_name());

        float profile_bytes = static_cast<float>(profile->get_byte_footprint());
        float proportion = profile_bytes / total_bytes;
        block->set_stretch_ratio(MIN_STRETCH_RATIO + proportion);

        block->connect("block_selected", Callable(this, "_on_non_buffer_block_selected"));
        block->connect("block_context_menu_requested", Callable(this, "_on_non_buffer_block_context_menu_requested"));
        block->connect("tooltip_requested", Callable(this, "_on_non_buffer_block_tooltip_requested"));

        add_child(block);
    }

    // 3. Map Transient Capacity
    int transient_mb = memory_manager->get_transient_capacity_mb();
    if (transient_mb > 0) {
        MemoryRibbonButton* block = memnew(MemoryRibbonButton);
        block->set_block_type(BLOCK_TRANSIENT);
        block->set_index(0);
        block->set_text(String("Transient (") + String::num(transient_mb) + " MB)");
        block->set_theme_type_variation("TransientBlock");
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        // Required non-empty string to trigger Godot's tooltip timer
        block->set_tooltip_text("Transient Capacity");

        float transient_bytes = static_cast<float>(transient_mb) * 1024.0f * 1024.0f;
        float proportion = transient_bytes / total_bytes;
        block->set_stretch_ratio(MIN_STRETCH_RATIO + proportion);

        block->connect("block_selected", Callable(this, "_on_non_buffer_block_selected"));
        block->connect("block_context_menu_requested", Callable(this, "_on_non_buffer_block_context_menu_requested"));
        block->connect("tooltip_requested", Callable(this, "_on_non_buffer_block_tooltip_requested"));

        add_child(block);
    }
}

void MemoryRibbon::highlight_grant_buffers(const PackedInt32Array& p_active_buffer_ids) {
    for (int i = 0; i < get_child_count(); ++i) {
        MemoryBlockButton* block = Object::cast_to<MemoryBlockButton>(get_child(i));
        if (block) {
            bool matches = p_active_buffer_ids.has(block->get_buffer_id());
            block->set_dimmed(!matches);
        }
    }
}

void MemoryRibbon::clear_dimming() {
    for (int i = 0; i < get_child_count(); ++i) {
        MemoryBlockButton* block = Object::cast_to<MemoryBlockButton>(get_child(i));
        if (block) {
            block->set_dimmed(false);
        }
    }
}

void MemoryRibbon::set_selected_buffers(const PackedInt32Array& p_indices) {
    selected_buffer_ids = p_indices;
    for (int i = 0; i < get_child_count(); ++i) {
        MemoryBlockButton* block = Object::cast_to<MemoryBlockButton>(get_child(i));
        if (block) {
            block->set_selected(selected_buffer_ids.has(block->get_buffer_id()));
        }
    }
}

void MemoryRibbon::clear_selection() {
    selected_buffer_ids.clear();
    for (int i = 0; i < get_child_count(); ++i) {
        MemoryBlockButton* block = Object::cast_to<MemoryBlockButton>(get_child(i));
        if (block) {
            block->set_selected(false);
        }
    }
}
void MemoryRibbon::set_cut_buffers(const PackedInt32Array& p_indices) {
    for (int i = 0; i < get_child_count(); ++i) {
        MemoryBlockButton* block = Object::cast_to<MemoryBlockButton>(get_child(i));
        if (block) {
            block->set_cut(p_indices.has(block->get_buffer_id()));
        }
    }
}

void MemoryRibbon::clear_cut_buffers() {
    for (int i = 0; i < get_child_count(); ++i) {
        MemoryBlockButton* block = Object::cast_to<MemoryBlockButton>(get_child(i));
        if (block) {
            block->set_cut(false);
        }
    }
}

void MemoryRibbon::_on_block_selected(int p_buffer_id, bool p_shift_pressed, bool p_ctrl_pressed) {
    emit_signal("inspection_requested", BLOCK_BUFFER, p_buffer_id, p_shift_pressed, p_ctrl_pressed);
}

void MemoryRibbon::_on_block_context_menu_requested(const Vector2& p_global_pos, int p_buffer_id) {
    emit_signal("buffer_context_menu_requested", p_global_pos, p_buffer_id);
}

void MemoryRibbon::_on_block_navigated(int p_buffer_id, int p_direction) {
    if (!memory_manager.is_valid()) return;
    
    int schema_count = memory_manager->get_buffer_schemas().size();
    int new_index = p_buffer_id + p_direction;

    if (new_index >= 0 && new_index < schema_count) {
        emit_signal("inspection_requested", BLOCK_BUFFER, new_index, false, false);
        for (int i = 0; i < get_child_count(); ++i) {
            MemoryBlockButton* b = Object::cast_to<MemoryBlockButton>(get_child(i));
            if (b && b->get_buffer_id() == new_index) {
                b->grab_focus();
                break;
            }
        }
    }
}

void MemoryRibbon::_on_block_select_all_requested() {
    emit_signal("select_all_requested");
}

void MemoryRibbon::_on_block_invert_selection_requested() {
    emit_signal("invert_selection_requested");
}

void MemoryRibbon::_on_block_copy_requested() { emit_signal("copy_requested"); }
void MemoryRibbon::_on_block_cut_requested() { emit_signal("cut_requested"); }
void MemoryRibbon::_on_block_paste_requested() { emit_signal("paste_requested"); }
void MemoryRibbon::_on_block_cancel_requested() { emit_signal("cancel_requested"); }
void MemoryRibbon::_on_block_drag_initiated(int p_buffer_id) { emit_signal("drag_initiated", p_buffer_id); }
void MemoryRibbon::_on_block_delete_requested(bool p_bypass_confirm) { emit_signal("delete_requested", p_bypass_confirm); }
void MemoryRibbon::_on_block_tooltip_requested(int p_buffer_id, Dictionary p_out_data) {
    if (!memory_manager.is_valid()) {
        return;
    }

    TypedArray<MemoryBufferResource> schemas = memory_manager->get_buffer_schemas();
    if (p_buffer_id < 0 || p_buffer_id >= schemas.size()) {
        return;
    }

    Ref<MemoryBufferResource> schema = schemas[p_buffer_id];
    if (!schema.is_valid()) {
        return;
    }

    p_out_data["block_type"] = BLOCK_BUFFER;
    p_out_data["buffer_id"] = p_buffer_id;
    p_out_data["buffer_name"] = schema->get_buffer_name();
    p_out_data["max_elements"] = schema->get_max_elements();
    p_out_data["alignment"] = schema->get_alignment();
    p_out_data["is_shadowed"] = schema->get_enable_shadowing();
    p_out_data["columns"] = schema->get_columns();

    int64_t layout_type = schema->get_layout_type();
    switch (layout_type) {
        case 0: p_out_data["layout_type_name"] = "Raw"; break;
        case 1: p_out_data["layout_type_name"] = "AoS"; break;
        case 2: p_out_data["layout_type_name"] = "SoA"; break;
        case 3: p_out_data["layout_type_name"] = "Sparse Set"; break;
        default: p_out_data["layout_type_name"] = "Custom"; break;
    }

    int64_t sel_mode = schema->get_selection_mode();
    switch (sel_mode) {
        case 0: p_out_data["selection_mode_name"] = "Dense (Bitmask)"; break;
        case 1: p_out_data["selection_mode_name"] = "Sparse (ID List)"; break;
        case 2: p_out_data["selection_mode_name"] = "Range (Contiguous)"; break;
        default: p_out_data["selection_mode_name"] = "None"; break;
    }

    int64_t max_elems = schema->get_max_elements();
    TypedArray<Dictionary> cols = schema->get_columns();
    int64_t raw_data_size = 0;
    for (int j = 0; j < cols.size(); ++j) {
        Dictionary d = cols[j];
        int64_t type_size = d.has("size") ? static_cast<int64_t>(d["size"]) : 1;
        raw_data_size += (type_size * max_elems);
    }

    int64_t total_footprint = raw_data_size;
    if (schema->get_enable_shadowing()) {
        int64_t selection_data_size = (sel_mode == 0) ? (((max_elems + 63) / 64) * sizeof(uint64_t)) : (max_elems * sizeof(int64_t));
        int64_t meta_soa_size = (max_elems * sizeof(uint32_t)) + (max_elems * sizeof(int64_t)) + (max_elems * sizeof(uint8_t));
        total_footprint += (selection_data_size + meta_soa_size + 256);
    }
    p_out_data["footprint_bytes"] = total_footprint;

    int readers = 0;
    int writers = 0;
    TypedArray<MemoryGrantResource> grants = memory_manager->get_active_emulated_grants();
    for (int g = 0; g < grants.size(); ++g) {
        Ref<MemoryGrantResource> grant = grants[g];
        if (!grant.is_valid()) continue;

        TypedArray<GrantPartResource> parts = grant->get_configured_parts();
        for (int p = 0; p < parts.size(); ++p) {
            Ref<GrantPartResource> part = parts[p];
            if (part.is_valid() && part->get_buffer_id() == p_buffer_id) {
                if (part->get_access_mode() == 1) {
                    writers++;
                } else {
                    readers++;
                }
            }
        }
    }
    p_out_data["active_readers"] = readers;
    p_out_data["active_writers"] = writers;
}

void MemoryRibbon::_on_grant_badge_pressed(int p_grant_index) {
    emit_signal("grant_selected", p_grant_index);
}

void MemoryRibbon::_on_grant_badge_mouse_entered(int p_grant_index) {
    if (!memory_manager.is_valid()) {
        return;
    }

    TypedArray<MemoryGrantResource> active_grants = memory_manager->get_active_emulated_grants();
    if (p_grant_index >= 0 && p_grant_index < active_grants.size()) {
        Ref<MemoryGrantResource> grant = active_grants[p_grant_index];
        if (grant.is_valid()) {
            highlight_grant_buffers(grant->get_buffer_ids());
        }
    }
}

void MemoryRibbon::_on_grant_badge_mouse_exited(int p_grant_index) {
    clear_dimming();
}

void MemoryRibbon::_on_non_buffer_block_selected(int p_block_type, int p_index, bool p_shift_pressed, bool p_ctrl_pressed) {
    clear_selection();
    emit_signal("inspection_requested", p_block_type, p_index, p_shift_pressed, p_ctrl_pressed);
}

void MemoryRibbon::_on_non_buffer_block_context_menu_requested(const Vector2& p_global_pos, int p_block_type, int p_index) {
    // -1 designates non-buffer items within the ribbon context menu handler
    emit_signal("buffer_context_menu_requested", p_global_pos, -1);
}

void MemoryRibbon::_on_non_buffer_block_tooltip_requested(int p_block_type, int p_index, Dictionary p_out_data) {
    if (!memory_manager.is_valid()) {
        return;
    }

    p_out_data["block_type"] = p_block_type;
    p_out_data["block_index"] = p_index;

    if (p_block_type == BLOCK_MANAGED) {
        TypedArray<ManagedBufferResource> profiles = memory_manager->get_managed_buffers();
        if (p_index < 0 || p_index >= profiles.size()) {
            return;
        }

        Ref<ManagedBufferResource> profile = profiles[p_index];
        if (!profile.is_valid()) {
            return;
        }

        p_out_data["consumer_name"] = String(profile->get_consumer_name());
        p_out_data["purpose"] = String(profile->get_purpose());
        p_out_data["is_live"] = profile->is_live();
        p_out_data["lifecycle_state"] = profile->get_lifecycle_state_string();
        p_out_data["is_vram_dirty"] = profile->is_vram_dirty();
        
        int64_t cap_bytes = profile->get_capacity_bytes();
        p_out_data["footprint_bytes"] = (cap_bytes > 0) ? cap_bytes : static_cast<int64_t>(profile->get_byte_footprint());
        p_out_data["used_bytes"] = profile->get_used_bytes();
        p_out_data["slack_bytes"] = profile->get_slack_bytes();
        p_out_data["fragmentation_pct"] = profile->get_fragmentation_percentage();
        p_out_data["element_count"] = profile->get_element_count();
        p_out_data["max_elements"] = profile->get_max_elements();
        p_out_data["active_readers"] = profile->get_active_reader_count();
        p_out_data["active_writers"] = profile->get_active_writer_count();
        p_out_data["alignment"] = profile->get_alignment();

        int eff_layout = profile->get_effective_layout_type();
        switch (eff_layout) {
            case 0: p_out_data["layout_type_name"] = "Raw / Flat"; break;
            case 1: p_out_data["layout_type_name"] = "AoS"; break;
            case 2: p_out_data["layout_type_name"] = "SoA"; break;
            case 3: p_out_data["layout_type_name"] = "Sparse Set"; break;
            default: p_out_data["layout_type_name"] = "Managed External"; break;
        }

        Ref<MemoryBufferResource> eff_schema = profile->get_effective_schema();
        if (eff_schema.is_valid()) {
            p_out_data["columns"] = eff_schema->get_columns();
            p_out_data["selection_mode_name"] = (eff_schema->get_selection_mode() == 0) ? "Dense (Bitmask)" : "Sparse (ID List)";
            p_out_data["is_shadowed"] = eff_schema->get_enable_shadowing();
        }
    } else if (p_block_type == BLOCK_TRANSIENT) {
        int transient_mb = memory_manager->get_transient_capacity_mb();
        int64_t transient_bytes = static_cast<int64_t>(transient_mb) * 1024 * 1024;
        int64_t total_footprint = static_cast<int64_t>(memory_manager->get_total_projected_footprint_bytes());

        p_out_data["transient_mb"] = transient_mb;
        p_out_data["footprint_bytes"] = transient_bytes;
        p_out_data["total_arena_bytes"] = total_footprint;
        p_out_data["alignment"] = 64;

        int strategy = memory_manager->get_scaling_strategy();
        p_out_data["scaling_strategy"] = (strategy == MemoryManagerResource::STRATEGY_SCALE_BY_RAM) ? "Scale By RAM" : "Fixed";
    }
}

void MemoryRibbon::_notification(int p_what) {
    if (p_what == NOTIFICATION_DRAG_END) {
        if (drop_spacer) drop_spacer->hide();
        // Note: If the user drops outside a valid area, the drag cancels.
        // The items will remain "Cut" (ghosted) until they hit Escape, 
        // matching standard clipboard behavior perfectly!
    }
}

void MemoryRibbon::_gui_input(const Ref<InputEvent>& p_event) {
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed()) {
        if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_RIGHT) {
            // -1 indicates empty ribbon space rather than a specific buffer block
            emit_signal("buffer_context_menu_requested", mb->get_global_position(), -1);
            accept_event();
            return;
        }
    }
    BoxContainer::_gui_input(p_event);
}

// --- Drag & Drop Sequencing ---

bool MemoryRibbon::_can_drop_data(const Vector2& p_at_position, const Variant& p_data) const {
    if (p_data.get_type() != Variant::DICTIONARY) return false;
    Dictionary d = p_data;
    if (!d.has("type") || String(d["type"]) != "memory_buffer_reorder") return false;

    // Calculate target and move spacer visually
    int target_idx = _get_drop_target_index(p_at_position);
    MemoryRibbon* mutable_this = const_cast<MemoryRibbon*>(this);
    
    mutable_this->move_child(mutable_this->drop_spacer, target_idx);
    
    // Scale the line based on flow direction
    if (mutable_this->is_vertical()) {
        mutable_this->drop_spacer->set_custom_minimum_size(Vector2(0, 4));
    } else {
        mutable_this->drop_spacer->set_custom_minimum_size(Vector2(4, 0));
    }
    mutable_this->drop_spacer->show();

    return true;
}

void MemoryRibbon::_drop_data(const Vector2& p_at_position, const Variant& p_data) {
    drop_spacer->hide();
    if (!memory_manager.is_valid() || !_can_drop_data(p_at_position, p_data)) return;

    int target_id = _get_drop_target_index(p_at_position);
    emit_signal("drop_paste_requested", target_id);
}

int MemoryRibbon::_get_drop_target_index(const Vector2& p_at_position) const {
    int buffer_count = 0;
    for (int i = 0; i < get_child_count(); ++i) {
        MemoryBlockButton* block = Object::cast_to<MemoryBlockButton>(get_child(i));
        if (block) {
            Vector2 local_pos = block->get_position();
            Vector2 size = block->get_size();
            
            if (is_vertical()) {
                if (p_at_position.y < local_pos.y + (size.y * 0.5f)) {
                    return buffer_count;
                }
            } else {
                if (p_at_position.x < local_pos.x + (size.x * 0.5f)) {
                    return buffer_count;
                }
            }
            buffer_count++;
        }
    }
    return buffer_count > 0 ? buffer_count - 1 : 0;
}
} // namespace ideam::godot_ext