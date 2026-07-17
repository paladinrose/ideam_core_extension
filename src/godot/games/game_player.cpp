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

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_player_profile", "profile"), &GamePlayer::set_player_profile);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_profile"), &GamePlayer::get_player_profile);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "player_profile", godot::PROPERTY_HINT_RESOURCE_TYPE, "GamePlayerProfile"), "set_player_profile", "get_player_profile");
    ADD_SIGNAL(godot::MethodInfo("player_profile_changed", godot::PropertyInfo(godot::Variant::OBJECT, "player_profile", godot::PROPERTY_HINT_RESOURCE_TYPE, "GamePlayerProfile")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_root", "new_root"), &GamePlayer::set_player_root);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_root"), &GamePlayer::get_player_root);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "player_root", godot::PROPERTY_HINT_NODE_TYPE, "Node"), "set_player_root", "get_player_root");
    ADD_SIGNAL(godot::MethodInfo("player_root_changed", godot::PropertyInfo(godot::Variant::OBJECT, "player_root", godot::PROPERTY_HINT_NODE_TYPE, "Node")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_agent_name", "name"), &GamePlayer::set_player_agent_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_agent_name"), &GamePlayer::get_player_agent_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "player_agent_name"), "set_player_agent_name", "get_player_agent_name");
    ADD_SIGNAL(godot::MethodInfo("player_agent_name_changed", godot::PropertyInfo(godot::Variant::STRING, "player_agent_name")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_allow_agent_reparenting", "allow"), &GamePlayer::set_allow_agent_reparenting);
    godot::ClassDB::bind_method(godot::D_METHOD("get_allow_agent_reparenting"), &GamePlayer::get_allow_agent_reparenting);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "allow_agent_reparenting"), "set_allow_agent_reparenting", "get_allow_agent_reparenting");
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_join_game_on_load", "join"), &GamePlayer::set_join_game_on_load);
    godot::ClassDB::bind_method(godot::D_METHOD("get_join_game_on_load"), &GamePlayer::get_join_game_on_load);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "join_game_on_load"), "set_join_game_on_load", "get_join_game_on_load");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_transition", "transition"), &GamePlayer::set_game_transition);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_transition"), &GamePlayer::get_game_transition);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_transition", godot::PROPERTY_HINT_NODE_TYPE, "SceneTransition"), "set_game_transition", "get_game_transition");
    ADD_SIGNAL(godot::MethodInfo("game_transition_changed", godot::PropertyInfo(godot::Variant::OBJECT, "game_transition", godot::PROPERTY_HINT_NODE_TYPE, "SceneTransition")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_board_transition", "transition"), &GamePlayer::set_board_transition);
    godot::ClassDB::bind_method(godot::D_METHOD("get_board_transition"), &GamePlayer::get_board_transition);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "board_transition", godot::PROPERTY_HINT_NODE_TYPE, "SceneTransition"), "set_board_transition", "get_board_transition");
    ADD_SIGNAL(godot::MethodInfo("board_transition_changed", godot::PropertyInfo(godot::Variant::OBJECT, "board_transition", godot::PROPERTY_HINT_NODE_TYPE, "SceneTransition")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_default_agent", "agent"), &GamePlayer::set_default_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("get_default_agent"), &GamePlayer::get_default_agent);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "default_agent", godot::PROPERTY_HINT_NODE_TYPE, "GameAgent"), "set_default_agent", "get_default_agent");
    ADD_SIGNAL(godot::MethodInfo("default_agent_changed", godot::PropertyInfo(godot::Variant::OBJECT, "default_agent", godot::PROPERTY_HINT_NODE_TYPE, "GameAgent")));
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_agent_node_assignments", "assignments"), &GamePlayer::set_agent_node_assignments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_agent_node_assignments"), &GamePlayer::get_agent_node_assignments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "agent_node_assignments", godot::PROPERTY_HINT_NODE_TYPE, "NodeRetargeter"), "set_agent_node_assignments", "get_agent_node_assignments");
    

    godot::ClassDB::bind_method(godot::D_METHOD("set_agent_signal_assignments", "assignments"), &GamePlayer::set_agent_signal_assignments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_agent_signal_assignments"), &GamePlayer::get_agent_signal_assignments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "agent_signal_assignments", godot::PROPERTY_HINT_NODE_TYPE, "SignalConnector"), "set_agent_signal_assignments", "get_agent_signal_assignments");

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
    godot::ClassDB::bind_method(godot::D_METHOD("set_game_player_manager", "gpm"), &GamePlayer::set_game_player_manager);
    godot::ClassDB::bind_method(godot::D_METHOD("find_and_control_player_agent", "on"), &GamePlayer::find_and_control_player_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("find_and_control_game_agents", "on"), &GamePlayer::find_and_control_game_agents);
    godot::ClassDB::bind_method(godot::D_METHOD("find_and_release_game_agents", "on"), &GamePlayer::find_and_release_game_agents);
    godot::ClassDB::bind_method(godot::D_METHOD("release_game_agent_at", "id"), &GamePlayer::release_game_agent_at);
    godot::ClassDB::bind_method(godot::D_METHOD("reparent_to_agent", "agent"), &GamePlayer::reparent_to_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("validate_agents"), &GamePlayer::validate_agents);
    godot::ClassDB::bind_method(godot::D_METHOD("save_data"), &GamePlayer::save_data);
    godot::ClassDB::bind_method(godot::D_METHOD("load_data", "data"), &GamePlayer::load_data);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_process_frame_find_game"), &GamePlayer::_on_process_frame_find_game);
    
    godot::ClassDB::bind_method(godot::D_METHOD("open_game_menu"), &GamePlayer::open_game_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("close_game_menu"), &GamePlayer::close_game_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("load_game_board_started", "game", "game_board_id"), &GamePlayer::load_game_board_started);
    godot::ClassDB::bind_method(godot::D_METHOD("load_game_board_completed", "game", "game_board"), &GamePlayer::load_game_board_completed);
    godot::ClassDB::bind_method(godot::D_METHOD("load_game_started", "game_id"), &GamePlayer::load_game_started);
    godot::ClassDB::bind_method(godot::D_METHOD("load_game_completed", "game"), &GamePlayer::load_game_completed);
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

void GamePlayer::set_player_profile(const godot::Ref<GamePlayerProfile>& p_profile) { 
    if (player_profile == p_profile) return;
    player_profile = p_profile; 
    emit_signal("player_profile_changed", player_profile);
}
godot::Ref<GamePlayerProfile> GamePlayer::get_player_profile() const { return player_profile; }

void GamePlayer::set_player_root(godot::Node* p_root) {
    if (p_root == _root) return;
    _root = p_root;
    emit_signal("player_root_changed", _root);
}

godot::Node* GamePlayer::get_player_root() const { return _root; }

void GamePlayer::set_player_agent_name(const godot::String& p_name) { 
    if (player_agent_name == p_name) return;
    player_agent_name = p_name; 
    emit_signal("player_agent_name_changed", player_agent_name);
}
godot::String GamePlayer::get_player_agent_name() const { return player_agent_name; }

void GamePlayer::set_allow_agent_reparenting(bool p_allow) { 
    if (allow_agent_reparenting == p_allow) return;
    allow_agent_reparenting = p_allow; 
}
bool GamePlayer::get_allow_agent_reparenting() const { return allow_agent_reparenting; }

void GamePlayer::set_join_game_on_load(bool p_join) { 
    if (join_game_on_load == p_join) return;
    join_game_on_load = p_join; 
}
bool GamePlayer::get_join_game_on_load() const { return join_game_on_load; }

void GamePlayer::set_game_transition(SceneTransition* p_transition) { 
    if (game_transition == p_transition) return;
    game_transition = p_transition; 
    emit_signal("game_transition_changed", game_transition);
}
SceneTransition* GamePlayer::get_game_transition() const { return game_transition; }

void GamePlayer::set_board_transition(SceneTransition* p_transition) { 
    if (board_transition == p_transition) return;
    board_transition = p_transition; 
    emit_signal("board_transition_changed", board_transition);
}
SceneTransition* GamePlayer::get_board_transition() const { return board_transition; }

void GamePlayer::set_default_agent(GameAgent* p_agent) { 
    if (default_agent == p_agent) return;
    default_agent = p_agent; 
    emit_signal("default_agent_changed", default_agent);
}
GameAgent* GamePlayer::get_default_agent() const { return default_agent; }

void GamePlayer::set_agent_node_assignments(NodeRetargeter* p_assignments) { 
    if (agent_node_assignments == p_assignments) return;
    agent_node_assignments = p_assignments; 
}
NodeRetargeter* GamePlayer::get_agent_node_assignments() const { return agent_node_assignments; }

void GamePlayer::set_agent_signal_assignments(SignalConnector* p_assignments) { 
    if (agent_signal_assignments == p_assignments) return;
    agent_signal_assignments = p_assignments; 
}
SignalConnector* GamePlayer::get_agent_signal_assignments() const { return agent_signal_assignments; }

void GamePlayer::find_game_player_manager(godot::Node* on) {
    // Requires GamePlayerManager to expose static singleton accessor
    GamePlayerManager* gpm = godot::Object::cast_to<GamePlayerManager>(get_tree()->get_first_node_in_group("GamePlayerManager"));
    if (gpm) {
        set_game_player_manager(gpm);
    }
}

void GamePlayer::set_game_player_manager(GamePlayerManager* gpm) {
    if (!gpm) return;
    
    if (!player_profile.is_valid()) {
        if (!profile_path.is_empty()) {
            gpm->load_player_profile(profile_path, this);
        }
        
        if (!player_profile.is_valid()) {
            player_profile = gpm->get_default_player_profile();
            profile_path = "";
        }
    }
    
    gpm->login_player(this);
}

void GamePlayer::find_current_game() {
    godot::Node* p = get_parent();
    Game* game_node = nullptr;
    
    // Tight loop traversing hierarchy; avoid dynamic string comparisons
    while (p != nullptr) {
        game_node = godot::Object::cast_to<Game>(p);
        if (game_node) {
            break;
        }
        p = p->get_parent();
    }
    
    if (game_node) {
        join_game(game_node);
    }
}

void GamePlayer::login() { emit_signal("logged_in"); }
void GamePlayer::logout() { emit_signal("logged_out"); }

void GamePlayer::join_game(Game* game) {
    if (!game || games.has(game)) return;
    
    games.append(game);
    
    game->connect("loading_game_board_started", godot::Callable(this, "load_game_board_started"));
    game->connect("game_board_loaded", godot::Callable(this, "load_game_board_completed"));
    
    godot::Node* gameMenuNode = game->find_child("Game_Menu", true, false);
    if (gameMenuNode) {
        GameMenu* gameMenu = godot::Object::cast_to<GameMenu>(gameMenuNode);
        if (gameMenu) {
            gameMenus.append(gameMenu);
            connect("game_menu_opened", godot::Callable(gameMenu, "open_menu"));
            connect("game_menu_closed", godot::Callable(gameMenu, "close_menu"));
        }
    }
    
    if (board_transition) {
        game->set_game_board_loader(board_transition);
    }
    
    emit_signal("joined_game", game);
}

int GamePlayer::get_game_id(Game* game) const {
    return games.find(game);
}

void GamePlayer::leave_game(int gameID) {
    if (gameID < 0 || gameID >= games.size()) return;
    
    Game* gameLeft = godot::Object::cast_to<Game>(games[gameID]);
    if (gameLeft) {
        gameLeft->disconnect("loading_game_board_started", godot::Callable(this, "load_game_board_started"));
        gameLeft->disconnect("game_board_loaded", godot::Callable(this, "load_game_board_completed"));
            
        if (board_transition && gameLeft->get_game_board_loader() == board_transition) {
            gameLeft->set_game_board_loader(nullptr);
        }
    }

    if (gameID < gameMenus.size()) {
        GameMenu* menuLeft = godot::Object::cast_to<GameMenu>(gameMenus[gameID]);
        if (menuLeft) {
            disconnect("game_menu_opened", godot::Callable(menuLeft, "open_menu"));
            disconnect("game_menu_closed", godot::Callable(menuLeft, "close_menu"));
        }
    }
    
    games.remove_at(gameID);
    gameMenus.remove_at(gameID);
    
    emit_signal("left_game", gameLeft);
}

void GamePlayer::open_game_menu() { emit_signal("game_menu_opened"); }
void GamePlayer::close_game_menu() { emit_signal("game_menu_closed"); }

void GamePlayer::load_game_board_started(Game* game, int game_board_id) { emit_signal("loading_game_board_started"); }
void GamePlayer::load_game_board_completed(Game* game, GameBoard* game_board) { emit_signal("game_board_loaded", game_board); }

void GamePlayer::load_game_started(int game_id) { emit_signal("loading_game_started"); }

void GamePlayer::load_game_completed(Game* game) {
    if (join_game_on_load) {
        join_game(game);
    }
    emit_signal("game_loaded", game);
}

void GamePlayer::find_and_control_player_agent(godot::Node* on) {
    if (!on) return;
    godot::TypedArray<godot::Node> ga = on->find_children(player_agent_name);
    for (int i = 0; i < ga.size(); ++i) {
        GameAgent* game_agent = godot::Object::cast_to<GameAgent>(ga[i]);
        if (game_agent) {
            control_game_agent(game_agent);
        }
    }
}

void GamePlayer::find_and_control_game_agents(godot::Node* on) {
    if (!on) return;
    godot::TypedArray<godot::Node> ga = on->find_children("*", "GameAgent");
    for (int i = 0; i < ga.size(); ++i) {
        GameAgent* game_agent = godot::Object::cast_to<GameAgent>(ga[i]);
        if (game_agent) {
            control_game_agent(game_agent);
        }
    }
}

void GamePlayer::find_and_release_game_agents(godot::Node* on) {
    if (!on) return;
    godot::TypedArray<godot::Node> ga = on->find_children("*", "GameAgent");
    for (int i = 0; i < ga.size(); ++i) {
        GameAgent* game_agent = godot::Object::cast_to<GameAgent>(ga[i]);
        if (game_agent) {
            release_game_agent(game_agent);
        }
    }
}

void GamePlayer::control_game_agent(GameAgent* new_agent) {
    if (!new_agent || controlled_agents.has(new_agent)) return;

    if (controlled_agents.size() == 0) {
        controlled_agents.append(new_agent);
        _connect_to_agent(new_agent);
        return;
    }

    AgentExclusivity exclusivity = new_agent->get_exclusivity();

    for (int i = controlled_agents.size() - 1; i >= 0; --i) {
        GameAgent* av = godot::Object::cast_to<GameAgent>(controlled_agents[i]);
        if (!av) continue;

        bool do_disconnect = false;

        if (exclusivity == AgentExclusivity::EXCLUSIVE) {
            do_disconnect = true;
        } else if (exclusivity == AgentExclusivity::GROUP_INCLUSIVE) {
            do_disconnect = true;
            godot::TypedArray<godot::StringName> groups = new_agent->get_groups();
            for (int g = 0; g < groups.size(); ++g) {
                if (av->is_in_group(groups[g])) {
                    do_disconnect = false;
                    break;
                }
            }
        } else if (exclusivity == AgentExclusivity::GROUP_EXCLUSIVE) {
            do_disconnect = false;
            godot::TypedArray<godot::StringName> groups = new_agent->get_groups();
            for (int g = 0; g < groups.size(); ++g) {
                if (av->is_in_group(groups[g])) {
                    do_disconnect = true;
                    break;
                }
            }
        }

        if (do_disconnect) {
            _disconnect_from_agent(av);
            controlled_agents.remove_at(i);
        }
    }

    controlled_agents.append(new_agent);
    
    _connect_to_agent(new_agent);

    emit_signal("agent_controlled", new_agent);
}

void GamePlayer::release_game_agent(GameAgent* toRelease) {
    int agentID = controlled_agents.find(toRelease);
    if (agentID >= 0) release_game_agent_at(agentID);
}

void GamePlayer::release_game_agent_at(int id) {
    if (id < 0 || id >= controlled_agents.size()) return;
    
    GameAgent* agent = godot::Object::cast_to<GameAgent>(controlled_agents[id]);
    
    if (agent == default_agent && controlled_agents.size() == 1) {
        return;
    }
    
    if (agent && agent->get_player_parent_target() && _original_parent) {
        if (_root) {
            _root->reparent(_original_parent);
        } else {
            reparent(_original_parent);
        }
    }
    
    controlled_agents.remove_at(id);
    _disconnect_from_agent(agent);
    emit_signal("agent_released", agent);
    
    if (controlled_agents.size() == 0 && default_agent) {
        control_game_agent(default_agent);
    }
}

void GamePlayer::reparent_to_agent(GameAgent* agent) {
    if (!allow_agent_reparenting || !agent || !agent->get_player_parent_target() || !_root) return;
        
    godot::Node* current_parent = _root->get_parent();
        
    if (current_parent) {
        _root->reparent(agent->get_player_parent_target());
    } else {
        agent->get_player_parent_target()->add_child(_root);
    }
}

void GamePlayer::validate_agents() {
    if (default_agent && controlled_agents.size() == 0) {
        controlled_agents.append(default_agent);
    }
    
    for (int i = 0; i < controlled_agents.size(); ++i) {
        GameAgent* agent = godot::Object::cast_to<GameAgent>(controlled_agents[i]);
        if (agent && agent->get_player() != this) {
            _connect_to_agent(agent);
        }
    }
}

void GamePlayer::_connect_to_agent(GameAgent* agent) {
    if (!agent) return;

    if (agent->get_player_parent_target()) {
        reparent_to_agent(agent);
    }
    
    if (agent_node_assignments) {
        agent_node_assignments->retarget(this, agent);
    }
    
    if (agent_signal_assignments) {
        agent_signal_assignments->connect_signals(this, agent);
    }
    
    agent->set_player(this);
}

void GamePlayer::_disconnect_from_agent(GameAgent* agent) {
    if (!agent) return;

    if (agent_node_assignments) {
        agent_node_assignments->clear_set_targets(this, agent);
    }
    
    if (agent_signal_assignments) {
        agent_signal_assignments->disconnect_signals(this, agent);
    }

    if (agent->get_player() == this) {
        agent->set_player(nullptr);
    }
}

godot::Dictionary GamePlayer::save_data() const {
    return godot::Dictionary();
}

void GamePlayer::load_data(const godot::Dictionary& data) {}

} // namespace ideam::godot_ext