#include "game_board.h"
#include <godot_cpp/core/class_db.hpp>

#include "../game.h"
#include "game_agent.h"
#include "game_piece.h"

namespace ideam::godot_ext {

void GameBoard::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("game_agent_added", godot::PropertyInfo(godot::Variant::OBJECT, "game_agent", godot::PROPERTY_HINT_NODE_TYPE, "GameAgent")));
    ADD_SIGNAL(godot::MethodInfo("game_agent_removed", godot::PropertyInfo(godot::Variant::OBJECT, "game_agent", godot::PROPERTY_HINT_NODE_TYPE, "GameAgent")));
    
    ADD_SIGNAL(godot::MethodInfo("game_piece_added", godot::PropertyInfo(godot::Variant::OBJECT, "game_piece", godot::PROPERTY_HINT_NODE_TYPE, "GamePiece")));
    ADD_SIGNAL(godot::MethodInfo("game_piece_removed", godot::PropertyInfo(godot::Variant::OBJECT, "game_piece", godot::PROPERTY_HINT_NODE_TYPE, "GamePiece")));
    
    ADD_SIGNAL(godot::MethodInfo("loaded_from_game", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_NODE_TYPE, "Game")));
    ADD_SIGNAL(godot::MethodInfo("board_reset_requested"));

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_game_agents", "agents"), &GameBoard::set_game_agents);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_agents"), &GameBoard::get_game_agents);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "game_agents", godot::PROPERTY_HINT_ARRAY_TYPE, "GameAgent"), "set_game_agents", "get_game_agents");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_pieces", "pieces"), &GameBoard::set_game_pieces);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_pieces"), &GameBoard::get_game_pieces);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "game_pieces", godot::PROPERTY_HINT_ARRAY_TYPE, "GamePiece"), "set_game_pieces", "get_game_pieces");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("request_quit_game"), &GameBoard::request_quit_game);
    godot::ClassDB::bind_method(godot::D_METHOD("request_pause_game"), &GameBoard::request_pause_game);
    godot::ClassDB::bind_method(godot::D_METHOD("reset_game_board"), &GameBoard::reset_game_board);

    godot::ClassDB::bind_method(godot::D_METHOD("gather_game_agent_titles"), &GameBoard::gather_game_agent_titles);
    godot::ClassDB::bind_method(godot::D_METHOD("add_game_agent", "new_game_agent"), &GameBoard::add_game_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_agent_id", "game_agent"), &GameBoard::get_game_agent_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_game_agent", "game_agent"), &GameBoard::has_game_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_game_agent", "game_agent"), &GameBoard::remove_game_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_game_agent_at", "game_agent_ID"), &GameBoard::remove_game_agent_at);

    godot::ClassDB::bind_method(godot::D_METHOD("gather_game_piece_titles"), &GameBoard::gather_game_piece_titles);
    godot::ClassDB::bind_method(godot::D_METHOD("add_game_piece", "new_game_piece"), &GameBoard::add_game_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_piece_id", "game_piece"), &GameBoard::get_game_piece_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_game_piece", "game_piece"), &GameBoard::has_game_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_game_piece", "game_piece"), &GameBoard::remove_game_piece);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_game_piece_at", "game_piece_ID"), &GameBoard::remove_game_piece_at);

    godot::ClassDB::bind_method(godot::D_METHOD("get_controlling_agents", "game_piece"), &GameBoard::get_controlling_agents);
}

GameBoard::GameBoard() {}

GameBoard::~GameBoard() {}

// Setters / Getters
void GameBoard::set_game_agents(const godot::TypedArray<GameAgent>& p_agents) { game_agents = p_agents; }
godot::TypedArray<GameAgent> GameBoard::get_game_agents() const { return game_agents; }

void GameBoard::set_game_pieces(const godot::TypedArray<GamePiece>& p_pieces) { game_pieces = p_pieces; }
godot::TypedArray<GamePiece> GameBoard::get_game_pieces() const { return game_pieces; }

// Game State Methods
void GameBoard::request_quit_game() {
    Game* game = get_game(); // Derived from GameEntity base
    if (game && game->has_method("quit_game")) {
        game->call("quit_game");
    }
}

void GameBoard::request_pause_game() {
    Game* game = get_game();
    if (game && game->has_method("pause_game")) {
        game->call("pause_game");
    }
}

void GameBoard::reset_game_board() {
    emit_signal("board_reset_requested");
}

// Game Agent Functions
godot::TypedArray<godot::String> GameBoard::gather_game_agent_titles() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < game_agents.size(); ++i) {
        GameAgent* agent = godot::Object::cast_to<GameAgent>(game_agents[i]);
        if (agent) {
            names.append(agent->get_title()); // Assuming get_title() inherited from GameEntity
        }
    }
    return names;
}

int GameBoard::add_game_agent(GameAgent* new_game_agent) {
    int id = get_game_agent_id(new_game_agent);
    if (id < 0) {
        id = game_agents.size();
        game_agents.append(new_game_agent);
        emit_signal("game_agent_added", new_game_agent);
    }
    return id;
}

int GameBoard::get_game_agent_id(GameAgent* game_agent) const {
    return game_agents.find(game_agent);
}

bool GameBoard::has_game_agent(GameAgent* game_agent) const {
    return get_game_agent_id(game_agent) >= 0;
}

bool GameBoard::remove_game_agent(GameAgent* game_agent) {
    int id = get_game_agent_id(game_agent);
    if (id >= 0) {
        return remove_game_agent_at(id);
    }
    return false;
}

