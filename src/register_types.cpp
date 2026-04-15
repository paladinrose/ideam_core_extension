#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

// UI/Editor Handshake
#include "godot/editor/ideam_editor_plugin.h"
#include "godot/editor/ideam_editor_inspector_plugin.h"

// --- Graph UI & Editor ---
#include "godot/graphs/ideam_graphs_plugin.h"

#include "godot/graphs/ideam_graph_resource.h"
#include "godot/graphs/ideam_graph_edit.h"
#include "godot/graphs/ideam_graph_node.h"
#include "godot/graphs/ideam_graph_inspector.h"
#include "godot/graphs/graph_composer.h"

// --- Memory UI & Editor ---
#include "godot/memory/ideam_memory_plugin.h"

#include "godot/memory/memory_graph_edit.h"
#include "godot/memory/memory_graph_node.h"
#include "godot/memory/memory_graph_inspector.h"
#include "godot/memory/memory_buffer_resource.h"
#include "godot/memory/managed_buffer_profile.h"
#include "godot/memory/memory_manager_resource.h"
#include "godot/memory/memory_graph_resource.h"

// --- Tasks UI & Editor ---
#include "godot/tasks/ideam_tasks_plugin.h"

#include "godot/tasks/task_graph_edit.h"
#include "godot/tasks/task_graph_node.h"
#include "godot/tasks/task_graph_inspector.h"
#include "godot/tasks/task_graph_resource.h"
#include "godot/tasks/task_graph_host.h" 

// Native task registration
#include "core/tasks/native_task_registry.h"
// --- Views ---
#include "core/memory/views/single_element_view.h"
#include "core/memory/views/multi_element_view.h"
#include "core/memory/views/aosoa_view.h"
#include "core/memory/views/atomic_view.h"
#include "core/memory/views/paged_view.h"
#include "core/memory/views/ring_view.h"
#include "core/memory/views/sparse_set_view.h"
#include "core/memory/views/static_stencil_view.h"
#include "core/memory/views/stencil_view.h"
#include "core/memory/views/swap_view.h"

// --- Strategies ---
#include "core/memory/views/strategies.h"


// --- Narratives UI & Editor ---
#include "godot/narratives/narreme.h"
#include "godot/narratives/narremes/character.h"
#include "godot/narratives/narremes/location.h"
#include "godot/narratives/narremes/prop.h"
#include "godot/narratives/narremes/incident.h"
#include "godot/narratives/narremes/plot.h"
#include "godot/narratives/narremes/narrative.h"

#include "godot/narratives/helpers/relationship.h"
#include "godot/narratives/helpers/incident_condition.h"
#include "godot/narratives/helpers/causal_condition.h"
#include "godot/narratives/helpers/gameplay_condition.h"
#include "godot/narratives/helpers/narrative_condition.h"
#include "godot/narratives/helpers/plot_event.h"

// Utility Wrapper for the QueryOp enum to be type-passed to the MatrixBuilder
template <ideam::core::QueryOp Op>
struct QueryOpTag { 
    static constexpr ideam::core::QueryOp value = Op; 
};

