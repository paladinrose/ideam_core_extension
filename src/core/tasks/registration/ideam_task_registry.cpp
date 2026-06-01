#include "ideam_task_registry.h"

// --- Sub-Registries ---
#include "query_task_registry.h"
#include "transform_task_registry.h"
#include "metadata_task_registry.h"

// --- Manual Tasks ---
#include "../entry_fill_task.h"
#include "../sub_graph_task.h"

// --- Godot Editor Bridging (Resources & UI Graph Nodes) ---
#include "../../../godot/tasks/entry_fill_task_resource.h"
#include "../../../godot/tasks/entry_fill_task_graph_node.h"
#include "../../../godot/tasks/sub_graph_task_resource.h"
#include "../../../godot/tasks/sub_graph_task_graph_node.h"
#include "../../../godot/tasks/task_graph_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::core {

godot::Ref<godot::FileAccess> IdeamTaskRegistry::bake_log_file;

void IdeamTaskRegistry::start_bake_log() {
    // Opens a text file at the root of your Godot project
    bake_log_file = godot::FileAccess::open("res://ideam_bake_log.txt", godot::FileAccess::WRITE);
    if (bake_log_file.is_valid()) {
        bake_log_file->store_line("=== TASK REGISTRY BAKE LOG START ===");
    } else {
        godot::UtilityFunctions::printerr("IdeamTasks: Could not open bake_log.txt for writing.");
    }
}

void IdeamTaskRegistry::log_with_registry(const godot::String& p_message) {
    if (bake_log_file.is_valid()) {
        bake_log_file->store_line(p_message);
    }
}

void IdeamTaskRegistry::end_bake_log() {
    if (bake_log_file.is_valid()) {
        bake_log_file->store_line("=== TASK REGISTRY BAKE LOG END ===");
        bake_log_file->flush();
        bake_log_file.unref(); // Safely closes the file stream
    }
}

IdeamTaskRegistry* IdeamTaskRegistry::singleton = nullptr;

void IdeamTaskRegistry::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("bake_manifest"), &IdeamTaskRegistry::bake_manifest);
    godot::ClassDB::bind_method(godot::D_METHOD("get_manifest_version"), &IdeamTaskRegistry::get_manifest_version);
    
    ADD_SIGNAL(godot::MethodInfo("manifest_updated"));
}

void IdeamTaskRegistry::init() {
    if (!singleton) {
        singleton = memnew(IdeamTaskRegistry);
        godot::Engine::get_singleton()->register_singleton("IdeamTaskRegistry", singleton);
    }

    // 1. Load or Create the Singleton Manifest Resource
    singleton->active_manifest = godot::ResourceLoader::get_singleton()->load(TaskManifest::MANIFEST_PATH);
    if (!singleton->active_manifest.is_valid()) {
        singleton->active_manifest.instantiate();
    }

    // 2. Register Unique Tasks (Populates C++ closures in memory + caches utility matrix definitions)
    singleton->register_task<EntryFillTask, godot_ext::EntryFillTaskResource, godot_ext::EntryFillTaskGraphNode>("EntryFillTask"); 
    singleton->register_task<SubGraphTask, godot_ext::SubGraphTaskResource, godot_ext::SubGraphTaskGraphNode>("SubGraphTask");

    // 3. Spin up the DOD Dispatchers (Fast-path C++ routing arrays)
    QueryTaskRegistry::init_execution_routing();
    TransformTaskRegistry::init_execution_routing();
    MetadataTaskRegistry::init_execution_routing();
}

std::unique_ptr<INativeTask> IdeamTaskRegistry::create(const godot::StringName& p_name) {
    if (manual_factories.has(p_name)) {
        return manual_factories[p_name]();
    }
    godot::UtilityFunctions::printerr("IdeamTaskRegistry: Failed to create unregistered manual task '", p_name, "'");
    return nullptr;
}

