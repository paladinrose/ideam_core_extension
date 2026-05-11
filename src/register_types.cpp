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
#include "godot/graphs/ideam_graph_node_resource.h"
#include "godot/graphs/ideam_graph_edit.h"
#include "godot/graphs/ideam_graph_node.h"
#include "godot/graphs/ideam_graph_inspector.h"
#include "godot/graphs/graph_composer.h"

// --- Memory UI & Editor ---
#include "godot/memory/ideam_memory_plugin.h"

#include "godot/memory/memory_graph_edit.h"
#include "godot/memory/memory_graph_node.h"
#include "godot/memory/memory_graph_inspector.h"
#include "godot/memory/memory_inspectors.h"
#include "godot/memory/memory_buffer_resource.h"
#include "godot/memory/managed_buffer_profile.h"
#include "godot/memory/memory_manager_resource.h"
#include "godot/memory/memory_graph_resource.h"
#include "godot/memory/memory_graph_node_resource.h"
#include "godot/memory/memory_grant_resource.h"
#include "godot/memory/grant_part_resource.h"

// --- Tasks UI & Editor ---
#include "godot/tasks/ideam_tasks_plugin.h"

#include "godot/tasks/task_graph_edit.h"
#include "godot/tasks/task_graph_node.h"
#include "godot/tasks/task_graph_inspector.h"
#include "godot/tasks/task_graph_resource.h"
#include "godot/tasks/task_graph_node_resource.h"
#include "godot/tasks/task_graph_host.h" 

// Native task registration
#include "core/tasks/registration/native_task_registry.h"

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

#include "core/memory/views/stencil_math.h"

// --- Strategies ---
#include "core/memory/views/strategies.h"

// --- Narratives Plugin ---
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

// --- Games Plugin ---
#include "godot/games/ideam_games_plugin.h"

#include "godot/games/game_hub.h"
#include "godot/games/game.h"

#include "godot/games/game_player_manager.h"
#include "godot/games/game_player.h"
#include "godot/games/game_player_profile.h"

#include "godot/games/game_entity.h"
#include "godot/games/game_entities/game_board.h"
#include "godot/games/game_entities/game_agent.h"
#include "godot/games/game_entities/actions/game_agent_action.h"
#include "godot/games/game_entities/game_piece.h"
#include "godot/games/game_entities/actions/game_piece_action.h"

#include "godot/games/game_entities/actions/game_interaction.h"

// --- Games UI ---
#include "godot/games/ui/game_hub_ui.h"
#include "godot/games/ui/game_menu.h"
#include "godot/games/ui/game_options_menu.h"
#include "godot/games/ui/game_pause_menu.h"
#include "godot/games/ui/chapter_select.h"

#include "godot/games/ui/game_agent_action_tool.h"
#include "godot/games/ui/game_agent_action_sequencer.h"

// --- Games Editors ---
#include "godot/games/editor/game_agent_editor_inspector_plugin.h"
#include "godot/games/editor/game_agent_action_editor_inspector_plugin.h"

using namespace godot;
using namespace ideam::core;
void initialize_ideam_core_module(ModuleInitializationLevel p_level) {
	// ========================================================================
	// SCENE LEVEL
	// ========================================================================
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		
		// Foundational Base Classes - Registered as Abstract to protect constructors
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamGraphResource);
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamGraphNodeResource);
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::MemoryGraphResource);
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::MemoryGraphNodeResource);
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::Narreme);
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::GameEntity);
		
        // Graphs UI
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphEdit);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphNode);
		GDREGISTER_CLASS(ideam::godot_ext::GraphComposer);

		// Memory UI
		GDREGISTER_CLASS(ideam::godot_ext::MemoryBufferResource);
		GDREGISTER_CLASS(ideam::godot_ext::ManagedBufferProfile);
		GDREGISTER_CLASS(ideam::godot_ext::MemoryManagerResource);
		GDREGISTER_CLASS(ideam::godot_ext::MemoryGraphEdit);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryGraphNode);
		GDREGISTER_CLASS(ideam::godot_ext::MemoryGrantResource);
		GDREGISTER_CLASS(ideam::godot_ext::GrantPartResource);

		// Tasks UI 
		GDREGISTER_CLASS(ideam::godot_ext::TaskGraphResource);
		GDREGISTER_CLASS(ideam::godot_ext::TaskGraphNodeResource);
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

		// --- Games ---
		GDREGISTER_CLASS(ideam::godot_ext::Game);
		GDREGISTER_CLASS(ideam::godot_ext::GameHub);
		GDREGISTER_CLASS(ideam::godot_ext::GamePlayer);
		GDREGISTER_CLASS(ideam::godot_ext::GamePlayerProfile);
		GDREGISTER_CLASS(ideam::godot_ext::GamePlayerManager);
		GDREGISTER_CLASS(ideam::godot_ext::GameBoard);
		GDREGISTER_CLASS(ideam::godot_ext::GameAgent);
		GDREGISTER_CLASS(ideam::godot_ext::GameAgentAction);
		GDREGISTER_CLASS(ideam::godot_ext::GamePiece);
		GDREGISTER_CLASS(ideam::godot_ext::GamePieceAction);
		GDREGISTER_CLASS(ideam::godot_ext::GameInteraction);

		// --- Games UI ---
		GDREGISTER_CLASS(ideam::godot_ext::GameHubUI);
		GDREGISTER_CLASS(ideam::godot_ext::GameMenu);
		GDREGISTER_CLASS(ideam::godot_ext::GameOptionsMenu);
		GDREGISTER_CLASS(ideam::godot_ext::GamePauseMenu);
		GDREGISTER_CLASS(ideam::godot_ext::ChapterSelect);

		GDREGISTER_CLASS(ideam::godot_ext::GameAgentActionTool);
		GDREGISTER_CLASS(ideam::godot_ext::GameAgentActionSequencer);

		// --- Native Task Registration ---
		NativeTaskRegistry::init();
		
	}

	// ========================================================================
	// EDITOR LEVEL
	// ========================================================================
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		
		// Ecosystem Base Classes - Registered as Abstract to expose API but block direct instantiation
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamEditorPlugin);
		GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamEditorInspectorPlugin);
		
        // Graph tooling
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphInspector);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphsPlugin);

		// Memory tooling
		GDREGISTER_CLASS(ideam::godot_ext::MemoryGraphInspector);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamMemoryPlugin);

		// Tasks tooling
		GDREGISTER_CLASS(ideam::godot_ext::TaskGraphInspector);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamTasksPlugin);

		// --- Games Editors ---
		GDREGISTER_CLASS(ideam::godot_ext::GameAgentEditorInspectorPlugin);
		GDREGISTER_CLASS(ideam::godot_ext::GameAgentActionEditorInspectorPlugin);
		GDREGISTER_CLASS(ideam::godot_ext::IdeamGamesPlugin);
	}
}

void uninitialize_ideam_core_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// Flush the static factories and dictionaries to prevent Godot memory leak warnings on exit
		NativeTaskRegistry::cleanup();
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		
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