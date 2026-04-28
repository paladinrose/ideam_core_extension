#include "game.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>

// Explicit includes for strict typing
#include "game_entities/game_board.h"
#include "scene_transition.h"
#include "gameplay/gameplay_style.h"
#include "game_entities/actions/game_interaction.h"

namespace ideam::godot_ext {

void Game::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("loaded_from_game_hub"));
    ADD_SIGNAL(godot::MethodInfo("unloaded_by_game_hub"));
    ADD_SIGNAL(godot::MethodInfo("dependencies_loaded"));
    ADD_SIGNAL(godot::MethodInfo("game_started"));
    ADD_SIGNAL(godot::MethodInfo("new_game_started"));
    ADD_SIGNAL(godot::MethodInfo("game_ended", godot::PropertyInfo(godot::Variant::INT, "game_hub_id")));
    ADD_SIGNAL(godot::MethodInfo("game_paused"));
    ADD_SIGNAL(godot::MethodInfo("game_continued"));
    ADD_SIGNAL(godot::MethodInfo("game_processed", godot::PropertyInfo(godot::Variant::FLOAT, "deltaTime")));
    ADD_SIGNAL(godot::MethodInfo("game_file_saved"));
    ADD_SIGNAL(godot::MethodInfo("game_file_loaded"));
    ADD_SIGNAL(godot::MethodInfo("game_file_deleted"));
    ADD_SIGNAL(godot::MethodInfo("loading_game_board_started", godot::PropertyInfo(godot::Variant::OBJECT, "game"), godot::PropertyInfo(godot::Variant::INT, "boardID")));
    ADD_SIGNAL(godot::MethodInfo("game_board_loaded", godot::PropertyInfo(godot::Variant::OBJECT, "game"), godot::PropertyInfo(godot::Variant::OBJECT, "board")));
    ADD_SIGNAL(godot::MethodInfo("game_board_already_loaded", godot::PropertyInfo(godot::Variant::OBJECT, "game"), godot::PropertyInfo(godot::Variant::OBJECT, "board")));
    ADD_SIGNAL(godot::MethodInfo("game_board_not_found", godot::PropertyInfo(godot::Variant::OBJECT, "game"), godot::PropertyInfo(godot::Variant::INT, "boardID")));
    ADD_SIGNAL(godot::MethodInfo("game_board_disabled", godot::PropertyInfo(godot::Variant::OBJECT, "board")));
    ADD_SIGNAL(godot::MethodInfo("game_board_enabled", godot::PropertyInfo(godot::Variant::OBJECT, "board")));

    // Bind Constants / Enums
    BIND_ENUM_CONSTANT(GameState::UNINITIALIZED);
    BIND_ENUM_CONSTANT(GameState::PREGAME);
    BIND_ENUM_CONSTANT(GameState::PLAYING);
    BIND_ENUM_CONSTANT(GameState::PAUSED);
    BIND_ENUM_CONSTANT(GameState::RESOLUTION);
    BIND_ENUM_CONSTANT(GameState::COMPLETE);

    BIND_ENUM_CONSTANT(SaveOptions::NO_SAVE);
    BIND_ENUM_CONSTANT(SaveOptions::PLAYSTATE_SAVE);
    BIND_ENUM_CONSTANT(SaveOptions::SAVE_FILES);
    BIND_ENUM_CONSTANT(SaveOptions::FULL_SAVE);

    BIND_ENUM_CONSTANT(LoadOptions::NO_LOAD);
    BIND_ENUM_CONSTANT(LoadOptions::RESUME_PLAYSTATE);
    BIND_ENUM_CONSTANT(LoadOptions::LOAD_FILES);
    BIND_ENUM_CONSTANT(LoadOptions::FULL_LOAD);

    // Properties Bindings
    godot::ClassDB::bind_method(godot::D_METHOD("get_continue_play_state"), &Game::get_continue_play_state);
    
    godot::ClassDB::bind_method(godot::D_METHOD("set_game_root", "root"), &Game::set_game_root);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_root"), &Game::get_game_root);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_root", godot::PROPERTY_HINT_NODE_TYPE, "Node"), "set_game_root", "get_game_root");

    godot::ClassDB::bind_method(godot::D_METHOD("set_title", "title"), &Game::set_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_title"), &Game::get_title);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "title"), "set_title", "get_title");

    godot::ClassDB::bind_method(godot::D_METHOD("set_time_scale", "scale"), &Game::set_time_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_time_scale"), &Game::get_time_scale);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "time_scale"), "set_time_scale", "get_time_scale");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_state", "state"), &Game::set_game_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_state"), &Game::get_game_state);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "game_state", godot::PROPERTY_HINT_ENUM, "Uninitialized,Pregame,Playing,Paused,Resolution,Complete"), "set_game_state", "get_game_state");

    godot::ClassDB::bind_method(godot::D_METHOD("set_close_hub_on_quit", "close"), &Game::set_close_hub_on_quit);
    godot::ClassDB::bind_method(godot::D_METHOD("get_close_hub_on_quit"), &Game::get_close_hub_on_quit);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "close_hub_on_quit"), "set_close_hub_on_quit", "get_close_hub_on_quit");

    godot::ClassDB::bind_method(godot::D_METHOD("set_save_path", "path"), &Game::set_save_path);
    godot::ClassDB::bind_method(godot::D_METHOD("get_save_path"), &Game::get_save_path);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "save_path"), "set_save_path", "get_save_path");

    godot::ClassDB::bind_method(godot::D_METHOD("set_save_options", "options"), &Game::set_save_options);
    godot::ClassDB::bind_method(godot::D_METHOD("get_save_options"), &Game::get_save_options);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "save_options", godot::PROPERTY_HINT_ENUM, "No Save,Playstate Save,Save Files,Full Save"), "set_save_options", "get_save_options");

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_save_files", "max_files"), &Game::set_max_save_files);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_save_files"), &Game::get_max_save_files);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_save_files"), "set_max_save_files", "get_max_save_files");

    godot::ClassDB::bind_method(godot::D_METHOD("set_load_options", "options"), &Game::set_load_options);
    godot::ClassDB::bind_method(godot::D_METHOD("get_load_options"), &Game::get_load_options);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "load_options", godot::PROPERTY_HINT_ENUM, "No Load,Resume Playstate,Load Files,Full Load"), "set_load_options", "get_load_options");

    godot::ClassDB::bind_method(godot::D_METHOD("set_new_game_board", "board"), &Game::set_new_game_board);
    godot::ClassDB::bind_method(godot::D_METHOD("get_new_game_board"), &Game::get_new_game_board);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "new_game_board"), "set_new_game_board", "get_new_game_board");

    godot::ClassDB::bind_method(godot::D_METHOD("set_unload_game_menu_scene_on_start", "unload"), &Game::set_unload_game_menu_scene_on_start);
    godot::ClassDB::bind_method(godot::D_METHOD("get_unload_game_menu_scene_on_start"), &Game::get_unload_game_menu_scene_on_start);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "unload_game_menu_scene_on_start"), "set_unload_game_menu_scene_on_start", "get_unload_game_menu_scene_on_start");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_menu_scene", "scene"), &Game::set_game_menu_scene);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_menu_scene"), &Game::get_game_menu_scene);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_menu_scene", godot::PROPERTY_HINT_NODE_TYPE, "Node"), "set_game_menu_scene", "get_game_menu_scene");

    godot::ClassDB::bind_method(godot::D_METHOD("set_start_game_on_load", "start"), &Game::set_start_game_on_load);
    godot::ClassDB::bind_method(godot::D_METHOD("get_start_game_on_load"), &Game::get_start_game_on_load);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "start_game_on_load"), "set_start_game_on_load", "get_start_game_on_load");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_board_paths", "paths"), &Game::set_game_board_paths);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_board_paths"), &Game::get_game_board_paths);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "game_board_paths", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_game_board_paths", "get_game_board_paths");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_board_titles", "titles"), &Game::set_game_board_titles);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_board_titles"), &Game::get_game_board_titles);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "game_board_titles", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_game_board_titles", "get_game_board_titles");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_board_loader", "loader"), &Game::set_game_board_loader);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_board_loader"), &Game::get_game_board_loader);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_board_loader", godot::PROPERTY_HINT_NODE_TYPE, "SceneTransition"), "set_game_board_loader", "get_game_board_loader");

    godot::ClassDB::bind_method(godot::D_METHOD("set_maximum_interactions", "max_interactions"), &Game::set_maximum_interactions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_maximum_interactions"), &Game::get_maximum_interactions);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "maximum_interactions"), "set_maximum_interactions", "get_maximum_interactions");

    godot::ClassDB::bind_method(godot::D_METHOD("set_gameplay_style", "style"), &Game::set_gameplay_style);
    godot::ClassDB::bind_method(godot::D_METHOD("get_gameplay_style"), &Game::get_gameplay_style);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "gameplay_style", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayStyle"), "set_gameplay_style", "get_gameplay_style");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_hub_ID", "id"), &Game::set_game_hub_ID);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_hub_ID"), &Game::get_game_hub_ID);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "game_hub_ID"), "set_game_hub_ID", "get_game_hub_ID");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("load_dependencies", "dependencies"), &Game::load_dependencies);
    godot::ClassDB::bind_method(godot::D_METHOD("game_loaded_from_game_hub"), &Game::game_loaded_from_game_hub);
    godot::ClassDB::bind_method(godot::D_METHOD("start_game"), &Game::start_game);
    godot::ClassDB::bind_method(godot::D_METHOD("new_game"), &Game::new_game);
    godot::ClassDB::bind_method(godot::D_METHOD("reset_to_menu"), &Game::reset_to_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("load_game", "fileID"), &Game::load_game);
    godot::ClassDB::bind_method(godot::D_METHOD("continue_in_progress"), &Game::continue_in_progress);
    godot::ClassDB::bind_method(godot::D_METHOD("save_game"), &Game::save_game);
    godot::ClassDB::bind_method(godot::D_METHOD("pause_game"), &Game::pause_game);
    godot::ClassDB::bind_method(godot::D_METHOD("quit_game"), &Game::quit_game);
    godot::ClassDB::bind_method(godot::D_METHOD("load_game_board", "game_board_ID"), &Game::load_game_board);
    godot::ClassDB::bind_method(godot::D_METHOD("game_board_load_complete", "game_board_node"), &Game::game_board_load_complete);
    godot::ClassDB::bind_method(godot::D_METHOD("game_board_load_fail"), &Game::game_board_load_fail);
    godot::ClassDB::bind_method(godot::D_METHOD("unload_game_board", "id"), &Game::unload_game_board);
    godot::ClassDB::bind_method(godot::D_METHOD("toggle_game_board_enabled", "board_id"), &Game::toggle_game_board_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("request_game_interaction"), &Game::request_game_interaction);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_gameplay_style", "newStyle"), &Game::apply_gameplay_style);
    godot::ClassDB::bind_method(godot::D_METHOD("save_data"), &Game::save_data);
    godot::ClassDB::bind_method(godot::D_METHOD("load_data", "data"), &Game::load_data);
    godot::ClassDB::bind_method(godot::D_METHOD("validate_continue_play_state"), &Game::validate_continue_play_state);
    godot::ClassDB::bind_method(godot::D_METHOD("create_game_directory"), &Game::create_game_directory);
    godot::ClassDB::bind_method(godot::D_METHOD("path_safe", "unsafe_path"), &Game::path_safe);
}