using namespace godot;
using namespace ideam::core;
void initialize_ideam_core_module(ModuleInitializationLevel p_level) {
	// ========================================================================
	// SCENE LEVEL
	// Register all runtime Nodes, Resources, and RefCounted objects here.
	// These are available in the running game and the exported binary.
	// ========================================================================
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		
		// Foundational Base Classes - Registered as Abstract to protect constructors
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamGraphResource);
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::MemoryGraphResource);
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::Narreme);
		
        // Graph UI elements needed at runtime/scene level
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphEdit);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphNode);

		// Memory UI elements needed at runtime/scene level
		GDREGISTER_CLASS(ideam::godot_ext::MemoryBufferResource);
		GDREGISTER_CLASS(ideam::godot_ext::ManagedBufferProfile);
		GDREGISTER_CLASS(ideam::godot_ext::MemoryManagerResource);
		GDREGISTER_CLASS(ideam::godot_ext::MemoryGraphEdit);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryGraphNode);

		// Task UI elements needed at runtime/scene level
		GDREGISTER_CLASS(ideam::godot_ext::TaskGraphResource);
		GDREGISTER_CLASS(ideam::godot_ext::TaskGraphHost);
		GDREGISTER_CLASS(ideam::godot_ext::TaskGraphEdit);
        GDREGISTER_CLASS(ideam::godot_ext::TaskGraphNode);
		
		// --- Narratives ---
		GDREGISTER_CLASS(ideam::godot_ext::Narrative);
		GDREGISTER_CLASS(ideam::godot_ext::Character);
		GDREGISTER_CLASS(ideam::godot_ext::Location);
		GDREGISTER_CLASS(ideam::godot_ext::Prop);
		GDREGISTER_CLASS(ideam::godot_ext::Incident);
		GDREGISTER_CLASS(ideam::godot_ext::Plot);

		// --- Narrative Helper Classes ---
		GDREGISTER_CLASS(ideam::godot_ext::Relationship);
		GDREGISTER_CLASS(ideam::godot_ext::Incident_Condition);
		GDREGISTER_CLASS(ideam::godot_ext::Causal_Condition);
		GDREGISTER_CLASS(ideam::godot_ext::Gameplay_Condition);
		GDREGISTER_CLASS(ideam::godot_ext::Narrative_Condition);
		GDREGISTER_CLASS(ideam::godot_ext::Plot_Event);

		// --- Native Task Registration ---
		ideam::core::NativeTaskRegistry::init();
		ideam::core::NativeTaskRegistry::register_task<ideam::core::EntryFillTask>("Entry Fill Task");

		// 1. Comprehensive Operations
    	// StochasticQueryLogic supports both CULL and ADD out of the box.
		using QueryOpsMatrix = std::tuple<
			QueryOpTag<QueryOp::CULL>, 
			QueryOpTag<QueryOp::ADD>
		>;

		// 2. Comprehensive Data Types (Stochastic Logics)
		// Registering the core DOD primitives defined in MemoryUtilities::get_type_byte_size.
		// Using explicit sizes guarantees uniform stride boundaries for the View iterators.
		using StochasticLogicsMatrix = std::tuple<
			StochasticQueryLogic<float>,
			StochasticQueryLogic<double>,
			StochasticQueryLogic<int32_t>,
			StochasticQueryLogic<int64_t>,
			StochasticQueryLogic<uint8_t>, // Mapped to DataType::BYTE
			StochasticQueryLogic<bool>     // Mapped to DataType::BOOL
		>;

		// 3. Comprehensive Strategies
		// The permutations of how the execution wave resolves the actual memory pointers.
		using StrategiesMatrix = std::tuple<
			FlatStrategy,
			SoAStrategy,
			AoSStrategy,
			Spatial2DStrategy,
			Spatial3DStrategy,
			Spatial4DStrategy,
			TiledSoAStrategy,
			RingStrategy,
			PagedStrategy
		>;

		// 4. Comprehensive Views
		// Lenses into the memory footprint. Supplying base primitive float as the anchor for the generic setup.
		using ViewsMatrix = std::tuple<
			SingleElementView<float>,
			MultiElementView<AoSStrategy>,
			AOSOAView<float>,
			AtomicView<float>,
			PagedView<float>,
			RingView<float>,
			SparseSetView<float>,
			SwapView<float>,

			StencilView<float, Spatial2DStrategy, 2>,
			StencilView<float, Spatial3DStrategy, 3>,
			StencilView<float, Spatial4DStrategy, 4>,

			StaticStencilView<float, Spatial2DStrategy, 5>,   // e.g., 2D Von Neumann (Center + 4 dirs)
			StaticStencilView<float, Spatial2DStrategy, 9>,   // e.g., 2D Moore Radius 1 (3x3 grid)
			StaticStencilView<float, Spatial3DStrategy, 27>,  // e.g., 3D Moore Radius 1 (3x3x3 grid)
			StaticStencilView<float, Spatial4DStrategy, 81>   // e.g., 4D Moore Radius 1 (3x3x3x3 grid)
			
		>;

		// 5. Build the Factory Matrix
		// This unfolds the tuples via fold expressions in NativeTaskRegistry, computing the `constexpr`
		// validity of every (Logic x Op x View x Strategy) combination at compile-time.
		NativeTaskRegistry::QueryMatrixBuilder<
			StochasticLogicsMatrix, 
			QueryOpsMatrix, 
			ViewsMatrix, 
			StrategiesMatrix
		>::build();
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

		// Editor-only Memory tooling
		GDREGISTER_CLASS(ideam::godot_ext::MemoryGraphInspector);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamMemoryPlugin);

		// Editor-only Tasks tooling
		GDREGISTER_CLASS(ideam::godot_ext::TaskGraphInspector);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamTasksPlugin);
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