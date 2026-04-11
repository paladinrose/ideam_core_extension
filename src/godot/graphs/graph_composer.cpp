#include "graph_composer.h"
#include "ideam_graphs_plugin.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// Bring Godot types into scope locally for the implementation file
using namespace godot;

namespace ideam::godot_ext {

void GraphComposer::_bind_methods() {
    // Expose instance operations to Godot's reflection system if GDScript interaction is required
    ClassDB::bind_method(D_METHOD("open_graph", "graph_edit"), &GraphComposer::open_graph);
    ClassDB::bind_method(D_METHOD("close_graph", "graph_edit"), &GraphComposer::close_graph);
}

GraphComposer::GraphComposer() {
    set_anchors_preset(PRESET_FULL_RECT);
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);

    tab_container = memnew(TabContainer);
    tab_container->set_h_size_flags(SIZE_EXPAND_FILL);
    tab_container->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(tab_container);

    // Pre-allocate vector capacity to prevent heap fragmentation during the first few insertions.
    active_sessions.reserve(8); 
}

GraphComposer::~GraphComposer() {
    // The vector drops automatically. Godot's internal memory management handles child node deletion.
}

void GraphComposer::_notification(int p_what) {
    // Hook standard Godot lifecycle events here if necessary
}

void GraphComposer::open_graph(IdeamGraphEdit* p_graph_edit) {
    if (!p_graph_edit) return;

    Ref<IdeamGraphResource> blueprint = p_graph_edit->get_blueprint();
    if (blueprint.is_null()) {
        // Reject invalid states before they dirty our tracking vectors.
        UtilityFunctions::printerr("GraphComposer: Attempted to open graph with null blueprint.");
        p_graph_edit->queue_free(); 
        return; 
    }

    // Fast-path lookup key extraction
    const auto* target_key = blueprint.ptr();

    // Contiguous memory scan: CPU cache friendly, zero VTable lookups, no node tree traversal.
    for (const auto& session : active_sessions) {
        if (session.resource_key == target_key) {
            // Deduplication hit. Focus the existing tab and discard the redundant incoming node.
            tab_container->set_current_tab(session.tab_index);
            p_graph_edit->queue_free(); 
            return; 
        }
    }

    // Cache-miss implies new session. Proceed with adding the new UI Representation.
    String node_name = blueprint->get_name().is_empty() ? "Untitled Graph" : blueprint->get_name();
    p_graph_edit->set_name(node_name);
    
    tab_container->add_child(p_graph_edit);
    int new_index = tab_container->get_tab_count() - 1;
    tab_container->set_current_tab(new_index);

    // Track state securely using the raw pointer as our 64-bit integer fast-lookup key.
    active_sessions.push_back({target_key, p_graph_edit, new_index});
}

void GraphComposer::close_graph(IdeamGraphEdit* p_graph_edit) {
    if (!p_graph_edit) return;

    // Use the node pointer itself for removal matching
    for (auto it = active_sessions.begin(); it != active_sessions.end(); ++it) {
        if (it->editor_node == p_graph_edit) {
            tab_container->remove_child(p_graph_edit);
            p_graph_edit->queue_free();

            // Erase and shift memory. O(N) is an acceptable overhead here given the strictly bounded N 
            // of physical UI tabs, combined with the cache locality of the shift.
            active_sessions.erase(it);

            // Re-align tab indices to keep the tracking struct strictly synchronized with the Godot UI state.
            for (int i = 0; i < active_sessions.size(); ++i) {
                active_sessions[i].tab_index = i;
            }
            break;
        }
    }
}

// --- Static Entry Points (Routing Layer) ---

void GraphComposer::edit_ideam_graph(IdeamGraphEdit* p_graph_edit, Control* p_owner) {
    GraphComposer* target_composer = nullptr;

    if (p_owner) {
        // Attempt to locate an existing GraphComposer on the explicitly provided owner
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            target_composer = Object::cast_to<GraphComposer>(p_owner->get_child(i));
            if (target_composer) break;
        }

        // Allocate and attach if none exists in the local context
        if (!target_composer) {
            target_composer = memnew(GraphComposer);
            p_owner->add_child(target_composer);
        }
    } else {
        // Fallback to the Plugin's shared window via the static provider
        Window* shared_window = IdeamGraphsPlugin::get_shared_composer_window();
        if (shared_window) {
            for (int i = 0; i < shared_window->get_child_count(); ++i) {
                target_composer = Object::cast_to<GraphComposer>(shared_window->get_child(i));
                if (target_composer) break;
            }

            if (!target_composer) {
                target_composer = memnew(GraphComposer);
                shared_window->add_child(target_composer);
            }
            
            shared_window->popup(); // Ensure visibility of the shared context
        }
    }

    // Execute the tightly packed instance method
    if (target_composer) {
        target_composer->open_graph(p_graph_edit);
    } else {
        // Safe disposal if routing completely fails
        p_graph_edit->queue_free();
    }
}

void GraphComposer::close_ideam_graph(IdeamGraphEdit* p_graph_edit, Control* p_owner) {
    GraphComposer* target_composer = nullptr;

    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            target_composer = Object::cast_to<GraphComposer>(p_owner->get_child(i));
            if (target_composer) break;
        }
    } else {
        Window* shared_window = IdeamGraphsPlugin::get_shared_composer_window();
        if (shared_window) {
            for (int i = 0; i < shared_window->get_child_count(); ++i) {
                target_composer = Object::cast_to<GraphComposer>(shared_window->get_child(i));
                if (target_composer) break;
            }
        }
    }

    if (target_composer) {
        target_composer->close_graph(p_graph_edit);
    }
}

} // namespace ideam::godot_ext