void IdeamTaskRegistry::cleanup() {
    if (singleton) {
        godot::Engine::get_singleton()->unregister_singleton("IdeamTaskRegistry");
        
        // Clean up fast-path execution routers
        QueryTaskRegistry::cleanup_execution_routing();
        TransformTaskRegistry::cleanup_execution_routing();
        MetadataTaskRegistry::cleanup_execution_routing();

        memdelete(singleton);
        singleton = nullptr;
    }
}

void IdeamTaskRegistry::bake_manifest() {
    start_bake_log();
    log_with_registry("INIT: Starting bake_manifest sequence.");

    if (!active_manifest.is_valid()) {
        log_with_registry("ERROR: Active manifest is invalid. Aborting.");
        godot::UtilityFunctions::printerr("IdeamTasks: Cannot bake manifest. Active manifest is invalid.");
        end_bake_log();
        return;
    }

    // 1. Instantiate local dictionaries on the stack
    godot::Dictionary query_matrix;
    godot::Dictionary transform_matrix;
    godot::Dictionary metadata_matrix;

    // 2. Trigger the intensive editor-bound UI scraping, passing the dictionaries by reference
    QueryTaskRegistry::generate_ui_matrices(query_matrix);
    log_with_registry("METRIC: query_matrix size after generation = " + godot::itos(query_matrix.size()));
    TransformTaskRegistry::generate_ui_matrices(transform_matrix);
    log_with_registry("METRIC: transform_matrix size after generation = " + godot::itos(transform_matrix.size()));
    MetadataTaskRegistry::generate_ui_matrices(metadata_matrix);
    log_with_registry("METRIC: metadata_matrix size after generation = " + godot::itos(metadata_matrix.size()));


    // 3. Extract out the generated dictionaries to the active manifest resource
    active_manifest->set_query_matrix(query_matrix);
    active_manifest->set_transform_matrix(transform_matrix);
    active_manifest->set_metadata_matrix(metadata_matrix);

    // Apply the locally cached utility matrix from manual registrations
    active_manifest->set_utility_matrix(pending_utility_matrix);
    active_manifest->set_manifest_version(TaskManifest::CURRENT_MANIFEST_VERSION);

    // 4. Serialize to disk 
    log_with_registry("SAVE: Attempting to save manifest to " + godot::String(TaskManifest::MANIFEST_PATH));
    godot::Error err = godot::ResourceSaver::get_singleton()->save(active_manifest, TaskManifest::MANIFEST_PATH);
    
    if (err == godot::OK) {
        log_with_registry("SUCCESS: Task Manifest baked successfully.");
        godot::UtilityFunctions::print("IdeamTasks: Task Manifest successfully baked!");
        emit_signal("manifest_updated");
    } else {
        log_with_registry("ERROR: Failed to save Task Manifest. Godot Error Code: " + godot::itos(err));
        godot::UtilityFunctions::printerr("IdeamTasks: Failed to save Task Manifest!");
    }

    end_bake_log();
}

int IdeamTaskRegistry::get_manifest_version() const {
    return active_manifest.is_valid() ? active_manifest->get_manifest_version() : 0;
}


godot::Dictionary IdeamTaskRegistry::get_ui_query_matrix() {
    if (singleton && singleton->active_manifest.is_valid()) {
        return singleton->active_manifest->get_query_matrix();
    }
    return godot::Dictionary();
}

godot::Dictionary IdeamTaskRegistry::get_ui_transform_matrix() {
    if (singleton && singleton->active_manifest.is_valid()) {
        return singleton->active_manifest->get_transform_matrix();
    }
    return godot::Dictionary();
}

godot::Dictionary IdeamTaskRegistry::get_ui_metadata_matrix() {
    if (singleton && singleton->active_manifest.is_valid()) {
        return singleton->active_manifest->get_metadata_matrix();
    }
    return godot::Dictionary();
}

godot::Dictionary IdeamTaskRegistry::get_ui_utility_matrix() {
    if (singleton && singleton->active_manifest.is_valid()) {
        return singleton->active_manifest->get_utility_matrix();
    }
    return godot::Dictionary();
}


} // namespace ideam::core