Game::Game() {}

Game::~Game() {}

void Game::_ready() {
    if (_is_ready) return;
    _is_ready = true;
    
    // if (!title.is_empty()) create_game_directory();
}

void Game::_process(double delta) {
    if (dependencies_loading) {
        godot::ResourceLoader* loader = godot::ResourceLoader::get_singleton();
        for (int i = loading_dependencies.size() - 1; i >= 0; --i) {
            if (loader->load_threaded_get_status(loading_dependencies[i]) == godot::ResourceLoader::THREAD_LOAD_LOADED) {
                loading_dependencies.remove_at(i);
            }
        }

        if (loading_dependencies.is_empty()) {
            dependencies_loading = false;
            emit_signal("dependencies_loaded");
            if (start_game_on_load) {
                start_game();
            }
        }
    }

    float gameDelta = delta * time_scale;

    godot::Array boards = loaded_game_boards.values();
    
    // DOD NOTE: Direct C++ invocation instead of string-based virtual dispatch.
    for (int i = 0; i < boards.size(); ++i) {
        GameBoard* board = godot::Object::cast_to<GameBoard>(boards[i]);
        if (board) {
            board->game_process(gameDelta);
        }
    }

    emit_signal("game_processed", gameDelta);

    for (int i = 0; i < boards.size(); ++i) {
        GameBoard* board = godot::Object::cast_to<GameBoard>(boards[i]);
        if (board) {
            board->game_process_clear();
        }
    }
}

