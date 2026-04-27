#include "game_player.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "game_player_manager.h"
#include "game_player_profile.h"
#include "game.h"
#include "game_entities/game_board.h"
#include "game_entities/game_agent.h"

#include "ui/game_menu.h"

#include "scene_transition.h"

#include "../utilities/node_retargeter.h"
#include "../utilities/signal_connector.h"

namespace ideam::godot_ext {

void GamePlayer::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("logged_in"));
    ADD_SIGNAL(godot::MethodInfo("logged_out"));
    ADD_SIGNAL(godot::MethodInfo("joined_game", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_NODE_TYPE, "Game")));
    ADD_SIGNAL(godot::MethodInfo("left_game", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_NODE_TYPE, "Game")));
    ADD_SIGNAL(godot::MethodInfo("game_menu_opened"));
    ADD_SIGNAL(godot::MethodInfo("game_menu_closed"));
    ADD_SIGNAL(godot::MethodInfo("agent_controlled", godot::PropertyInfo(godot::Variant::OBJECT, "agent", godot::PROPERTY_HINT_NODE_TYPE, "GameAgent")));
    ADD_SIGNAL(godot::MethodInfo("agent_released", godot::PropertyInfo(godot::Variant::OBJECT, "agent", godot::PROPERTY_HINT_NODE_TYPE, "GameAgent")));
    ADD_SIGNAL(godot::MethodInfo("loading_game_board_started"));
    ADD_SIGNAL(godot::MethodInfo("game_board_loaded", godot::PropertyInfo(godot::Variant::OBJECT, "game_board", godot::PROPERTY_HINT_NODE_TYPE, "GameBoard")));
    ADD_SIGNAL(godot::MethodInfo("loading_game_started"));
    ADD_SIGNAL(godot::MethodInfo("game_loaded", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_NODE_TYPE, "Game")));

    // Properties (Abridged binding logic for standard exports)
    godot::ClassDB::bind_method(godot::D_METHOD("set_player_root", "new_root"), &GamePlayer::set_player_root);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_root"), &GamePlayer::get_player_root);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "player_root", godot::PROPERTY_HINT_NODE_TYPE, "Node"), "set_player_root", "get_player_root");

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_agent_name", "name"), &GamePlayer::set_player_agent_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_agent_name"), &GamePlayer::get_player_agent_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "player_agent_name"), "set_player_agent_name", "get_player_agent_name");

    // Additional typical properties... (allow_agent_reparenting, join_game_on_load, default_agent, etc.)

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("find_game_player_manager", "on"), &GamePlayer::find_game_player_manager, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("find_current_game"), &GamePlayer::find_current_game);
    godot::ClassDB::bind_method(godot::D_METHOD("login"), &GamePlayer::login);
    godot::ClassDB::bind_method(godot::D_METHOD("logout"), &GamePlayer::logout);
    godot::ClassDB::bind_method(godot::D_METHOD("join_game", "game"), &GamePlayer::join_game);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_id", "game"), &GamePlayer::get_game_id);
    godot::ClassDB::bind_method(godot::D_METHOD("leave_game", "gameID"), &GamePlayer::leave_game);
    godot::ClassDB::bind_method(godot::D_METHOD("control_game_agent", "new_agent"), &GamePlayer::control_game_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("release_game_agent", "toRelease"), &GamePlayer::release_game_agent);
    
    godot::ClassDB::bind_method(godot::D_METHOD("_on_process_frame_find_game"), &GamePlayer::_on_process_frame_find_game);
}

GamePlayer::GamePlayer() {}

GamePlayer::~GamePlayer() {}

void GamePlayer::_ready() {
    find_game_player_manager();
    
    if (join_game_on_load) {
        if (get_tree()) {
            get_tree()->connect("process_frame", godot::Callable(this, "_on_process_frame_find_game"), godot::Object::CONNECT_ONE_SHOT);
        }
    }
}

void GamePlayer::_on_process_frame_find_game() {
    find_current_game();
}

// Implement Setters/Getters
void GamePlayer::set_player_root(godot::Node* p_root) {
    if (p_root == _root) return;
    _root = p_root;
}
godot::Node* GamePlayer::get_player_root() const { return _root; }

void GamePlayer::set_player_agent_name(const godot::String& p_name) { player_agent_name = p_name; }
godot::String GamePlayer::get_player_agent_name() const { return player_agent_name; }

// ... other straightforward getters/setters omitted for concise DOD focus ...

void GamePlayer::find_game_player_manager(godot::Node* on) {
    // Assumption: GamePlayerManager::get_manager() exists as a static singleton accessor
    // GamePlayerManager* gpm = GamePlayerManager::get_manager();
    // if (gpm) set_game_player_manager(gpm);
}

void GamePlayer::set_game_player_manager(GamePlayerManager* gpm) {
    // Implementing translation from GDScript logic...
    // gpm->login_player(this);
}

void GamePlayer::find_current_game() {
    godot::Node* p = get_parent();
    while (p != nullptr && p->get_class() != "Game") {
        p = p->get_parent();
    }
    
    if (p && p->get_class() == "Game") {
        // join_game(godot::Object::cast_to<Game>(p));
    }
}

