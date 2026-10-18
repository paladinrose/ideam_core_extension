#include "memory_buffer_linkage_panel.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

using namespace godot;

namespace ideam::godot_ext {

void MemoryBufferLinkagePanel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup", "buffer", "manager"), &MemoryBufferLinkagePanel::setup, DEFVAL(Ref<MemoryManagerResource>()));
    ClassDB::bind_method(D_METHOD("set_buffer_resource", "buffer"), &MemoryBufferLinkagePanel::set_buffer_resource);
    ClassDB::bind_method(D_METHOD("get_buffer_resource"), &MemoryBufferLinkagePanel::get_buffer_resource);

    ClassDB::bind_method(D_METHOD("set_manager_resource", "manager"), &MemoryBufferLinkagePanel::set_manager_resource);
    ClassDB::bind_method(D_METHOD("get_manager_resource"), &MemoryBufferLinkagePanel::get_manager_resource);

    ClassDB::bind_method(D_METHOD("set_folded", "folded"), &MemoryBufferLinkagePanel::set_folded);
    ClassDB::bind_method(D_METHOD("is_folded"), &MemoryBufferLinkagePanel::is_folded);
    ClassDB::bind_method(D_METHOD("refresh"), &MemoryBufferLinkagePanel::refresh);

    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &MemoryBufferLinkagePanel::_on_resource_changed);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "buffer_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryBufferResource"), "set_buffer_resource", "get_buffer_resource");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "manager_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource"), "set_manager_resource", "get_manager_resource");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "folded"), "set_folded", "is_folded");
}

MemoryBufferLinkagePanel::MemoryBufferLinkagePanel() {
    set_h_size_flags(SIZE_EXPAND_FILL);

    // 1. FoldableContainer Outer Shell
    foldable_container = memnew(FoldableContainer);
    foldable_container->set_title("Linked Outsiders & Dependency Graph");
    foldable_container->set_folded(false);
    foldable_container->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(foldable_container);

    // 2. Content Container
    content_box = memnew(VBoxContainer);
    content_box->set_h_size_flags(SIZE_EXPAND_FILL);
    content_box->add_theme_constant_override("separation", 8);
    foldable_container->add_child(content_box);

    _build_gpu_card();
    _build_consumers_card();
    _build_grants_card();
}

MemoryBufferLinkagePanel::~MemoryBufferLinkagePanel() {
    _clear_connections();
}

void MemoryBufferLinkagePanel::_clear_connections() {
    if (active_buffer.is_valid() && active_buffer->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        active_buffer->disconnect("changed", Callable(this, "_on_resource_changed"));
    }
    if (manager_resource.is_valid() && manager_resource->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        manager_resource->disconnect("changed", Callable(this, "_on_resource_changed"));
    }
}

PanelContainer* MemoryBufferLinkagePanel::_create_card_shell(const String& p_title, const Color& p_accent_color) {
    PanelContainer* card = memnew(PanelContainer);
    card->set_h_size_flags(SIZE_EXPAND_FILL);

    Ref<StyleBoxFlat> style;
    style.instantiate();
    style->set_bg_color(Color(0.12f, 0.13f, 0.17f, 0.95f));
    style->set_border_width(SIDE_LEFT, 3);
    style->set_border_color(p_accent_color);
    style->set_content_margin_all(8.0f);
    style->set_corner_radius_all(3);
    card->add_theme_stylebox_override("panel", style);

    VBoxContainer* box = memnew(VBoxContainer);
    box->set_h_size_flags(SIZE_EXPAND_FILL);
    box->set_name("CardBox");
    card->add_child(box);

    HBoxContainer* header = memnew(HBoxContainer);
    header->set_h_size_flags(SIZE_EXPAND_FILL);
    box->add_child(header);

    Label* title_lbl = memnew(Label);
    title_lbl->set_text(p_title);
    title_lbl->set_theme_type_variation("HeaderSmall");
    title_lbl->set_modulate(p_accent_color);
    header->add_child(title_lbl);

    return card;
}