// Assorted Setters/Getters
godot::String Game::get_continue_play_state() const { return continue_play_state; }
void Game::set_game_root(godot::Node* p_root) { game_root = p_root; }
godot::Node* Game::get_game_root() const { return game_root; }
void Game::set_title(const godot::String& p_title) { title = p_title; }
godot::String Game::get_title() const { return title; }
void Game::set_time_scale(float p_scale) { time_scale = p_scale; }
float Game::get_time_scale() const { return time_scale; }
void Game::set_game_state(GameState p_state) { game_state = p_state; }
GameState Game::get_game_state() const { return game_state; }
void Game::set_close_hub_on_quit(bool p_close) { close_hub_on_quit = p_close; }
bool Game::get_close_hub_on_quit() const { return close_hub_on_quit; }
void Game::set_save_path(const godot::String& p_path) { save_path = p_path; }
godot::String Game::get_save_path() const { return save_path; }
void Game::set_save_options(SaveOptions p_options) { save_options = p_options; }
SaveOptions Game::get_save_options() const { return save_options; }
void Game::set_max_save_files(int p_max) { max_save_files = p_max; }
int Game::get_max_save_files() const { return max_save_files; }
void Game::set_load_options(LoadOptions p_options) { load_options = p_options; }
LoadOptions Game::get_load_options() const { return load_options; }
void Game::set_new_game_board(int p_board) { new_game_board = p_board; }
int Game::get_new_game_board() const { return new_game_board; }
void Game::set_unload_game_menu_scene_on_start(bool p_unload) { unload_game_menu_scene_on_start = p_unload; }
bool Game::get_unload_game_menu_scene_on_start() const { return unload_game_menu_scene_on_start; }
void Game::set_game_menu_scene(godot::Node* p_scene) { game_menu_scene = p_scene; }
godot::Node* Game::get_game_menu_scene() const { return game_menu_scene; }
void Game::set_start_game_on_load(bool p_start) { start_game_on_load = p_start; }
bool Game::get_start_game_on_load() const { return start_game_on_load; }
void Game::set_game_board_paths(const godot::TypedArray<godot::String>& p_paths) { game_board_paths = p_paths; }
godot::TypedArray<godot::String> Game::get_game_board_paths() const { return game_board_paths; }
void Game::set_game_board_titles(const godot::TypedArray<godot::String>& p_titles) { game_board_titles = p_titles; }
godot::TypedArray<godot::String> Game::get_game_board_titles() const { return game_board_titles; }
void Game::set_maximum_interactions(int p_max) { maximum_interactions = p_max; }
int Game::get_maximum_interactions() const { return maximum_interactions; }
void Game::set_gameplay_style(const godot::Ref<GameplayStyle>& p_style) { gameplay_style = p_style; }
godot::Ref<GameplayStyle> Game::get_gameplay_style() const { return gameplay_style; }

