#include "query_task_graph_node.h"
#include "../../core/tasks/registration/native_task_registry.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

namespace ideam::godot_ext {

void QueryTaskGraphNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_op_selected", "index"), &QueryTaskGraphNode::_on_op_selected);
    ClassDB::bind_method(D_METHOD("_on_view_selected", "index"), &QueryTaskGraphNode::_on_view_selected);
    ClassDB::bind_method(D_METHOD("_on_strategy_selected", "index"), &QueryTaskGraphNode::_on_strategy_selected);
    ClassDB::bind_method(D_METHOD("_on_type_selected", "index"), &QueryTaskGraphNode::_on_type_selected);
}

QueryTaskGraphNode::QueryTaskGraphNode() {
}

void QueryTaskGraphNode::_rebuild_dynamic_ui() {
    // 1. Mandatory base teardown
    TaskGraphNode::_rebuild_dynamic_ui();

    if (!custom_parameters_container) return;

    // 2. Instantiate the 4D Matrix Controls
    // We emphasize the QueryOp as it alters the fundamental topology of the node.
    Label* op_label = memnew(Label);
    op_label->set_text("Operational Topology");
    op_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    custom_parameters_container->add_child(op_label);

    op_dropdown = memnew(OptionButton);
    _populate_op_dropdown();
    custom_parameters_container->add_child(op_dropdown);

    HSeparator* separator = memnew(HSeparator);
    custom_parameters_container->add_child(separator);

    view_dropdown = memnew(OptionButton);
    strategy_dropdown = memnew(OptionButton);
    type_dropdown = memnew(OptionButton);

    _populate_view_dropdown();
    _populate_strategy_dropdown();
    _populate_type_dropdown();

    custom_parameters_container->add_child(view_dropdown);
    custom_parameters_container->add_child(strategy_dropdown);
    custom_parameters_container->add_child(type_dropdown);

    // 3. Bind UI Signals
    op_dropdown->connect("item_selected", Callable(this, "_on_op_selected"));
    view_dropdown->connect("item_selected", Callable(this, "_on_view_selected"));
    strategy_dropdown->connect("item_selected", Callable(this, "_on_strategy_selected"));
    type_dropdown->connect("item_selected", Callable(this, "_on_type_selected"));

    // 4. Restore state from DOD Property Dictionary
    Dictionary props = get_properties();
    if (props.has("op_id")) op_dropdown->select(static_cast<int>(props["op_id"]));
    if (props.has("view_id")) view_dropdown->select(static_cast<int>(props["view_id"]));
    if (props.has("strategy_id")) strategy_dropdown->select(static_cast<int>(props["strategy_id"]));
    if (props.has("type_id")) type_dropdown->select(static_cast<int>(props["type_id"]));

    // 5. Run initial guardrail evaluation to prune invalid combos
    _update_matrix_guardrails();
}

uint64_t QueryTaskGraphNode::_calculate_flat_index() const {
    if (!op_dropdown || !view_dropdown || !strategy_dropdown || !type_dropdown) return 0;

    uint32_t o_id = static_cast<uint32_t>(op_dropdown->get_selected_id());
    uint32_t v_id = static_cast<uint32_t>(view_dropdown->get_selected_id());
    uint32_t s_id = static_cast<uint32_t>(strategy_dropdown->get_selected_id());
    uint32_t t_id = static_cast<uint32_t>(type_dropdown->get_selected_id());

    // Matrix Dimensions: O_COUNT (2) * V_COUNT (16) * S_COUNT (9) * T_COUNT (17)
    return (o_id * 2448) + (v_id * 153) + (s_id * 17) + t_id;
}

