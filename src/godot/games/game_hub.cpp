#include "game_hub.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "scene_transition.h"
#include "game_player.h"
#include "game.h"

namespace ideam::godot_ext {

void GameHub::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("game_not_found", godot::PropertyInfo(godot::Variant::INT, "game_id")));
    ADD_SIGNAL(godot::MethodInfo("load_game_started", godot::PropertyInfo(godot::Variant::INT, "game_id")));
    ADD_SIGNAL(godot::MethodInfo("load_game_canceled", godot::PropertyInfo(godot::Variant::INT, "game_id")));
    ADD_SIGNAL(godot::MethodInfo("load_game_completed", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_RESOURCE_TYPE, "Game")));
    ADD_SIGNAL(godot::MethodInfo("game_already_loaded", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_RESOURCE_TYPE, "Game")));
    ADD_SIGNAL(godot::MethodInfo("game_unloaded", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_RESOURCE_TYPE, "Game")));

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_game_paths", "paths"), &GameHub::set_game_paths);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_paths"), &GameHub::get_game_paths);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "game_paths", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_game_paths", "get_game_paths");
    ADD_SIGNAL(godot::MethodInfo("game_paths_changed", godot::PropertyInfo(godot::Variant::ARRAY, "game_paths", godot::PROPERTY_HINT_ARRAY_TYPE, "String")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_titles", "titles"), &GameHub::set_game_titles);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_titles"), &GameHub::get_game_titles);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "game_titles", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_game_titles", "get_game_titles");
    ADD_SIGNAL(godot::MethodInfo("game_titles_changed", godot::PropertyInfo(godot::Variant::ARRAY, "game_titles", godot::PROPERTY_HINT_ARRAY_TYPE, "String")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_startup_game", "startup_game"), &GameHub::set_startup_game);
    godot::ClassDB::bind_method(godot::D_METHOD("get_startup_game"), &GameHub::get_startup_game);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "startup_game"), "set_startup_game", "get_startup_game");
    ADD_SIGNAL(godot::MethodInfo("startup_game_changed", godot::PropertyInfo(godot::Variant::INT, "startup_game")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_loader", "loader"), &GameHub::set_game_loader);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_loader"), &GameHub::get_game_loader);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_loader", godot::PROPERTY_HINT_NODE_TYPE, "SceneTransition"), "set_game_loader", "get_game_loader");
    ADD_SIGNAL(godot::MethodInfo("game_loader_changed", godot::PropertyInfo(godot::Variant::OBJECT, "game_loader", godot::PROPERTY_HINT_NODE_TYPE, "SceneTransition")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_hub_scene", "scene"), &GameHub::set_hub_scene);
    godot::ClassDB::bind_method(godot::D_METHOD("get_hub_scene"), &GameHub::get_hub_scene);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "hub_scene", godot::PROPERTY_HINT_NODE_TYPE, "Node"), "set_hub_scene", "get_hub_scene");
    ADD_SIGNAL(godot::MethodInfo("hub_scene_changed", godot::PropertyInfo(godot::Variant::OBJECT, "hub_scene", godot::PROPERTY_HINT_NODE_TYPE, "Node")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_disable_hub_scene_on_game_load", "disable"), &GameHub::set_disable_hub_scene_on_game_load);
    godot::ClassDB::bind_method(godot::D_METHOD("get_disable_hub_scene_on_game_load"), &GameHub::get_disable_hub_scene_on_game_load);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "disable_hub_scene_on_game_load"), "set_disable_hub_scene_on_game_load", "get_disable_hub_scene_on_game_load");

    godot::ClassDB::bind_method(godot::D_METHOD("get_loaded_games"), &GameHub::get_loaded_games);

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("connect_to_player", "player"), &GameHub::connect_to_player);
    godot::ClassDB::bind_method(godot::D_METHOD("load_startup_game"), &GameHub::load_startup_game);
    godot::ClassDB::bind_method(godot::D_METHOD("load_game", "game_id"), &GameHub::load_game);
    godot::ClassDB::bind_method(godot::D_METHOD("game_load_complete", "game_node"), &GameHub::game_load_complete);
    godot::ClassDB::bind_method(godot::D_METHOD("game_load_fail"), &GameHub::game_load_fail);
    godot::ClassDB::bind_method(godot::D_METHOD("unload_game", "game_id"), &GameHub::unload_game);
    godot::ClassDB::bind_method(godot::D_METHOD("disable_hub_scene"), &GameHub::disable_hub_scene);
    godot::ClassDB::bind_method(godot::D_METHOD("enable_hub_scene"), &GameHub::enable_hub_scene);
    godot::ClassDB::bind_method(godot::D_METHOD("add_game", "gamePath"), &GameHub::add_game);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_game", "game_id"), &GameHub::remove_game);
    
    // Internal callback binding
    godot::ClassDB::bind_method(godot::D_METHOD("_on_process_frame_startup"), &GameHub::_on_process_frame_startup);
}