void Game::set_game_hub_ID(int p_id) { game_hub_ID = p_id; }
int Game::get_game_hub_ID() const { return game_hub_ID; }

void Game::set_game_board_loader(SceneTransition* p_loader) {
    if (p_loader == _game_board_loader) return;
    
    if (!_is_ready) {
        _game_board_loader = p_loader;
        return;
    }
    
    if (_game_board_loader) {
        _disconnect_from_game_board_loader();
    }
    
    _game_board_loader = p_loader;
    
    if (_game_board_loader) {
        _connect_to_game_board_loader();
    }
}

SceneTransition* Game::get_game_board_loader() const { return _game_board_loader; }

void Game::load_dependencies(const godot::TypedArray<godot::String>& dependencies) {
    loading_dependencies = dependencies;
    godot::ResourceLoader* loader = godot::ResourceLoader::get_singleton();
    for (int i = 0; i < dependencies.size(); ++i) {
        godot::String dep = dependencies[i];
        if (loader->load_threaded_get_status(dep) == godot::ResourceLoader::THREAD_LOAD_INVALID_RESOURCE) {
            loader->load_threaded_request(dep);
        }
    }
    dependencies_loading = true;
}

void Game::game_loaded_from_game_hub() {
    emit_signal("loaded_from_game_hub");
    game_state = GameState::PREGAME;

    if (dependencies_loading) return;

    if (start_game_on_load) {
        if (new_game_board >= 0) {
            new_game();
        } else {
            start_game();
        }
    }
}