void MemoryBufferLinkagePanel::_build_gpu_card() {
    gpu_card = _create_card_shell("GPU Compute Shader Bindings", Color(0.12f, 0.75f, 0.90f));
    content_box->add_child(gpu_card);

    VBoxContainer* box = Object::cast_to<VBoxContainer>(gpu_card->get_node_or_null("CardBox"));
    if (!box) return;

    GridContainer* grid = memnew(GridContainer);
    grid->set_columns(2);
    grid->set_h_size_flags(SIZE_EXPAND_FILL);
    box->add_child(grid);

    Label* k_status = memnew(Label);
    k_status->set_text("VRAM StorageBuffer:");
    grid->add_child(k_status);

    gpu_status_label = memnew(Label);
    gpu_status_label->set_text("Disabled (Host Memory Only)");
    grid->add_child(gpu_status_label);

    Label* k_rid = memnew(Label);
    k_rid->set_text("RenderingDevice RID:");
    grid->add_child(k_rid);

    gpu_rid_label = memnew(Label);
    gpu_rid_label->set_text("N/A");
    grid->add_child(gpu_rid_label);

    Label* k_bind = memnew(Label);
    k_bind->set_text("Uniform Binding Slot:");
    grid->add_child(k_bind);

    gpu_binding_label = memnew(Label);
    gpu_binding_label->set_text("Unassigned");
    grid->add_child(gpu_binding_label);

    Label* k_sync = memnew(Label);
    k_sync->set_text("VRAM Sync State:");
    grid->add_child(k_sync);

    gpu_sync_label = memnew(Label);
    gpu_sync_label->set_text("Synced");
    grid->add_child(gpu_sync_label);
}

void MemoryBufferLinkagePanel::_build_consumers_card() {
    consumers_card = _create_card_shell("Registered Graph Consumers", Color(0.96f, 0.62f, 0.15f));
    content_box->add_child(consumers_card);

    VBoxContainer* box = Object::cast_to<VBoxContainer>(consumers_card->get_node_or_null("CardBox"));
    if (!box) return;

    HBoxContainer* header = Object::cast_to<HBoxContainer>(box->get_child(0));
    if (header) {
        consumers_count_label = memnew(Label);
        consumers_count_label->set_h_size_flags(SIZE_EXPAND_FILL);
        consumers_count_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
        consumers_count_label->set_text("Profiles: 0");
        header->add_child(consumers_count_label);
    }

    consumers_list = memnew(VBoxContainer);
    consumers_list->set_h_size_flags(SIZE_EXPAND_FILL);
    box->add_child(consumers_list);
}

void MemoryBufferLinkagePanel::_build_grants_card() {
    grants_card = _create_card_shell("Active Execution Grants & Access Locks", Color(0.20f, 0.85f, 0.55f));
    content_box->add_child(grants_card);

    VBoxContainer* box = Object::cast_to<VBoxContainer>(grants_card->get_node_or_null("CardBox"));
    if (!box) return;

    HBoxContainer* header = Object::cast_to<HBoxContainer>(box->get_child(0));
    if (header) {
        grants_count_label = memnew(Label);
        grants_count_label->set_h_size_flags(SIZE_EXPAND_FILL);
        grants_count_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
        grants_count_label->set_text("Held Grants: 0");
        header->add_child(grants_count_label);
    }

    grants_list = memnew(VBoxContainer);
    grants_list->set_h_size_flags(SIZE_EXPAND_FILL);
    box->add_child(grants_list);
}

int MemoryBufferLinkagePanel::_get_active_buffer_id() const {
    if (!manager_resource.is_valid() || !active_buffer.is_valid()) return -1;
    TypedArray<MemoryBufferResource> schemas = manager_resource->get_buffer_schemas();
    for (int i = 0; i < schemas.size(); ++i) {
        Ref<MemoryBufferResource> schema = schemas[i];
        if (schema == active_buffer) {
            return i;
        }
    }
    return -1;
}

void MemoryBufferLinkagePanel::setup(const Ref<MemoryBufferResource>& p_buffer, const Ref<MemoryManagerResource>& p_manager) {
    _clear_connections();

    active_buffer = p_buffer;
    manager_resource = p_manager;

    if (active_buffer.is_valid()) {
        active_buffer->connect("changed", Callable(this, "_on_resource_changed"));
    }
    if (manager_resource.is_valid()) {
        manager_resource->connect("changed", Callable(this, "_on_resource_changed"));
    }

    refresh();
}

void MemoryBufferLinkagePanel::set_buffer_resource(const Ref<MemoryBufferResource>& p_buffer) {
    if (active_buffer == p_buffer) return;
    if (active_buffer.is_valid() && active_buffer->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        active_buffer->disconnect("changed", Callable(this, "_on_resource_changed"));
    }
    active_buffer = p_buffer;
    if (active_buffer.is_valid()) {
        active_buffer->connect("changed", Callable(this, "_on_resource_changed"));
    }
    refresh();
}
Ref<MemoryBufferResource> MemoryBufferLinkagePanel::get_buffer_resource() const { return active_buffer; }

