#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "godot/graphs/ideam_graph_resource.h"
#include "godot/graphs/ideam_graph_edit.h"
#include "godot/graphs/ideam_graph_node.h"
#include "godot/graphs/ideam_graphs_plugin.h"
#include "godot/graphs/ideam_graph_inspector.h"
#include "godot/graphs/graph_composer.h"

#include "godot/memory/memory_buffer_resource.h"
#include "godot/memory/managed_buffer_profile.h"
#include "godot/memory/memory_manager_resource.h"
#include "godot/memory/memory_graph_resource.h"

#include "godot/tasks/task_graph_resource.h"
#include "godot/tasks/task_graph_host.h" 

// UI/Editor Handshake
#include "godot/editor/ideam_editor_plugin.h"
#include "godot/editor/ideam_editor_inspector_plugin.h"

// Native task registration
#include "core/tasks/native_task_registry.h"

using namespace godot;

void initialize_ideam_core_module(ModuleInitializationLevel p_level) {
	// ========================================================================
	// SCENE LEVEL
	// Register all runtime Nodes, Resources, and RefCounted objects here.
	// These are available in the running game and the exported binary.
	// ========================================================================
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		
		GDREGISTER_CLASS(ideam::godot_ext::MemoryBufferResource);
		GDREGISTER_CLASS(ideam::godot_ext::ManagedBufferProfile);
		GDREGISTER_CLASS(ideam::godot_ext::MemoryManagerResource);
		
		// Foundational Base Classes - Registered as Abstract to protect constructors
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamGraphResource);
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::MemoryGraphResource);
		
        // Graph UI elements needed at runtime/scene level
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphEdit);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphNode);

		GDREGISTER_CLASS(ideam::godot_ext::TaskGraphResource);
		GDREGISTER_CLASS(ideam::godot_ext::TaskGraphHost);

		// --- Core Native Task Registration ---
		ideam::core::NativeTaskRegistry::init();
		ideam::core::NativeTaskRegistry::register_task<ideam::core::TestTask>("TestTask");
	}

	// ========================================================================
	// EDITOR LEVEL
	// Register all Tool, Inspector, and Graph UI nodes here.
	// These are stripped from the final exported binary.
	// ========================================================================
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		
		// Ecosystem Base Classes - Registered as Abstract to expose API but block direct instantiation
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamEditorPlugin);
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamEditorInspectorPlugin);
		
        // Editor-only Graph tooling
		GDREGISTER_CLASS(ideam::godot_ext::GraphComposer);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphInspector);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphsPlugin);
	}
}

void uninitialize_ideam_core_module(ModuleInitializationLevel p_level) {
	// Add teardown logic here if your Singletons or static managers need explicit cleanup.
	// Failing to clean up allocated static pointers here will cause Godot to flag memory 
	// leaks in the console on exit.
	
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// Cleanup Scene level statics/singletons
		
		// Flush the static factories and dictionaries to prevent Godot memory leak warnings on exit
		ideam::core::NativeTaskRegistry::cleanup();
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