void Game::start_game() {
    emit_signal("game_started");
    game_state = GameState::PLAYING;

    if (unload_game_menu_scene_on_start && game_menu_scene) {
        game_menu_scene->queue_free();
    }

    godot::Array boards = loaded_game_boards.values();
    for (int i = 0; i < boards.size(); ++i) {
        GameBoard* board = godot::Object::cast_to<GameBoard>(boards[i]);
        if (board) {
            board->game_start();
        }
    }
}

void Game::new_game() {
    if (new_game_board >= 0) {
        game_state = GameState::PREGAME;
        load_game_board(new_game_board);
        emit_signal("new_game_started");
    } else {
        start_game();
    }
}

void Game::reset_to_menu() {}
void Game::load_game(int fileID) {}
void Game::continue_in_progress() {}

void Game::save_game() {
    if (save_options == SaveOptions::NO_SAVE) return;

    godot::String save_name;
    godot::Dictionary data = save_data();
    godot::TypedArray<godot::String> save_file;
    godot::Ref<godot::JSON> json;
    json.instantiate();

    switch (save_options) {
        case SaveOptions::PLAYSTATE_SAVE:
            save_file.append(json->stringify(data));
            save_name = "user://" + title + "_save.game";
            break;
        case SaveOptions::SAVE_FILES:
            save_file.append(json->stringify(data));
            save_name = "user://";
            break;
        case SaveOptions::FULL_SAVE:
            return;
        default: break;
    }

    save_game_file(save_name, save_file);
}

void Game::save_game_file(const godot::String& save_name, const godot::TypedArray<godot::String>& save_file) {
    godot::Ref<godot::FileAccess> file_access = godot::FileAccess::open(save_name, godot::FileAccess::WRITE);
    if (file_access.is_valid()) {
        for (int i = 0; i < save_file.size(); ++i) {
            file_access->store_line(save_file[i]);
        }
        file_access->close();
    }
}

void Game::pause_game() {
    godot::Array boards = loaded_game_boards.values();
    if (!game_is_paused) {
        game_is_paused = true;
        emit_signal("game_paused");
        
        for (int i = 0; i < boards.size(); ++i) {
            GameBoard* board = godot::Object::cast_to<GameBoard>(boards[i]);
            if (board) board->game_pause();
        }
    } else {
        for (int i = 0; i < boards.size(); ++i) {
            GameBoard* board = godot::Object::cast_to<GameBoard>(boards[i]);
            if (board) board->game_continue();
        }
        game_is_paused = false;
        emit_signal("game_continued");
    }
}

void Game::quit_game() {
    emit_signal("game_ended", game_hub_ID);
    if (game_hub_ID < 0) {
        if (get_tree()) get_tree()->notification(NOTIFICATION_WM_CLOSE_REQUEST);
    }
}

void Game::load_game_board(int game_board_ID) {
    if (loaded_game_boards.has(game_board_ID)) {
        GameBoard* loaded_game_board = godot::Object::cast_to<GameBoard>(loaded_game_boards[game_board_ID]);
        if (loaded_game_board) {
            emit_signal("game_board_already_loaded", this, loaded_game_board);
            return;
        }
    }

    if (game_board_ID < game_board_paths.size()) {
        godot::String game_board_path = game_board_paths[game_board_ID];
        SceneTransition* gbl = _game_board_loader;
        
        if (!gbl) {
            if (!_default_board_loader) {
                _create_default_game_board_loader();
            }
            gbl = _default_board_loader;
        }
        
        gbl->start_transition(game_board_path);
        loading_game_boards.append(game_board_path);
        emit_signal("loading_game_board_started", this, game_board_ID);
    } else {
        emit_signal("game_board_not_found", this, game_board_ID);
    }
}

