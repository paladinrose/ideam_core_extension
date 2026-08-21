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

    

    ClassDB::bind_method(D_METHOD("_on_non_buffer_block_pressed", "block_type", "index"), &MemoryRibbon::_on_non_buffer_block_pressed);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_vertical"), "set_is_vertical", "get_is_vertical");
    
    // Signals
    ADD_SIGNAL(MethodInfo("select_all_requested"));
    ADD_SIGNAL(MethodInfo("invert_selection_requested"));
    ADD_SIGNAL(MethodInfo("copy_requested"));
    ADD_SIGNAL(MethodInfo("cut_requested"));
    ADD_SIGNAL(MethodInfo("paste_requested"));
    ADD_SIGNAL(MethodInfo("cancel_requested"));
    
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
}

MemoryRibbon::~MemoryRibbon() {}

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
    for (int i = get_child_count() - 1; i >= 0; --i) {
        Node* child = get_child(i);
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

    // 1. Map Memory Buffer Resources via MemoryBlockButton[cite: 1]
    TypedArray<MemoryBufferResource> schemas = memory_manager->get_buffer_schemas();
    for (int i = 0; i < schemas.size(); ++i) {
        Ref<MemoryBufferResource> schema = schemas[i];
        if (!schema.is_valid()) continue;

        MemoryBlockButton* block = memnew(MemoryBlockButton);
        block->set_text(schema->get_buffer_name());
        block->set_buffer_id(i);
        block->set_theme_type_variation("MemoryBufferBlock");
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        int64_t max_elems = schema->get_max_elements();
        TypedArray<Dictionary> dict_columns = schema->get_columns();
        float raw_data_size = 0.0f;
        
        for (int j = 0; j < dict_columns.size(); ++j) {
            Dictionary dict = dict_columns[j];
            uint32_t type_size = dict.has("size") ? static_cast<uint32_t>(dict["size"]) : 1;
            raw_data_size += (type_size * max_elems);
        }
        
        if (schema->get_enable_shadowing()) {
            raw_data_size *= 2.0f; 
        }

        float proportion = raw_data_size / total_bytes;
        block->set_stretch_ratio(MIN_STRETCH_RATIO + proportion);

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

        Button* block = memnew(Button);
        block->set_text(profile->get_consumer_name());
        block->set_theme_type_variation("ManagedBufferBlock");
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        float profile_bytes = static_cast<float>(profile->get_byte_footprint());
        float proportion = profile_bytes / total_bytes;
        block->set_stretch_ratio(MIN_STRETCH_RATIO + proportion);

        Array args;
        args.append(BLOCK_MANAGED);
        args.append(i);
        block->connect("pressed", Callable(this, "_on_non_buffer_block_pressed").bindv(args));
        
        add_child(block);
    }

    // 3. Map Transient Capacity[cite: 1]
    int transient_mb = memory_manager->get_transient_capacity_mb();
    if (transient_mb > 0) {
        Button* block = memnew(Button);
        block->set_text(String("Transient (") + String::num(transient_mb) + " MB)");
        block->set_theme_type_variation("TransientBlock");
        block->set_h_size_flags(SIZE_EXPAND_FILL);
        block->set_v_size_flags(SIZE_EXPAND_FILL);

        float transient_bytes = static_cast<float>(transient_mb) * 1024.0f * 1024.0f;
        float proportion = transient_bytes / total_bytes;
        block->set_stretch_ratio(MIN_STRETCH_RATIO + proportion);

        Array args;
        args.append(BLOCK_TRANSIENT);
        args.append(0); 
        block->connect("pressed", Callable(this, "_on_non_buffer_block_pressed").bindv(args));
        
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

void MemoryRibbon::_on_non_buffer_block_pressed(int p_block_type, int p_index) {
    clear_selection();
    emit_signal("inspection_requested", p_block_type, p_index, false, false);
}

// --- Drag & Drop Sequencing ---

bool MemoryRibbon::_can_drop_data(const Vector2& p_at_position, const Variant& p_data) const {
    if (p_data.get_type() != Variant::DICTIONARY) return false;
    Dictionary d = p_data;
    return d.has("type") && String(d["type"]) == "memory_buffer_reorder";
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

void MemoryRibbon::_drop_data(const Vector2& p_at_position, const Variant& p_data) {
    if (!memory_manager.is_valid() || !_can_drop_data(p_at_position, p_data)) return;

    Dictionary d = p_data;
    int origin_id = d["origin_buffer_id"];
    int target_id = _get_drop_target_index(p_at_position);

    if (origin_id != target_id) {
        memory_manager->move_buffer(origin_id, target_id);
    }
}

} // namespace ideam::godot_ext