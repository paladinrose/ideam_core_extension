#include "memory_inspectors.h"

namespace ideam::godot_ext {

// --- MemorySelectionInspector ---

void MemorySelectionInspector::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_element_count"), &MemorySelectionInspector::get_element_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_buffer_id"), &MemorySelectionInspector::get_target_buffer_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_buffer_version"), &MemorySelectionInspector::get_buffer_version);
    godot::ClassDB::bind_method(godot::D_METHOD("get_selection_mode_string"), &MemorySelectionInspector::get_selection_mode_string);
    godot::ClassDB::bind_method(godot::D_METHOD("is_valid"), &MemorySelectionInspector::is_valid);
}

void MemorySelectionInspector::initialize_from_pod(const core::MemoryBufferSelectionPOD& p_pod) {
    element_count = p_pod.element_count;
    target_buffer_id = p_pod.target_buffer_id;
    buffer_version = p_pod.buffer_version;
    valid = p_pod.is_valid();

    // Map the internal DOD enum to a human-readable string for the UI
    switch (p_pod.mode) {
        case core::SelectionMode::DENSE: selection_mode_name = "Dense (Bitmask)"; break;
        case core::SelectionMode::SPARSE: selection_mode_name = "Sparse (ID List)"; break;
        case core::SelectionMode::RANGE: selection_mode_name = "Range (Contiguous)"; break;
        default: selection_mode_name = "Unknown"; break;
    }
}

// --- MemoryGrantInspector ---

void MemoryGrantInspector::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_manager_version"), &MemoryGrantInspector::get_manager_version);
    godot::ClassDB::bind_method(godot::D_METHOD("is_active"), &MemoryGrantInspector::is_active);
    godot::ClassDB::bind_method(godot::D_METHOD("is_emulated"), &MemoryGrantInspector::is_emulated);
    godot::ClassDB::bind_method(godot::D_METHOD("get_part_count"), &MemoryGrantInspector::get_part_count);
    
    godot::ClassDB::bind_method(godot::D_METHOD("setup_emulated_grant", "mock_parts"), &MemoryGrantInspector::setup_emulated_grant);
    godot::ClassDB::bind_method(godot::D_METHOD("get_part_snapshot", "index"), &MemoryGrantInspector::get_part_snapshot);
    godot::ClassDB::bind_method(godot::D_METHOD("has_error"), &MemoryGrantInspector::has_error);
    godot::ClassDB::bind_method(godot::D_METHOD("is_dirty"), &MemoryGrantInspector::is_dirty);
}

void MemoryGrantInspector::setup_emulated_grant(const godot::TypedArray<godot::Dictionary>& p_mock_parts) {
    manager_version = UINT64_MAX; // Use max to technically flag this as a mock instance internally if needed
    active = true;
    emulated = true;
    error_state = false;
    dirty_state = false;
    part_snapshots = p_mock_parts;
}

godot::Dictionary MemoryGrantInspector::get_part_snapshot(int p_index) const {
    if (p_index >= 0 && p_index < part_snapshots.size()) {
        return part_snapshots[p_index];
    }
    return godot::Dictionary(); // Return empty dictionary if out of bounds
}

godot::PackedInt32Array MemoryGrantInspector::get_buffer_ids() const {
    godot::PackedInt32Array ids;
    for (int i = 0; i < part_snapshots.size(); ++i) {
        godot::Dictionary dict = part_snapshots[i];
        if (dict.has("buffer_id")) {
            ids.push_back(dict["buffer_id"]);
        }
    }
    return ids;
}

} // namespace ideam::godot_ext