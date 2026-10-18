#include "ring_visualizer.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

void RingVisualizer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup_from_resource", "buffer"), &RingVisualizer::setup_from_resource);
    ClassDB::bind_method(D_METHOD("_on_buffer_changed"), &RingVisualizer::_on_buffer_changed);

    ClassDB::bind_method(D_METHOD("step_enqueue", "count"), &RingVisualizer::step_enqueue, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("step_dequeue", "count"), &RingVisualizer::step_dequeue, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("reset_simulation"), &RingVisualizer::reset_simulation);

    ClassDB::bind_method(D_METHOD("get_head_index"), &RingVisualizer::get_head_index);
    ClassDB::bind_method(D_METHOD("get_tail_index"), &RingVisualizer::get_tail_index);
    ClassDB::bind_method(D_METHOD("get_element_count"), &RingVisualizer::get_element_count);
    ClassDB::bind_method(D_METHOD("get_is_overflowing"), &RingVisualizer::get_is_overflowing);

    ClassDB::bind_method(D_METHOD("_on_enqueue_pressed"), &RingVisualizer::_on_enqueue_pressed);
    ClassDB::bind_method(D_METHOD("_on_dequeue_pressed"), &RingVisualizer::_on_dequeue_pressed);
    ClassDB::bind_method(D_METHOD("_on_batch_pressed"), &RingVisualizer::_on_batch_pressed);
    ClassDB::bind_method(D_METHOD("_on_reset_pressed"), &RingVisualizer::_on_reset_pressed);
}

RingVisualizer::RingVisualizer() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    // 1. Emulation Toolbar
    toolbar = memnew(HBoxContainer);
    toolbar->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(toolbar);

    btn_enqueue = memnew(Button);
    btn_enqueue->set_text("+1 Enqueue");
    btn_enqueue->set_tooltip_text("Simulate Write Head enqueue step (+1 element)");
    btn_enqueue->connect("pressed", Callable(this, "_on_enqueue_pressed"));
    toolbar->add_child(btn_enqueue);

    btn_dequeue = memnew(Button);
    btn_dequeue->set_text("-1 Dequeue");
    btn_dequeue->set_tooltip_text("Simulate Read Tail dequeue step (-1 element)");
    btn_dequeue->connect("pressed", Callable(this, "_on_dequeue_pressed"));
    toolbar->add_child(btn_dequeue);

    btn_batch = memnew(Button);
    btn_batch->set_text("+8 Step");
    btn_batch->set_tooltip_text("Enqueue 8 elements in a single step");
    btn_batch->connect("pressed", Callable(this, "_on_batch_pressed"));
    toolbar->add_child(btn_batch);

    btn_reset = memnew(Button);
    btn_reset->set_text("Reset");
    btn_reset->set_tooltip_text("Reset streaming Head and Tail pointers");
    btn_reset->connect("pressed", Callable(this, "_on_reset_pressed"));
    toolbar->add_child(btn_reset);

    telemetry_label = memnew(Label);
    telemetry_label->set_h_size_flags(SIZE_EXPAND_FILL);
    telemetry_label->set_text("Head: 0 | Tail: 0 | Elements: 0/0 | State: EMPTY");
    toolbar->add_child(telemetry_label);

    hazard_badge = memnew(Label);
    hazard_badge->set_text("[OVERFLOW HAZARD]");
    hazard_badge->set_modulate(Color(0.95f, 0.25f, 0.25f));
    hazard_badge->set_visible(false);
    toolbar->add_child(hazard_badge);

    // 2. Circular Gauge Viewport
    gauge_container = memnew(CenterContainer);
    gauge_container->set_h_size_flags(SIZE_EXPAND_FILL);
    gauge_container->set_v_size_flags(SIZE_EXPAND_FILL);
    gauge_container->set_custom_minimum_size(Vector2(200.0f, 200.0f));
    add_child(gauge_container);

    gauge = memnew(CircularGauge);
    gauge->set_custom_minimum_size(Vector2(190.0f, 190.0f));
    gauge->set_h_size_flags(SIZE_EXPAND_FILL);
    gauge->set_v_size_flags(SIZE_EXPAND_FILL);
    gauge->set_start_angle(0.0f);
    gauge->set_end_angle(360.0f);
    gauge->set_marker_count(2);
    gauge->set_draw_as_band(true);
    gauge->set_draw_slot_ticks(true);
    gauge->set_draw_head_tail_markers(true);
    gauge_container->add_child(gauge);

    // 3. Legend & Tuple Stream Footer
    legend_container = memnew(HBoxContainer);
    legend_container->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(legend_container);
}

