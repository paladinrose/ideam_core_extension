#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

// --- Editor ---
#include "godot/editor/ideam_editor_inspector_plugin.h"
#include "godot/editor/ideam_editor_plugin.h"

// --- Graphs ---
#include "godot/graphs/ideam_graph_edit.h"
#include "godot/graphs/ideam_graph_inspector.h"
#include "godot/graphs/ideam_graph_node.h"
#include "godot/graphs/ideam_graph_resource.h"
#include "godot/graphs/ideam_graphs_plugin.h"

// --- Memory ---
#include "godot/memory/memory_buffer_resource.h"
#include "godot/memory/memory_graph_edit.h"
#include "godot/memory/memory_graph_node.h"
#include "godot/memory/memory_graph_resource.h"
#include "godot/memory/memory_graph_inspector.h"
#include "godot/memory/memory_inspectors.h"
#include "godot/memory/memory_manager_resource.h"

// --- Project Tools ---
#include "godot/project_tools/ideam_project_tools_plugin.h"
#include "godot/project_tools/project_settings_tool.h"
#include "godot/project_tools/project_wizard.h"

// --- Simulations ---


// --- Tasks ---
#include "godot/tasks/ideam_tasks_plugin.h"
#include "godot/tasks/task_graph_inspector.h"
#include "godot/tasks/task_graph_edit.h"
#include "godot/tasks/task_graph_node.h"
#include "godot/tasks/task_graph_resource.h"

using namespace godot;

void initialize_ideam_core_module(ModuleInitializationLevel p_level) {
	// ========================================================================
	// SCENE LEVEL
	// Register all runtime Nodes, Resources, and RefCounted objects here.
	// These are available in the running game and the exported binary.
	// ========================================================================
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// Base Graph Components
		ClassDB::register_class<IdeamGraphNode>();
		ClassDB::register_class<IdeamGraphEdit>();
		ClassDB::register_class<ideam::godot_ext::IdeamGraphResource>();
		
		// Memory Components
		ClassDB::register_class<ideam::godot_ext::MemoryBufferResource>();
		ClassDB::register_class<ideam::godot_ext::MemoryManagerResource>();
		ClassDB::register_class<ideam::godot_ext::MemoryGraphResource>();
		ClassDB::register_class<ideam::godot_ext::MemorySelectionInspector>();
		ClassDB::register_class<ideam::godot_ext::MemoryGrantInspector>();
		ClassDB::register_class<MemoryGraphNode>();
		ClassDB::register_class<MemoryGraphEdit>();
		
		// Project Tool Components (Controls/UI logic used by the editor but fundamentally UI)
		ClassDB::register_class<ProjectSettingsTool>();
		ClassDB::register_class<ScriptNode>();
		ClassDB::register_class<ResourceNode>();
		ClassDB::register_class<SceneNode>();
		ClassDB::register_class<FolderNode>();
		ClassDB::register_class<ProjectWizard>();
		
		// Simulations
		
		
		// Tasks
		ClassDB::register_class<ideam::godot_ext::TaskGraphResource>();
		ClassDB::register_class<TaskGraphNode>();
		ClassDB::register_class<TaskGraphEdit>();
	}

	// ========================================================================
	// EDITOR LEVEL
	// Register all EditorPlugins and EditorInspectorPlugins here.
	// If you register these at SCENE level, your export builds will crash!
	// ========================================================================
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		// Abstract Base classes
		ClassDB::register_class<IdeamEditorPlugin>();
		ClassDB::register_class<IdeamEditorInspectorPlugin>();

		// Concrete Editor Plugins
		ClassDB::register_class<IdeamGraphsPlugin>();
		ClassDB::register_class<IdeamProjectToolsPlugin>();
		ClassDB::register_class<IdeamTasksPlugin>();

		// Concrete Inspector Plugins
		ClassDB::register_class<IdeamGraphInspector>();
		ClassDB::register_class<MemoryGraphInspector>();
		ClassDB::register_class<TaskGraphInspector>();
	}
}

void uninitialize_ideam_core_module(ModuleInitializationLevel p_level) {
	// Add teardown logic here if your Singletons or static managers need explicit cleanup.
	// Failing to clean up allocated static pointers here will cause Godot to flag memory 
	// leaks in the console on exit.
	
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// Cleanup Scene level statics/singletons
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		// Cleanup Editor level statics/singletons
	}
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT ideam_core_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_ideam_core_module);
	init_obj.register_terminator(uninitialize_ideam_core_module);
	
	// We set minimum level to SCENE, but our initialization function handles both SCENE and EDITOR
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}