bool GameBoard::remove_game_agent_at(int game_agent_ID) {
    if (game_agent_ID < 0 || game_agent_ID >= game_agents.size()) {
        return false;
    }
    GameAgent* game_agent = godot::Object::cast_to<GameAgent>(game_agents[game_agent_ID]);
    
    // DOD NOTE: O(n) array shift operation here. Use standard vector swap-and-pop if order doesn't matter.
    game_agents.remove_at(game_agent_ID); 
    
    emit_signal("game_agent_removed", game_agent);
    return true;
}

// Game Piece Functions
godot::TypedArray<godot::String> GameBoard::gather_game_piece_titles() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < game_pieces.size(); ++i) {
        godot::Object* piece = game_pieces[i];
        if (piece && piece->has_method("get_title")) {
            names.append(piece->call("get_title"));
        }
    }
    return names;
}

int GameBoard::add_game_piece(GamePiece* new_game_piece) {
    int id = get_game_piece_id(new_game_piece);
    if (id < 0) {
        id = game_pieces.size();
        game_pieces.append(new_game_piece);
        emit_signal("game_piece_added", new_game_piece);
    }
    return id;
}

int GameBoard::get_game_piece_id(GamePiece* game_piece) const {
    return game_pieces.find(game_piece);
}

bool GameBoard::has_game_piece(GamePiece* game_piece) const {
    return get_game_piece_id(game_piece) >= 0;
}

bool GameBoard::remove_game_piece(GamePiece* game_piece) {
    int id = get_game_piece_id(game_piece);
    if (id >= 0) {
        return remove_game_piece_at(id);
    }
    return false;
}

bool GameBoard::remove_game_piece_at(int game_piece_ID) {
    if (game_piece_ID < 0 || game_piece_ID >= game_pieces.size()) {
        return false;
    }
    
    GamePiece* game_piece = godot::Object::cast_to<GamePiece>(game_pieces[game_piece_ID]);
    game_pieces.remove_at(game_piece_ID);
    
    emit_signal("game_piece_removed", game_piece);
    return true;
}

godot::TypedArray<GameAgent> GameBoard::get_controlling_agents(GamePiece* game_piece) const {
    godot::TypedArray<GameAgent> controlling_agents;
    for (int i = 0; i < game_agents.size(); ++i) {
        GameAgent* agent = godot::Object::cast_to<GameAgent>(game_agents[i]);
        if (agent && agent->has_game_piece(game_piece)) {
            controlling_agents.append(agent);
        }
    }
    return controlling_agents;
}

// Function Overrides
void GameBoard::enter_game() {
    GameEntity::enter_game();
    
    Game* game = get_game();
    for (int i = 0; i < game_agents.size(); ++i) {
        if (GameAgent* agent = godot::Object::cast_to<GameAgent>(game_agents[i])) agent->set_game(game);
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        if (godot::Object* piece = game_pieces[i]) piece->call("set_game", game); // Assuming GamePiece implementation
    }
}

void GameBoard::game_start() {
    GameEntity::game_start();
    
    for (int i = 0; i < game_agents.size(); ++i) {
        if (GameAgent* agent = godot::Object::cast_to<GameAgent>(game_agents[i])) agent->game_start();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        if (godot::Object* piece = game_pieces[i]) piece->call("game_start");
    }
}

void GameBoard::game_pause() {
    // Note: Assuming `entity_is_paused` is exposed via getter or protected in GameEntity
    // if (get_entity_is_paused()) return;
    GameEntity::game_pause();
    
    for (int i = 0; i < game_agents.size(); ++i) {
        if (GameAgent* agent = godot::Object::cast_to<GameAgent>(game_agents[i])) agent->game_pause();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        if (godot::Object* piece = game_pieces[i]) piece->call("game_pause");
    }
}

void GameBoard::game_continue() {
    // if (!get_entity_is_paused()) return;
    GameEntity::game_continue();
    
    for (int i = 0; i < game_agents.size(); ++i) {
        if (GameAgent* agent = godot::Object::cast_to<GameAgent>(game_agents[i])) agent->game_continue();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        if (godot::Object* piece = game_pieces[i]) piece->call("game_continue");
    }
}

void GameBoard::game_end() {
    for (int i = 0; i < game_agents.size(); ++i) {
        if (GameAgent* agent = godot::Object::cast_to<GameAgent>(game_agents[i])) agent->game_end();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        if (godot::Object* piece = game_pieces[i]) piece->call("game_end");
    }
    GameEntity::game_end();
}

void GameBoard::exit_game() {
    for (int i = 0; i < game_agents.size(); ++i) {
        if (GameAgent* agent = godot::Object::cast_to<GameAgent>(game_agents[i])) agent->exit_game();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        if (godot::Object* piece = game_pieces[i]) piece->call("exit_game");
    }
    GameEntity::exit_game();
}

void GameBoard::game_process(double delta) {
    // Note: Assuming `game_processed` tracking boolean exists in GameEntity scope
    // if (game_processed) return; 
    
    double board_delta = delta * get_time_scale();
    
    for (int i = 0; i < game_agents.size(); ++i) {
        if (GameAgent* agent = godot::Object::cast_to<GameAgent>(game_agents[i])) agent->game_process(board_delta);
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        if (godot::Object* piece = game_pieces[i]) piece->call("game_process", board_delta);
    }
    
    // game_processed = true;
}

void GameBoard::game_process_clear() {
    for (int i = 0; i < game_agents.size(); ++i) {
        if (GameAgent* agent = godot::Object::cast_to<GameAgent>(game_agents[i])) agent->game_process_clear();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        if (godot::Object* piece = game_pieces[i]) piece->call("game_process_clear");
    }
    // game_processed = false;
}

godot::Dictionary GameBoard::save_data() const {
    return GameEntity::save_data();
}

void GameBoard::load_data(const godot::Dictionary& data) {
    GameEntity::load_data(data);
}

} // namespace ideam::godot_ext