void GamePlayer::login() { emit_signal("logged_in"); }
void GamePlayer::logout() { emit_signal("logged_out"); }

void GamePlayer::join_game(Game* game) {
    if (!game || games.has(game)) return;
    
    games.append(game);
    
    // game->connect("loading_game_board_started", godot::Callable(this, "load_game_board_started"));
    // game->connect("game_board_loaded", godot::Callable(this, "load_game_board_completed"));
    
    godot::Node* gameMenuNode = game->find_child("GameMenu", true, false);
    if (gameMenuNode) {
        // GameMenu* gameMenu = godot::Object::cast_to<GameMenu>(gameMenuNode);
        // gameMenus.append(gameMenu);
    }
    
    emit_signal("joined_game", game);
}

int GamePlayer::get_game_id(Game* game) const {
    return games.find(game);
}

void GamePlayer::leave_game(int gameID) {
    if (gameID < 0 || gameID >= games.size()) return;
    
    Game* gameLeft = godot::Object::cast_to<Game>(games[gameID]);
    
    // Disconnections...
    
    // DOD NOTE: `TypedArray::remove_at` requires shifting all subsequent elements in memory. 
    // For large collections or frequent modifications, utilize `std::vector` and the 
    // swap-and-pop idiom: `std::swap(vec[id], vec.back()); vec.pop_back();` to guarantee O(1) removal.
    games.remove_at(gameID);
    gameMenus.remove_at(gameID);
    
    emit_signal("left_game", gameLeft);
}

// ... other menu/loading wrappers ...

void GamePlayer::control_game_agent(GameAgent* new_agent) {
    if (!new_agent || controlled_agents.has(new_agent)) return;

    if (controlled_agents.size() == 0) {
        controlled_agents.append(new_agent);
        _connect_to_agent(new_agent);
        return;
    }

    // DOD NOTE: String-based Godot Groups (`is_in_group`) allocate memory and perform hash map lookups.
    // By replacing groups with a 64-bit integer `collision_mask` or `group_mask` on the agent, 
    // the exclusivity checks below can be vectorized or evaluated branchlessly using pure SIMD bitwise AND ops.
    // e.g., `bool overlap = (av->group_mask & new_agent->group_mask) != 0;`

    int exclusivity = 0; // Assumption: new_agent->get_exclusivity(); 
                         // 0: EXCLUSIVE, 1: GROUP_INCLUSIVE, 2: GROUP_EXCLUSIVE

    for (int i = controlled_agents.size() - 1; i >= 0; --i) {
        GameAgent* av = godot::Object::cast_to<GameAgent>(controlled_agents[i]);
        if (!av) continue;

        bool do_disconnect = false;

        switch (exclusivity) {
            case 0: // EXCLUSIVE
                do_disconnect = true;
                break;
            case 1: // GROUP_INCLUSIVE
                do_disconnect = true;
                // godot::TypedArray<godot::StringName> groups = new_agent->get_groups();
                // Check if av shares ANY group... if so, do_disconnect = false;
                break;
            case 2: // GROUP_EXCLUSIVE
                do_disconnect = false;
                // godot::TypedArray<godot::StringName> groups = new_agent->get_groups();
                // Check if av shares ANY group... if so, do_disconnect = true;
                break;
        }

        if (do_disconnect) {
            _disconnect_from_agent(av);
            controlled_agents.remove_at(i);
        }
    }

    controlled_agents.append(new_agent);
    _connect_to_agent(new_agent);
}

void GamePlayer::release_game_agent(GameAgent* toRelease) {
    int agentID = controlled_agents.find(toRelease);
    if (agentID >= 0) release_game_agent_at(agentID);
}

void GamePlayer::release_game_agent_at(int id) {
    if (id < 0 || id >= controlled_agents.size()) return;
    
    GameAgent* agent = godot::Object::cast_to<GameAgent>(controlled_agents[id]);
    
    if (agent == default_agent && controlled_agents.size() == 1) {
        GameAgent* onlyAgent = godot::Object::cast_to<GameAgent>(controlled_agents[0]);
        if (onlyAgent == default_agent) {
            return;
        }
    }
    
    // if (agent->player_parent_target && _original_parent) reparent(_original_parent);
    
    controlled_agents.remove_at(id);
    _disconnect_from_agent(agent);
    
    if (controlled_agents.size() == 0 && default_agent) {
        control_game_agent(default_agent);
    }
}

void GamePlayer::_connect_to_agent(GameAgent* agent) {
    // Assumption implementations mapping logic
    // if (agent->player_parent_target) reparent_to_agent(agent);
    // if (agent_node_assignments) agent_node_assignments->retarget(this, agent);
    // if (agent_signal_assignments) agent_signal_assignments->connect_signals(this, agent);
    // agent->set_player(this);
}

void GamePlayer::_disconnect_from_agent(GameAgent* agent) {
    // if (agent_node_assignments) agent_node_assignments->clear_set_targets(this, agent);
    // if (agent_signal_assignments) agent_signal_assignments->disconnect_signals(this, agent);
}

godot::Dictionary GamePlayer::save_data() const {
    return godot::Dictionary();
}

void GamePlayer::load_data(const godot::Dictionary& data) {}

} // namespace ideam::godot_ext