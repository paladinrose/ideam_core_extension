#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GamePlayerProfile;
class SceneTransition;
class GameAgent;
class NodeRetargeter;
class SignalConnector;
class Game;
class GameMenu;
class GameBoard;
class GamePlayerManager;

class GamePlayer : public godot::Node {
    GDCLASS(GamePlayer, godot::Node)

protected:
    static void _bind_methods();

private:
    godot::Ref<GamePlayerProfile> player_profile;
    
    godot::Node* _root = nullptr;
    godot::String player_agent_name = "Player_Agent";
    bool allow_agent_reparenting = true;
    bool join_game_on_load = true;

    SceneTransition* game_transition = nullptr;
    SceneTransition* board_transition = nullptr;
    GameAgent* default_agent = nullptr;
    
    NodeRetargeter* agent_node_assignments = nullptr;
    SignalConnector* agent_signal_assignments = nullptr;

    // DOD NOTE: Storing Node pointers in TypedArray causes severe cache thrashing when iterating 
    // over these collections to evaluate exclusivity rules. In a DOD paradigm, `controlled_agents` 
    // should be replaced by a dense `std::vector<uint32_t>` containing purely structural IDs. 
    godot::TypedArray<GameAgent> controlled_agents;
    godot::TypedArray<Game> games;
    godot::TypedArray<GameMenu> gameMenus;

    godot::String profile_path;
    godot::Node* _original_parent = nullptr;

    // Internal async callback
    void _on_process_frame_find_game();

public:
    GamePlayer();
    ~GamePlayer();

    virtual void _ready() override;

    // Setters / Getters
    void set_player_profile(const godot::Ref<GamePlayerProfile>& p_profile);
    godot::Ref<GamePlayerProfile> get_player_profile() const;

    void set_player_root(godot::Node* p_root);
    godot::Node* get_player_root() const;

    void set_player_agent_name(const godot::String& p_name);
    godot::String get_player_agent_name() const;

    void set_allow_agent_reparenting(bool p_allow);
    bool get_allow_agent_reparenting() const;

    void set_join_game_on_load(bool p_join);
    bool get_join_game_on_load() const;

    void set_game_transition(SceneTransition* p_transition);
    SceneTransition* get_game_transition() const;

    void set_board_transition(SceneTransition* p_transition);
    SceneTransition* get_board_transition() const;

    void set_default_agent(GameAgent* p_agent);
    GameAgent* get_default_agent() const;

    void set_agent_node_assignments(NodeRetargeter* p_assignments);
    NodeRetargeter* get_agent_node_assignments() const;

    void set_agent_signal_assignments(SignalConnector* p_assignments);
    SignalConnector* get_agent_signal_assignments() const;

    // Class Functions
    void find_game_player_manager(godot::Node* on = nullptr);
    void set_game_player_manager(GamePlayerManager* gpm);
    void find_current_game();
    
    void login();
    void logout();
    
    void join_game(Game* game);
    int get_game_id(Game* game) const;
    void leave_game(int gameID);
    
    void open_game_menu();
    void close_game_menu();
    
    void load_game_board_started(Game* game, int game_board_id);
    void load_game_board_completed(Game* game, GameBoard* game_board);
    
    void load_game_started(int game_id);
    void load_game_completed(Game* game);

    // Agent Functions
    void find_and_control_player_agent(godot::Node* on);
    void find_and_control_game_agents(godot::Node* on);
    void find_and_release_game_agents(godot::Node* on);
    
    void control_game_agent(GameAgent* new_agent);
    void release_game_agent(GameAgent* toRelease);
    void release_game_agent_at(int id);
    void reparent_to_agent(GameAgent* agent);
    void validate_agents();

    void _connect_to_agent(GameAgent* agent);
    void _disconnect_from_agent(GameAgent* agent);
    
    godot::Dictionary save_data() const;
    void load_data(const godot::Dictionary& data);
};

} // namespace ideam::godot_ext