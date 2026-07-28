#include "managed_buffer_view.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

void ManagedBufferView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open_resource", "resource"), &ManagedBufferView::open_resource);
    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &ManagedBufferView::_on_resource_changed);
}

ManagedBufferView::ManagedBufferView() {
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    _setup_ui();
}

ManagedBufferView::~ManagedBufferView() {}

void ManagedBufferView::_setup_ui() {
    header_label = memnew(Label);
    header_label->set_theme_type_variation("HeaderLarge");
    header_label->set_text("No Managed Resource Selected");
    add_child(header_label);

    HSeparator* separator = memnew(HSeparator);
    add_child(separator);

    metrics_grid = memnew(GridContainer);
    metrics_grid->set_columns(2);
    metrics_grid->set_h_size_flags(SIZE_EXPAND_FILL);
    // Hide the grid initially until a resource is loaded
    metrics_grid->set_visible(false); 
    add_child(metrics_grid);

    // Lambda helper to create key-value rows
    auto add_metric_row = [&](const String& p_label_text, Label*& r_val_label) {
        Label* key_label = memnew(Label);
        key_label->set_text(p_label_text);
        key_label->set_theme_type_variation("HeaderSmall"); // Give keys slight emphasis
        metrics_grid->add_child(key_label);

        r_val_label = memnew(Label);
        r_val_label->set_text("N/A");
        r_val_label->set_h_size_flags(SIZE_EXPAND_FILL);
        metrics_grid->add_child(r_val_label);
    };

    add_metric_row("Consumer Name:", val_consumer);
    add_metric_row("Purpose:", val_purpose);
    add_metric_row("Layout Type:", val_layout);
    add_metric_row("Byte Footprint:", val_footprint);
    add_metric_row("Hardware Alignment:", val_alignment);
}

void ManagedBufferView::open_resource(Ref<ManagedBufferResource> p_resource) {
    if (active_resource == p_resource) return;

    if (active_resource.is_valid() && active_resource->is_connected("changed", Callable(this, "_on_resource_changed"))) {
        active_resource->disconnect("changed", Callable(this, "_on_resource_changed"));
    }

    active_resource = p_resource;

    if (active_resource.is_valid()) {
        active_resource->connect("changed", Callable(this, "_on_resource_changed"));
    }

    _build_view();
}

void ManagedBufferView::_on_resource_changed() {
    _build_view();
}

void ManagedBufferView::_clear_view() {
    header_label->set_text("No Managed Resource Selected");
    metrics_grid->set_visible(false);
}

void ManagedBufferView::_build_view() {
    if (!active_resource.is_valid()) {
        _clear_view();
        return;
    }

    metrics_grid->set_visible(true);

    // Update Header
    String consumer = active_resource->get_consumer_name();
    header_label->set_text(consumer.is_empty() ? "Unnamed Consumer" : consumer);

    // Update Metrics
    val_consumer->set_text(consumer.is_empty() ? "None" : consumer);
    
    String purpose = active_resource->get_purpose();
    val_purpose->set_text(purpose.is_empty() ? "None" : purpose);

    val_layout->set_text(_get_layout_string(active_resource->get_layout_type()));
    
    // Format bytes nicely (e.g., standardizing output to match manager logic)
    int bytes = active_resource->get_byte_footprint();
    val_footprint->set_text(String::num_int64(bytes) + " Bytes");

    int align = active_resource->get_alignment();
    val_alignment->set_text(String::num_int64(align) + "-Byte Boundary");
}

String ManagedBufferView::_get_layout_string(int p_layout) const {
    // Maps back to the MemoryBufferResource::LayoutType flags if they share the same enum
    switch (p_layout) {
        case 1:  return "Flat Array";
        case 2:  return "Array of Structs (AoS)";
        case 4:  return "Struct of Arrays (SoA)";
        case 8:  return "Sparse Set";
        case 16: return "Tiled SoA";
        case 32: return "Ring Buffer";
        case 64: return "Paged Memory";
        default: return "Unknown Layout";
    }
}

} // namespace ideam::godot_ext