RingVisualizer::~RingVisualizer() {
    _clear_connections();
}

void RingVisualizer::_clear_connections() {
    if (active_buffer.is_valid() && active_buffer->is_connected("changed", Callable(this, "_on_buffer_changed"))) {
        active_buffer->disconnect("changed", Callable(this, "_on_buffer_changed"));
    }
}

void RingVisualizer::setup_from_resource(Ref<MemoryBufferResource> p_buffer) {
    if (active_buffer == p_buffer) return;

    _clear_connections();
    active_buffer = p_buffer;

    if (active_buffer.is_valid()) {
        active_buffer->connect("changed", Callable(this, "_on_buffer_changed"));
    }

    reset_simulation();
}

void RingVisualizer::_on_buffer_changed() {
    _update_gauge_and_ui();
}

Color RingVisualizer::_get_type_color(DataType p_type) const {
    switch (p_type) {
        case DataType::BOOL:     return Color(0.96f, 0.62f, 0.04f);
        case DataType::BYTE:     return Color(0.47f, 0.44f, 0.42f);
        case DataType::INT32:    return Color(0.06f, 0.72f, 0.51f);
        case DataType::INT64:    return Color(0.02f, 0.47f, 0.34f);
        case DataType::FLOAT32:  return Color(0.05f, 0.65f, 0.91f);
        case DataType::FLOAT64:  return Color(0.11f, 0.31f, 0.85f);
        case DataType::VECTOR2:
        case DataType::VECTOR3:
        case DataType::VECTOR4:  return Color(0.02f, 0.71f, 0.83f);
        case DataType::VECTOR2I:
        case DataType::VECTOR3I:
        case DataType::VECTOR4I: return Color(0.08f, 0.72f, 0.65f);
        case DataType::COLOR:    return Color(0.85f, 0.27f, 0.94f);
        default:                 return Color(0.55f, 0.36f, 0.96f);
    }
}

String RingVisualizer::_get_type_badge(DataType p_type) const {
    switch (p_type) {
        case DataType::BOOL:     return "[B1]";
        case DataType::BYTE:     return "[U1]";
        case DataType::INT32:    return "[I4]";
        case DataType::INT64:    return "[I8]";
        case DataType::FLOAT32:  return "[F4]";
        case DataType::FLOAT64:  return "[F8]";
        case DataType::VECTOR2:  return "[V2F]";
        case DataType::VECTOR3:  return "[V3F]";
        case DataType::VECTOR4:  return "[V4F]";
        case DataType::COLOR:    return "[C4]";
        default:                 return "[?]";
    }
}