void QueryTaskGraphNode::_update_matrix_guardrails() {
    if (!op_dropdown || !view_dropdown || !strategy_dropdown || !type_dropdown) return;

    Dictionary matrix = core::NativeTaskRegistry::get_ui_query_matrix();
    String logic_str = String::num_int64(get_logic_id());
    
    if (!matrix.has(logic_str)) return;

    Dictionary logic_def = matrix[logic_str];
    if (!logic_def.has("valid_combinations")) return;

    PackedInt64Array valid_hashes = logic_def["valid_combinations"];

    uint32_t current_o = static_cast<uint32_t>(op_dropdown->get_selected_id());
    uint32_t current_v = static_cast<uint32_t>(view_dropdown->get_selected_id());
    uint32_t current_s = static_cast<uint32_t>(strategy_dropdown->get_selected_id());
    uint32_t current_t = static_cast<uint32_t>(type_dropdown->get_selected_id());

    // Helper to evaluate if a flat index exists in the valid hashes
    auto is_valid = [&](uint64_t hash) -> bool {
        for (int i = 0; i < valid_hashes.size(); ++i) {
            if (valid_hashes[i] == hash) return true;
        }
        return false;
    };

    bool is_current_selection_valid = is_valid(_calculate_flat_index());

    // Evaluate and prune Ops based on current View & Strategy & Type
    for (int o = 0; o < op_dropdown->get_item_count(); ++o) {
        uint64_t test_hash = (o * 2448) + (current_v * 153) + (current_s * 17) + current_t;
        op_dropdown->set_item_disabled(o, !is_valid(test_hash));
    }

    // Evaluate and prune Views based on current Op & Strategy & Type
    for (int v = 0; v < view_dropdown->get_item_count(); ++v) {
        uint64_t test_hash = (current_o * 2448) + (v * 153) + (current_s * 17) + current_t;
        view_dropdown->set_item_disabled(v, !is_valid(test_hash));
    }

    // Evaluate and prune Strategies based on current Op & View & Type
    for (int s = 0; s < strategy_dropdown->get_item_count(); ++s) {
        uint64_t test_hash = (current_o * 2448) + (current_v * 153) + (s * 17) + current_t;
        strategy_dropdown->set_item_disabled(s, !is_valid(test_hash));
    }

    // Evaluate and prune Types based on current Op & View & Strategy
    for (int t = 0; t < type_dropdown->get_item_count(); ++t) {
        uint64_t test_hash = (current_o * 2448) + (current_v * 153) + (current_s * 17) + t;
        type_dropdown->set_item_disabled(t, !is_valid(test_hash));
    }

    // Visually flag the node if the user's upstream topological changes locked out the current config
    if (!is_current_selection_valid) {
        set_error_state(true);
    } else {
        set_error_state(false);
    }
}

// --- Interaction Routing ---

void QueryTaskGraphNode::_on_op_selected(int p_index) {
    _on_custom_param_changed("op_id", p_index);
}

void QueryTaskGraphNode::_on_view_selected(int p_index) {
    _on_custom_param_changed("view_id", p_index);
}

void QueryTaskGraphNode::_on_strategy_selected(int p_index) {
    _on_custom_param_changed("strategy_id", p_index);
}

void QueryTaskGraphNode::_on_type_selected(int p_index) {
    _on_custom_param_changed("type_id", p_index);
}

// --- Population Helpers ---

void QueryTaskGraphNode::_populate_op_dropdown() {
    op_dropdown->add_item("CULL (Synchronous)", 0);
    op_dropdown->add_item("ADD (Deferred Wave)", 1);
}

void QueryTaskGraphNode::_populate_view_dropdown() {
    view_dropdown->add_item("Single Element", 0);
    view_dropdown->add_item("Multi Element", 1);
    view_dropdown->add_item("Sparse Set", 2);
    view_dropdown->add_item("Paged", 3);
    view_dropdown->add_item("Ring", 4);
    view_dropdown->add_item("Stencil", 5);
    view_dropdown->add_item("Atomic", 6);
    view_dropdown->add_item("Swap", 7);
    view_dropdown->add_item("Static Stencil", 8);
    view_dropdown->add_item("Bridge", 9);
    view_dropdown->add_item("AOSOA Tight AVX2", 10);
    view_dropdown->add_item("AOSOA Tight AVX512", 11);
    view_dropdown->add_item("AOSOA STD430 AVX2", 12);
    view_dropdown->add_item("AOSOA STD430 AVX512", 13);
    view_dropdown->add_item("AOSOA STD140 AVX2", 14);
    view_dropdown->add_item("AOSOA STD140 AVX512", 15);
}

void QueryTaskGraphNode::_populate_strategy_dropdown() {
    strategy_dropdown->add_item("Flat", 0);
    strategy_dropdown->add_item("SoA", 1);
    strategy_dropdown->add_item("AoS", 2);
    strategy_dropdown->add_item("Spatial 2D", 3);
    strategy_dropdown->add_item("Spatial 3D", 4);
    strategy_dropdown->add_item("Spatial 4D", 5);
    strategy_dropdown->add_item("Tiled SoA", 6);
    strategy_dropdown->add_item("Ring", 7);
    strategy_dropdown->add_item("Paged", 8);
}

void QueryTaskGraphNode::_populate_type_dropdown() {
    type_dropdown->add_item("BOOL", 0);
    type_dropdown->add_item("BYTE", 1);
    type_dropdown->add_item("INT32", 2);
    type_dropdown->add_item("INT64", 3);
    type_dropdown->add_item("FLOAT32", 4);
    type_dropdown->add_item("FLOAT64", 5);
    type_dropdown->add_item("VECTOR2", 6);
    type_dropdown->add_item("VECTOR3", 7);
    type_dropdown->add_item("VECTOR4", 8);
    type_dropdown->add_item("VECTOR2I", 9);
    type_dropdown->add_item("VECTOR3I", 10);
    type_dropdown->add_item("VECTOR4I", 11);
    type_dropdown->add_item("VECTOR2D", 12);
    type_dropdown->add_item("VECTOR3D", 13);
    type_dropdown->add_item("VECTOR4D", 14);
    type_dropdown->add_item("COLOR", 15);
    type_dropdown->add_item("CUSTOM", 16);
}

} // namespace ideam::godot_ext