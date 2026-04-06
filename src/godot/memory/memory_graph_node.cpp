#include "memory_graph_node.h"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void MemoryGraphNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("update_telemetry", "inspector"), &MemoryGraphNode::update_telemetry);
    ClassDB::bind_method(D_METHOD("_on_inspect_memory_pressed"), &MemoryGraphNode::_on_inspect_memory_pressed);
    ClassDB::bind_method(D_METHOD("get_port_signature", "port_idx", "is_output"), &MemoryGraphNode::get_port_signature);

    ADD_SIGNAL(MethodInfo("inspect_memory_requested", PropertyInfo(Variant::OBJECT, "inspector", PROPERTY_HINT_RESOURCE_TYPE, "MemoryGrantInspector")));
}

MemoryGraphNode::MemoryGraphNode() {
}

void MemoryGraphNode::_build_ui() {
    IdeamGraphNode::_build_ui(); // Call base setup

    // Pre-create the telemetry button, but keep it hidden until runtime data arrives
    inspect_memory_btn = memnew(Button);
    inspect_memory_btn->set_text("No Memory Grant");
    inspect_memory_btn->set_disabled(true);
    inspect_memory_btn->connect("pressed", Callable(this, "_on_inspect_memory_pressed"));
    add_child(inspect_memory_btn);
}

void MemoryGraphNode::update_telemetry(const Ref<ideam::godot_ext::MemoryGrantInspector>& p_inspector) {
    latest_grant_snapshot = p_inspector;

    if (inspect_memory_btn) {
        if (latest_grant_snapshot.is_valid() && latest_grant_snapshot->is_active()) {
            inspect_memory_btn->set_disabled(false);
            inspect_memory_btn->set_text(String("Inspect Grant (") + String::num_int64(latest_grant_snapshot->get_part_count()) + " parts)");
            
            // Visual cue: Tint the node slightly green to show it has active locks
            set_self_modulate(Color(0.8f, 1.0f, 0.8f)); 
        } else {
            inspect_memory_btn->set_disabled(true);
            inspect_memory_btn->set_text("Grant Inactive");
            set_self_modulate(Color(1.0f, 1.0f, 1.0f));
        }
    }
}

void MemoryGraphNode::_on_inspect_memory_pressed() {
    if (latest_grant_snapshot.is_valid()) {
        // Bubble the request up so a side-panel or the Editor Inspector can display the read-only data
        emit_signal("inspect_memory_requested", latest_grant_snapshot);
    }
}

void MemoryGraphNode::register_port_signature(int p_port_idx, bool p_is_output, uint32_t p_trait_mask) {
    if (p_is_output) {
        output_port_signatures[p_port_idx] = p_trait_mask;
    } else {
        input_port_signatures[p_port_idx] = p_trait_mask;
    }
}

uint32_t MemoryGraphNode::get_port_signature(int p_port_idx, bool p_is_output) const {
    if (p_is_output) {
        auto it = output_port_signatures.find(p_port_idx);
        return it != output_port_signatures.end() ? it->second : 0;
    } else {
        auto it = input_port_signatures.find(p_port_idx);
        return it != input_port_signatures.end() ? it->second : 0;
    }
}

} // namespace godot