GameHub::GameHub() {
    // Note: Defer heavy initializations to _ready to avoid heap fragmentation 
    // when instances are spawned rapidly from packed scenes.
}

GameHub::~GameHub() {}

void GameHub::_ready() {
    if (_is_ready || godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    _is_ready = true;

    if (hub_scene) {
        hub_scene_parent = hub_scene->get_parent();
    }

    // Translating GDScript's `await get_tree().process_frame`
    // We bind to process_frame ONE_SHOT to execute the rest of the startup routine.
    if (get_tree()) {
        get_tree()->connect("process_frame", godot::Callable(this, "_on_process_frame_startup"), godot::Object::CONNECT_ONE_SHOT);
    }
}

void GameHub::_on_process_frame_startup() {
    if (_game_loader) {
        _connect_to_game_loader();
    }
    load_startup_game();
}

void GameHub::_notification(int p_what) {
    if (p_what == NOTIFICATION_WM_CLOSE_REQUEST) {
        if (get_tree()) {
            get_tree()->quit(); // Default behavior
        }
    }
}

// Property Implementations
void GameHub::set_game_paths(const godot::TypedArray<godot::String>& p_paths) { 
    if (game_paths == p_paths) return;
    game_paths = p_paths; 
    emit_signal("game_paths_changed", game_paths);
}
godot::TypedArray<godot::String> GameHub::get_game_paths() const { return game_paths; }

void GameHub::set_game_titles(const godot::TypedArray<godot::String>& p_titles) { 
    if (game_titles == p_titles) return;
    game_titles = p_titles; 
    emit_signal("game_titles_changed", game_titles);
}
godot::TypedArray<godot::String> GameHub::get_game_titles() const { return game_titles; }

void GameHub::set_startup_game(int p_startup) { 
    if (startup_game == p_startup) return;
    startup_game = p_startup; 
    emit_signal("startup_game_changed", startup_game);
}
int GameHub::get_startup_game() const { return startup_game; }

void GameHub::set_game_loader(SceneTransition* p_loader) {
    if (p_loader == _game_loader) return;

    if (!_is_ready) {
        _game_loader = p_loader;
        return;
    }

    if (_game_loader) {
        _disconnect_from_game_loader();
    }

    _game_loader = p_loader;

    if (_game_loader) {
        _connect_to_game_loader();
    }
    emit_signal("game_loader_changed", _game_loader);
}
SceneTransition* GameHub::get_game_loader() const { return _game_loader; }

void GameHub::set_hub_scene(godot::Node* p_scene) { 
    if (hub_scene == p_scene) return;
    hub_scene = p_scene; 
    emit_signal("hub_scene_changed", hub_scene);
}
godot::Node* GameHub::get_hub_scene() const { return hub_scene; }

void GameHub::set_disable_hub_scene_on_game_load(bool p_disable) { 
    if (disable_hub_scene_on_game_load == p_disable) return;
    disable_hub_scene_on_game_load = p_disable; 
}
bool GameHub::get_disable_hub_scene_on_game_load() const { return disable_hub_scene_on_game_load; }

godot::Dictionary GameHub::get_loaded_games() const { 
    return loaded_games; 
}

// Class Methods
void GameHub::connect_to_player(GamePlayer* player) {
    if (!player) return;
    
    SceneTransition* transition = player->get_game_transition();
    if (transition) {
        if (_game_loader) {
            _disconnect_from_game_loader();
        }
        
        _game_loader = transition;
        _connect_to_game_loader();
    }
}

void GameHub::load_startup_game() {
    if (startup_game < 0 || startup_game >= game_paths.size()) {
        return;
    }
    load_game(startup_game);
}

void GameHub::load_game(int game_id) {
    if (loaded_games.has(game_id)) {
        godot::Object* loadedGameObj = loaded_games[game_id];
        if (loadedGameObj) {
            emit_signal("game_already_loaded", loadedGameObj);
            return;
        }
    }

    if (game_id >= game_paths.size()) {
        emit_signal("game_not_found", game_id);
        return;
    }

    if (!_game_loader) {
        if (!_default_game_loader) {
            _create_default_game_loader();
        }
        set_game_loader(_default_game_loader);
    }

    godot::String gamePath = game_paths[game_id];
    
    _game_loader->start_transition(gamePath);
    
    loading_games.append(gamePath);
    emit_signal("load_game_started", game_id);
}

void GameHub::game_load_complete(godot::Node* game_node) {
    Game* loadedGame = godot::Object::cast_to<Game>(game_node);
    if (!loadedGame) return;
    
    if (!_game_loader) {
        if (!_default_game_loader) {
            _create_default_game_loader();
        }
        set_game_loader(_default_game_loader);
    }

    godot::String gamePath = _game_loader->get_to_path();
    
    int path_index = loading_games.find(gamePath);
    if (path_index != -1) {
        loading_games.remove_at(path_index);
    }

    int game_id = game_paths.find(gamePath);
    loaded_games[game_id] = loadedGame;
    
    add_child(loadedGame);
    
    if (get_tree() && get_tree()->get_edited_scene_root()) {
        loadedGame->set_owner(get_tree()->get_edited_scene_root());
    }
    
    loadedGame->set_game_hub_ID(game_id);
    
    if (!loadedGame->is_connected("game_ended", godot::Callable(this, "unload_game"))) {
        loadedGame->connect("game_ended", godot::Callable(this, "unload_game"));
    }
    
    if (disable_hub_scene_on_game_load && hub_scene) {
        disable_hub_scene();
    }
    
    // Translating GDScript commented block:
    // godot::PackedStringArray dependencies = godot::ResourceLoader::get_singleton()->get_dependencies(gamePath);
    // if (dependencies.size() > 0) {
    //      loadedGame->load_dependencies(dependencies);
    // }
        
    loadedGame->game_loaded_from_game_hub();
    emit_signal("load_game_completed", loadedGame);
    
    _game_loader->complete_transition();
}

void GameHub::game_load_fail() {
    // Pass equivalent in GDScript
}

void GameHub::unload_game(int game_id) {
    if (!loaded_games.has(game_id)) return;
    
    Game* loadedGame = godot::Object::cast_to<Game>(loaded_games[game_id]);
    if (!loadedGame) return;
    
    if (loadedGame->get_close_hub_on_quit()) {
        if (get_tree() && get_tree()->get_root()) {
            get_tree()->get_root()->propagate_notification(NOTIFICATION_WM_CLOSE_REQUEST);
        }
    } else {
        emit_signal("game_unloaded", loadedGame);
        
        godot::Node* target_scene = nullptr;
        if (get_tree()) {
            target_scene = get_tree()->get_current_scene();
        }
        
        if (target_scene) {
            target_scene->remove_child(loadedGame);
        }
        
        loaded_games.erase(game_id);
        loadedGame->queue_free();
    }
}

void GameHub::disable_hub_scene() {
    if (hub_scene_disabled || !hub_scene_parent || !hub_scene) return;

    hub_scene_parent->remove_child(hub_scene);
    hub_scene_disabled = true;
}

void GameHub::enable_hub_scene() {
    if (!hub_scene_disabled || !hub_scene_parent || !hub_scene) return;

    hub_scene_parent->add_child(hub_scene);
    hub_scene_disabled = false;
}

int GameHub::add_game(const godot::String& gamePath) {
    if (godot::ResourceLoader::get_singleton()->exists(gamePath)) {
        if (!game_paths.has(gamePath)) {
            game_paths.append(gamePath);
            return game_paths.size() - 1;
        }
    }
    return -1;
}

void GameHub::remove_game(int game_id) {
    if (game_id >= 0 && game_id < game_paths.size()) {
        game_paths.remove_at(game_id);
    }
}

void GameHub::_create_default_game_loader() {
    _default_game_loader = memnew(SceneTransition);
    add_child(_default_game_loader);
}

void GameHub::_connect_to_game_loader() {
    if (!_game_loader) return;
    
    if (!_game_loader->is_connected("load_to_scene_completed", godot::Callable(this, "game_load_complete"))) {
        _game_loader->connect("load_to_scene_completed", godot::Callable(this, "game_load_complete"));
    }
    if (!_game_loader->is_connected("load_to_scene_failed", godot::Callable(this, "game_load_fail"))) {
        _game_loader->connect("load_to_scene_failed", godot::Callable(this, "game_load_fail"));
    }
}

void GameHub::_disconnect_from_game_loader() {
    if (!_game_loader) return;
    
    if (_game_loader->is_connected("load_to_scene_completed", godot::Callable(this, "game_load_complete"))) {
        _game_loader->disconnect("load_to_scene_completed", godot::Callable(this, "game_load_complete"));
    }
    if (_game_loader->is_connected("load_to_scene_failed", godot::Callable(this, "game_load_fail"))) {
        _game_loader->disconnect("load_to_scene_failed", godot::Callable(this, "game_load_fail"));
    }
}

} // namespace ideam::godot_ext