void Game::game_board_load_complete(godot::Node* game_board_node) {
    GameBoard* loaded_board = godot::Object::cast_to<GameBoard>(game_board_node);
    if (!loaded_board) return;

    SceneTransition* gbl = _game_board_loader;
    if (!gbl) {
        if (!_default_board_loader) {
            _create_default_game_board_loader();
        }
        gbl = _default_board_loader;
    }

    godot::String game_board_path = gbl->get_to_path();
    int remove_idx = loading_game_boards.find(game_board_path);
    if (remove_idx >= 0) {
        loading_game_boards.remove_at(remove_idx);
    }

    int game_board_ID = game_board_paths.find(game_board_path);
    loaded_game_boards[game_board_ID] = loaded_board;

    add_child(loaded_board);
    if (get_tree()) {
        loaded_board->set_owner(get_tree()->get_edited_scene_root());
    }

    // Process frame await equivalent
    call_deferred("emit_signal", "game_board_loaded", this, loaded_board);

    if (game_board_ID == new_game_board && game_state == GameState::PREGAME) {
        start_game();
    } else if (game_state == GameState::PLAYING || game_state == GameState::PAUSED) {
        loaded_board->enter_game();
    }

    gbl->complete_transition();
}

void Game::game_board_load_fail() {}

void Game::unload_game_board(int id) {
    if (loaded_game_boards.has(id)) {
        GameBoard* board = godot::Object::cast_to<GameBoard>(loaded_game_boards[id]);
        loaded_game_boards[id] = godot::Variant();
        loaded_game_boards.erase(id);
        
        if (board) {
            board->exit_game();
            board->queue_free();
        }
    }
}

void Game::toggle_game_board_enabled(int board_id) {
    if (board_id >= 0 && board_id < game_board_paths.size()) {
        godot::String board_path = game_board_paths[board_id];
        if (loaded_game_boards.has(board_path)) {
            GameBoard* board = godot::Object::cast_to<GameBoard>(loaded_game_boards[board_path]);
            if (board) {
                if (board->get_enabled()) {
                    board->disable_game_entity();
                } else {
                    board->enable_game_entity();
                }
            }
        }
    }
}

GameInteraction* Game::request_game_interaction() {
    GameInteraction* available_interaction = nullptr;
    
    for (int i = 0; i < interactions_pool.size(); ++i) {
        GameInteraction* interaction = godot::Object::cast_to<GameInteraction>(interactions_pool[i]);
        if (interaction && !interaction->is_interaction_active()) {
            available_interaction = interaction;
            break;
        }
    }
    
    if (!available_interaction) {
        if (maximum_interactions == 0 || interactions_pool.size() < maximum_interactions) {
            available_interaction = memnew(GameInteraction);
            available_interaction->set_name("Interaction " + godot::String::num_int64(interactions_pool.size()));
            interactions_pool.append(available_interaction);
            add_child(available_interaction);
        }
    }
        
    return available_interaction;
}

void Game::apply_gameplay_style(const godot::Ref<GameplayStyle>& newStyle) {
    if (newStyle.is_valid()) {
        resolved_gameplay_style = newStyle->duplicate(true);
        resolved_gameplay_style->apply_style(gameplay_style);
        
        godot::Array boards = loaded_game_boards.values();
        for (int i = 0; i < boards.size(); ++i) {
            GameBoard* board = godot::Object::cast_to<GameBoard>(boards[i]);
            if (board) board->apply_gameplay_style(resolved_gameplay_style);
        }
    }
}

godot::Dictionary Game::save_data() const {
    godot::Dictionary data;
    data["title"] = title;
    data["time_scale"] = time_scale;
    data["save_options"] = save_options;
    data["load_options"] = load_options;
    
    if (!continue_play_state.is_empty()) {
        data["continue_play_state"] = continue_play_state;
    }
    
    data["new_game_board"] = new_game_board;
    data["unload_game_menu_scene_on_start"] = unload_game_menu_scene_on_start;
    
    if (game_menu_scene) {
        data["game_menu_scene"] = game_menu_scene->get_path();
    }
    
    data["start_game_on_load"] = start_game_on_load;
    data["game_board_paths"] = game_board_paths;
    data["game_board_titles"] = game_board_titles;
    
    if (gameplay_style.is_valid()) {
        if (gameplay_style->is_local_to_scene()) {
            data["style"] = gameplay_style->save_style();
        } else {
            data["style_path"] = gameplay_style->get_path();
        }
    }
        
    return data;
}