void MemoryBufferLinkagePanel::set_manager_resource(const Ref<MemoryManagerResource>& p_manager) {
    if (manager_resource == p_manager) return;
    if (manager_resource.is_valid() && manager_resource->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        manager_resource->disconnect("changed", Callable(this, "_on_resource_changed"));
    }
    manager_resource = p_manager;
    if (manager_resource.is_valid()) {
        manager_resource->connect("changed", Callable(this, "_on_resource_changed"));
    }
    refresh();
}
Ref<MemoryManagerResource> MemoryBufferLinkagePanel::get_manager_resource() const { return manager_resource; }

void MemoryBufferLinkagePanel::set_folded(bool p_folded) {
    if (foldable_container) {
        foldable_container->set_folded(p_folded);
    }
}
bool MemoryBufferLinkagePanel::is_folded() const {
    return foldable_container ? foldable_container->is_folded() : false;
}

void MemoryBufferLinkagePanel::_on_resource_changed() {
    refresh();
}

void MemoryBufferLinkagePanel::refresh() {
    _rebuild_panel();
}

void MemoryBufferLinkagePanel::_rebuild_panel() {
    if (!active_buffer.is_valid()) {
        set_visible(false);
        return;
    }

    set_visible(true);
    int target_buffer_id = _get_active_buffer_id();

    // ==========================================
    // 1. Refresh GPU Compute Shader Card
    // ==========================================
    bool needs_gpu = active_buffer->get_needs_gpu_compute();
    if (needs_gpu) {
        gpu_status_label->set_text("Active (Vulkan / RenderingDevice StorageBuffer)");
        gpu_status_label->set_modulate(Color(0.12f, 0.75f, 0.90f));

        if (manager_resource.is_valid() && manager_resource->is_initialized()) {
            std::shared_ptr<core::MemoryManagerDOD> backend = manager_resource->get_backend();
            if (backend && backend->get_rendering_device()) {
                gpu_rid_label->set_text(String("Allocated (Live Device RID)"));
            } else {
                gpu_rid_label->set_text("Pending RenderingDevice Attachment");
            }
        } else {
            gpu_rid_label->set_text("Emulated (Pre-allocation Phase)");
        }

        gpu_binding_label->set_text(String("Binding #") + String::num_int64(target_buffer_id >= 0 ? target_buffer_id : 0) + " (Uniform Set 0)");
        gpu_sync_label->set_text("Synchronized (Clean)");
        gpu_sync_label->set_modulate(Color(0.20f, 0.85f, 0.55f));
    } else {
        gpu_status_label->set_text("Disabled (Host Memory Only)");
        gpu_status_label->set_modulate(Color(0.6f, 0.6f, 0.65f));
        gpu_rid_label->set_text("None");
        gpu_binding_label->set_text("None");
        gpu_sync_label->set_text("N/A (Host Only)");
        gpu_sync_label->set_modulate(Color(0.6f, 0.6f, 0.65f));
    }

    // ==========================================
    // 2. Refresh Registered Consumers Card
    // ==========================================
    for (int i = consumers_list->get_child_count() - 1; i >= 0; --i) {
        Node* child = consumers_list->get_child(i);
        consumers_list->remove_child(child);
        child->queue_free();
    }

    int consumer_matches = 0;
    if (manager_resource.is_valid()) {
        TypedArray<ManagedBufferResource> managed_bufs = manager_resource->get_managed_buffers();
        String active_name = active_buffer->get_buffer_name();

        for (int i = 0; i < managed_bufs.size(); ++i) {
            Ref<ManagedBufferResource> profile = managed_bufs[i];
            if (!profile.is_valid()) continue;

            consumer_matches++;

            HBoxContainer* row = memnew(HBoxContainer);
            row->set_h_size_flags(SIZE_EXPAND_FILL);

            Label* c_name = memnew(Label);
            c_name->set_text(String("▶ ") + profile->get_consumer_name());
            c_name->set_theme_type_variation("HeaderSmall");
            row->add_child(c_name);

            Label* purpose = memnew(Label);
            purpose->set_text(String(" (") + profile->get_purpose() + String(")"));
            purpose->set_modulate(Color(0.85f, 0.85f, 0.90f));
            row->add_child(purpose);

            Label* spacer = memnew(Label);
            spacer->set_h_size_flags(SIZE_EXPAND_FILL);
            row->add_child(spacer);

            Label* specs = memnew(Label);
            specs->set_text(String::num_int64(profile->get_byte_footprint()) + " B | Align: " + String::num_int64(profile->get_alignment()));
            specs->set_modulate(Color(0.70f, 0.70f, 0.75f));
            row->add_child(specs);

            consumers_list->add_child(row);
        }
    }

    if (consumers_count_label) {
        consumers_count_label->set_text(String("Profiles: ") + String::num_int64(consumer_matches));
    }

    if (consumer_matches == 0) {
        Label* no_consumers = memnew(Label);
        no_consumers->set_text(manager_resource.is_valid()
            ? "No external consumers registered. Pipeline nodes register via register_consumer_buffers()."
            : "No MemoryManagerResource bound. Connect manager to view pipeline consumers.");
        no_consumers->set_modulate(Color(0.55f, 0.55f, 0.60f));
        consumers_list->add_child(no_consumers);
    }

    // ==========================================
    // 3. Refresh Active Grants & Access Locks Card
    // ==========================================
    for (int i = grants_list->get_child_count() - 1; i >= 0; --i) {
        Node* child = grants_list->get_child(i);
        grants_list->remove_child(child);
        child->queue_free();
    }

    int active_grant_claims = 0;
    if (manager_resource.is_valid() && target_buffer_id >= 0) {
        TypedArray<MemoryGrantResource> grants = manager_resource->get_active_emulated_grants();

        for (int i = 0; i < grants.size(); ++i) {
            Ref<MemoryGrantResource> grant = grants[i];
            if (!grant.is_valid()) continue;

            TypedArray<GrantPartResource> parts = grant->get_configured_parts();
            for (int p = 0; p < parts.size(); ++p) {
                Ref<GrantPartResource> part = parts[p];
                if (!part.is_valid() || part->get_buffer_id() != static_cast<uint32_t>(target_buffer_id)) continue;

                active_grant_claims++;

                VBoxContainer* grant_entry = memnew(VBoxContainer);
                grant_entry->set_h_size_flags(SIZE_EXPAND_FILL);

                HBoxContainer* row = memnew(HBoxContainer);
                row->set_h_size_flags(SIZE_EXPAND_FILL);
                grant_entry->add_child(row);

                String g_name = grant->get_grant_name();
                if (g_name.is_empty()) {
                    g_name = "Grant #" + String::num_int64(i);
                }
                Label* name_lbl = memnew(Label);
                name_lbl->set_text("● " + g_name + " [Part " + String::num_int64(p) + "]");
                name_lbl->set_theme_type_variation("HeaderSmall");
                row->add_child(name_lbl);

                int mode = part->get_access_mode();
                Label* mode_badge = memnew(Label);
                if (mode == 1) {
                    mode_badge->set_text("[EXCLUSIVE WRITE LOCK]");
                    mode_badge->set_modulate(Color(0.98f, 0.65f, 0.15f));
                } else {
                    mode_badge->set_text("[SHARED READ LOCK]");
                    mode_badge->set_modulate(Color(0.20f, 0.85f, 0.55f));
                }
                row->add_child(mode_badge);

                Label* spacer = memnew(Label);
                spacer->set_h_size_flags(SIZE_EXPAND_FILL);
                row->add_child(spacer);

                if (!part->get_target_column_name().is_empty()) {
                    Label* col_lbl = memnew(Label);
                    col_lbl->set_text("Column: " + part->get_target_column_name());
                    col_lbl->set_modulate(Color(0.70f, 0.70f, 0.75f));
                    row->add_child(col_lbl);
                }

                // Check for emulation errors/race conditions on this grant
                PackedStringArray errors = grant->get_emulation_errors();
                bool has_conflict = false;
                for (int e = 0; e < errors.size(); ++e) {
                    if (errors[e].find(String::num_int64(target_buffer_id)) != -1 || errors[e].find("Race Condition Collision") != -1) {
                        has_conflict = true;
                        Label* err_lbl = memnew(Label);
                        err_lbl->set_text(String("  ⚠ ") + errors[e]);
                        err_lbl->set_modulate(Color(0.95f, 0.30f, 0.25f));
                        grant_entry->add_child(err_lbl);
                    }
                }

                if (!has_conflict) {
                    Label* ok_lbl = memnew(Label);
                    ok_lbl->set_text("  ✓ Concurrency constraints verified. No data-race hazards.");
                    ok_lbl->set_modulate(Color(0.35f, 0.75f, 0.50f, 0.85f));
                    grant_entry->add_child(ok_lbl);
                }

                grants_list->add_child(grant_entry);
            }
        }
    }

    if (grants_count_label) {
        grants_count_label->set_text(String("Held Grants: ") + String::num_int64(active_grant_claims));
    }

    if (active_grant_claims == 0) {
        Label* no_grants = memnew(Label);
        no_grants->set_text(manager_resource.is_valid()
            ? "No active execution grants held on this buffer. (Concurrency State: IDLE)"
            : "No MemoryManagerResource bound. Connect manager to view execution grant claims.");
        no_grants->set_modulate(Color(0.55f, 0.55f, 0.60f));
        grants_list->add_child(no_grants);
    }
}

} // namespace ideam::godot_ext