void RingVisualizer::_update_gauge_and_ui() {
    if (!active_buffer.is_valid()) return;

    int capacity = active_buffer->get_max_elements();
    if (capacity <= 0) capacity = 1;

    // Build concentric layer colors matching column schema
    PackedColorArray layer_colors;
    TypedArray<Dictionary> cols = active_buffer->get_columns();
    for (int i = 0; i < cols.size(); ++i) {
        Dictionary col = cols[i];
        int64_t raw_dt = col.has("data_type") ? static_cast<int64_t>(col["data_type"]) : static_cast<int64_t>(DataType::FLOAT32);
        layer_colors.push_back(_get_type_color(static_cast<DataType>(raw_dt)));
    }
    if (layer_colors.is_empty()) {
        layer_colors.push_back(Color(0.12f, 0.65f, 0.85f, 1.0f));
    }

    gauge->set_min_value(0.0f);
    gauge->set_max_value(static_cast<float>(capacity));
    gauge->set_slot_count(capacity);
    gauge->set_layer_colors(layer_colors);

    gauge->set_value_1(static_cast<float>(sim_tail)); // Tail / Read
    gauge->set_value_2(static_cast<float>(sim_head)); // Head / Write

    bool is_full = (sim_count >= capacity);
    bool is_wrapped = (sim_head < sim_tail) && !is_full;

    gauge->set_is_full(is_full);
    gauge->set_is_wrapped(is_wrapped);
    gauge->set_is_overflowing(is_overflowing);

    // Update Status Telemetry
    String state_str = "STREAMING";
    if (sim_count == 0) state_str = "EMPTY";
    else if (is_overflowing) state_str = "OVERFLOW";
    else if (is_full) state_str = "FULL";
    else if (is_wrapped) state_str = "WRAPPED SEAM";

    telemetry_label->set_text(
        String("Head: ") + String::num_int64(sim_head) +
        " | Tail: " + String::num_int64(sim_tail) +
        " | Elements: " + String::num_int64(sim_count) + "/" + String::num_int64(capacity) +
        " | State: " + state_str
    );

    hazard_badge->set_visible(is_overflowing);

    // Rebuild Legend
    for (int i = legend_container->get_child_count() - 1; i >= 0; --i) {
        Node* child = legend_container->get_child(i);
        legend_container->remove_child(child);
        child->queue_free();
    }

    Label* head_leg = memnew(Label);
    head_leg->set_text("▷ Write Head (Outer)");
    head_leg->set_modulate(Color(0.98f, 0.78f, 0.15f));
    legend_container->add_child(head_leg);

    Label* tail_leg = memnew(Label);
    tail_leg->set_text("♢ Read Tail (Inner)");
    tail_leg->set_modulate(Color(0.18f, 0.85f, 0.98f));
    legend_container->add_child(tail_leg);

    if (!cols.is_empty()) {
        Label* sep = memnew(Label);
        sep->set_text("| Streams:");
        legend_container->add_child(sep);

        for (int i = 0; i < cols.size(); ++i) {
            Dictionary col = cols[i];
            String name = col.has("name") ? String(col["name"]) : "stream_" + String::num_int64(i);
            int64_t raw_dt = col.has("data_type") ? static_cast<int64_t>(col["data_type"]) : static_cast<int64_t>(DataType::FLOAT32);
            DataType dt = static_cast<DataType>(raw_dt);

            Label* col_lbl = memnew(Label);
            col_lbl->set_text(_get_type_badge(dt) + " " + name);
            col_lbl->set_modulate(_get_type_color(dt));
            legend_container->add_child(col_lbl);
        }
    }
}

void RingVisualizer::step_enqueue(int p_count) {
    if (!active_buffer.is_valid()) return;
    int capacity = active_buffer->get_max_elements();
    if (capacity <= 0) return;

    for (int i = 0; i < p_count; ++i) {
        if (sim_count < capacity) {
            sim_head = (sim_head + 1) % capacity;
            sim_count++;
        } else {
            // Buffer full: Write head advances and displaces unread read tail
            sim_head = (sim_head + 1) % capacity;
            sim_tail = (sim_tail + 1) % capacity;
            is_overflowing = true;
        }
    }

    _update_gauge_and_ui();
}

void RingVisualizer::step_dequeue(int p_count) {
    if (!active_buffer.is_valid()) return;
    int capacity = active_buffer->get_max_elements();
    if (capacity <= 0) return;

    for (int i = 0; i < p_count; ++i) {
        if (sim_count > 0) {
            sim_tail = (sim_tail + 1) % capacity;
            sim_count--;
        }
    }

    if (sim_count < capacity) {
        is_overflowing = false;
    }

    _update_gauge_and_ui();
}

void RingVisualizer::reset_simulation() {
    sim_head = 0;
    sim_tail = 0;
    sim_count = 0;
    is_overflowing = false;
    _update_gauge_and_ui();
}

int RingVisualizer::get_head_index() const { return sim_head; }
int RingVisualizer::get_tail_index() const { return sim_tail; }
int RingVisualizer::get_element_count() const { return sim_count; }
bool RingVisualizer::get_is_overflowing() const { return is_overflowing; }

void RingVisualizer::_on_enqueue_pressed() { step_enqueue(1); }
void RingVisualizer::_on_dequeue_pressed() { step_dequeue(1); }
void RingVisualizer::_on_batch_pressed() { step_enqueue(8); }
void RingVisualizer::_on_reset_pressed() { reset_simulation(); }

Vector2 RingVisualizer::_get_minimum_size() const {
    return Vector2(240.0f, 280.0f);
}

} // namespace ideam::godot_ext