void Game::load_data(const godot::Dictionary& data) {
    if (data.has("title")) title = data["title"];
    if (data.has("time_scale")) time_scale = data["time_scale"];
    if (data.has("save_options")) save_options = static_cast<SaveOptions>((int)data["save_options"]);
    if (data.has("continue_play_state")) continue_play_state = data["continue_play_state"];
    if (data.has("load_options")) load_options = static_cast<LoadOptions>((int)data["load_options"]);
    if (data.has("new_game_board")) new_game_board = data["new_game_board"];
    if (data.has("unload_game_menu_scene_on_start")) unload_game_menu_scene_on_start = data["unload_game_menu_scene_on_start"];
    
    if (data.has("game_menu_scene")) {
        // game_menu_scene re-attachment requires path resolution logic normally handled higher up
    }
    
    if (data.has("start_game_on_load")) start_game_on_load = data["start_game_on_load"];
    if (data.has("game_board_paths")) game_board_paths = data["game_board_paths"];
    if (data.has("game_board_titles")) game_board_titles = data["game_board_titles"];
    
    if (data.has("style_path")) {
        gameplay_style = godot::ResourceLoader::get_singleton()->load(data["style_path"]);
    } else if (data.has("style")) {
        gameplay_style.instantiate();
        gameplay_style->set_local_to_scene(true);
        gameplay_style->load_style(data["style"]);
    }
}

void Game::validate_continue_play_state() {
    if (continue_play_state.is_empty()) return;
    
    godot::String directory = path_safe(title);
    godot::Ref<godot::DirAccess> dirAccess = godot::DirAccess::open("user://");
    
    if (dirAccess.is_valid() && dirAccess->dir_exists(directory)) {
        directory += "/saves";
        if (dirAccess->dir_exists(directory) && !dirAccess->file_exists(continue_play_state)) {
            continue_play_state = "";
        }
    }
}

void Game::create_game_directory() {
    godot::String directory = path_safe(title);
    godot::Ref<godot::DirAccess> dirAccess = godot::DirAccess::open("user://");
    
    if (dirAccess.is_valid()) {
        if (!dirAccess->dir_exists(directory)) {
            dirAccess->make_dir(directory);
        }
        
        directory += "/saves";
        if (!dirAccess->dir_exists(directory)) {
            dirAccess->make_dir(directory);
        }
        
        dirAccess->change_dir(directory);
        save_file_paths.clear();
        
        godot::PackedStringArray files = dirAccess->get_files();
        for (int i = 0; i < files.size(); ++i) {
            godot::String file = files[i];
            if (file.get_extension() == "game") {
                save_file_paths.append(file.get_file().get_basename());
            }
        }
    }
}

godot::String Game::path_safe(const godot::String& unsafe_path) const {
    return unsafe_path.strip_escapes();
}

void Game::_create_default_game_board_loader() {
    _default_board_loader = memnew(SceneTransition);
    add_child(_default_board_loader);

    if (!_default_board_loader->is_connected("load_to_scene_completed", godot::Callable(this, "game_board_load_complete"))) {
        _default_board_loader->connect("load_to_scene_completed", godot::Callable(this, "game_board_load_complete"));
    }
    
    if (!_default_board_loader->is_connected("load_to_scene_failed", godot::Callable(this, "game_board_load_fail"))) {
        _default_board_loader->connect("load_to_scene_failed", godot::Callable(this, "game_board_load_fail"));
    }
}

void Game::_connect_to_game_board_loader() {
    if (_game_board_loader && !_game_board_loader->is_connected("load_to_scene_completed", godot::Callable(this, "game_board_load_complete"))) {
        _game_board_loader->connect("load_to_scene_completed", godot::Callable(this, "game_board_load_complete"));
    }
    
    if (_game_board_loader && !_game_board_loader->is_connected("load_to_scene_failed", godot::Callable(this, "game_board_load_fail"))) {
        _game_board_loader->connect("load_to_scene_failed", godot::Callable(this, "game_board_load_fail"));
    }
}

void Game::_disconnect_from_game_board_loader() {
    if (_game_board_loader && _game_board_loader->is_connected("load_to_scene_completed", godot::Callable(this, "game_board_load_complete"))) {
        _game_board_loader->disconnect("load_to_scene_completed", godot::Callable(this, "game_board_load_complete"));
    }
    
    if (_game_board_loader && _game_board_loader->is_connected("load_to_scene_failed", godot::Callable(this, "game_board_load_fail"))) {
        _game_board_loader->disconnect("load_to_scene_failed", godot::Callable(this, "game_board_load_fail"));
    }
}

} // namespace ideam::godot_ext