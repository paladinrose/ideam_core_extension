#include "game_entity.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

// Explicit includes for static resolution
#include "game.h"
#include "gameplay/gameplay_style.h"

namespace ideam::godot_ext {

void GameEntity::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("game_entered", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_NODE_TYPE, "Game")));
    ADD_SIGNAL(godot::MethodInfo("game_started"));
    ADD_SIGNAL(godot::MethodInfo("game_ended"));
    ADD_SIGNAL(godot::MethodInfo("game_exited"));
    ADD_SIGNAL(godot::MethodInfo("game_paused"));
    ADD_SIGNAL(godot::MethodInfo("game_continued"));
    ADD_SIGNAL(godot::MethodInfo("entity_enabled"));
    ADD_SIGNAL(godot::MethodInfo("entity_disabled"));

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_enabled", "enabled"), &GameEntity::set_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enabled"), &GameEntity::get_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "enabled"), "set_enabled", "get_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_title", "title"), &GameEntity::set_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_title"), &GameEntity::get_title);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "title"), "set_title", "get_title");

    godot::ClassDB::bind_method(godot::D_METHOD("set_root_node", "root_node"), &GameEntity::set_root_node);
    godot::ClassDB::bind_method(godot::D_METHOD("get_root_node"), &GameEntity::get_root_node);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "root_node", godot::PROPERTY_HINT_NODE_TYPE, "Node"), "set_root_node", "get_root_node");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game", "game"), &GameEntity::set_game);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game"), &GameEntity::get_game);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_NODE_TYPE, "Game"), "set_game", "get_game");

    godot::ClassDB::bind_method(godot::D_METHOD("set_time_scale", "time_scale"), &GameEntity::set_time_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_time_scale"), &GameEntity::get_time_scale);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "time_scale"), "set_time_scale", "get_time_scale");

    godot::ClassDB::bind_method(godot::D_METHOD("set_gameplay_style", "style"), &GameEntity::set_gameplay_style);
    godot::ClassDB::bind_method(godot::D_METHOD("get_gameplay_style"), &GameEntity::get_gameplay_style);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "gameplay_style", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayStyle"), "set_gameplay_style", "get_gameplay_style");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("validate_game"), &GameEntity::validate_game);
    godot::ClassDB::bind_method(godot::D_METHOD("game_start"), &GameEntity::game_start);
    godot::ClassDB::bind_method(godot::D_METHOD("enter_game"), &GameEntity::enter_game);
    godot::ClassDB::bind_method(godot::D_METHOD("game_end"), &GameEntity::game_end);
    godot::ClassDB::bind_method(godot::D_METHOD("exit_game"), &GameEntity::exit_game);
    godot::ClassDB::bind_method(godot::D_METHOD("game_pause"), &GameEntity::game_pause);
    godot::ClassDB::bind_method(godot::D_METHOD("game_continue"), &GameEntity::game_continue);
    godot::ClassDB::bind_method(godot::D_METHOD("game_process", "delta"), &GameEntity::game_process);
    godot::ClassDB::bind_method(godot::D_METHOD("game_process_clear"), &GameEntity::game_process_clear);
    
    godot::ClassDB::bind_method(godot::D_METHOD("toggle_game_entity_enabled"), &GameEntity::toggle_game_entity_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("enable_game_entity"), &GameEntity::enable_game_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("disable_game_entity"), &GameEntity::disable_game_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("action_consequences", "score", "consequences"), &GameEntity::action_consequences);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_gameplay_style", "newStyle"), &GameEntity::apply_gameplay_style);
    
    godot::ClassDB::bind_method(godot::D_METHOD("save_data"), &GameEntity::save_data);
    godot::ClassDB::bind_method(godot::D_METHOD("load_data", "data"), &GameEntity::load_data);
}

GameEntity::GameEntity() {}

GameEntity::~GameEntity() {}

void GameEntity::_ready() {
    if (godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    entity_is_initialized = true;
    
    if (enabled) {
        enable_game_entity();
    }
}

// Setters / Getters
void GameEntity::set_enabled(bool p_enabled) {
    if (enabled == p_enabled) return;
    enabled = p_enabled;
    
    if (godot::Engine::get_singleton()->is_editor_hint()) return;
    
    if (enabled && entity_is_initialized) {
        enable_game_entity();
    } else {
        disable_game_entity();
    }
}

bool GameEntity::get_enabled() const { return enabled; }

void GameEntity::set_title(const godot::String& p_title) { title = p_title; }
godot::String GameEntity::get_title() const { return title; }

void GameEntity::set_root_node(godot::Node* p_root) { root_node = p_root; }
godot::Node* GameEntity::get_root_node() const { return root_node; }

void GameEntity::set_game(Game* p_game) {
    if (p_game == _game) return;
    
    if (!entity_is_initialized) {
        _game = p_game;
        return;
    }
    
    if (_game) {
        exit_game();
    }
    
    _game = p_game;
    if (_game) {
        enter_game();
    }
}
Game* GameEntity::get_game() const { return _game; }

void GameEntity::set_time_scale(float p_scale) { time_scale = p_scale; }
float GameEntity::get_time_scale() const { return time_scale; }

void GameEntity::set_gameplay_style(const godot::Ref<GameplayStyle>& p_style) { gameplay_style = p_style; }
godot::Ref<GameplayStyle> GameEntity::get_gameplay_style() const { return gameplay_style; }

bool GameEntity::get_entity_is_paused() const { return entity_is_paused; }
void GameEntity::set_entity_is_paused(bool p_paused) { entity_is_paused = p_paused; }

void GameEntity::validate_game() {
    if (!_game) {
        godot::Node* p = get_parent();
        while (p) {
            Game* g = godot::Object::cast_to<Game>(p);
            if (g) {
                _game = g;
                break;
            }
            p = p->get_parent();
        }
    }
}

void GameEntity::game_start() {
    emit_signal("game_started");
}

void GameEntity::enter_game() {
    emit_signal("game_entered", _game);
}

void GameEntity::game_end() {
    emit_signal("game_ended");
}

void GameEntity::exit_game() {
    emit_signal("game_exited");
}

void GameEntity::game_pause() {
    if (entity_is_paused) return;
    entity_is_paused = true;
    emit_signal("game_paused");
}

void GameEntity::game_continue() {
    if (!entity_is_paused) return;
    entity_is_paused = false;
    emit_signal("game_continued");
}

void GameEntity::game_process(double delta) {
    // Defined intentionally as a base stub mirroring GDScript
}

void GameEntity::game_process_clear() {
    game_processed = false;
}

void GameEntity::toggle_game_entity_enabled() {
    set_enabled(!enabled);
}

void GameEntity::enable_game_entity() {
    emit_signal("entity_enabled");
}

void GameEntity::disable_game_entity() {
    emit_signal("entity_disabled");
}

void GameEntity::action_consequences(int score, const godot::Dictionary& consequences) {
    if (consequences.has("set_time_scale")) {
        godot::Variant ts_val = consequences["set_time_scale"];
        if (ts_val.get_type() == godot::Variant::STRING) {
            godot::String str_val = ts_val;
            if (str_val == "score") time_scale = static_cast<float>(score);
            else if (str_val == "-score") time_scale = static_cast<float>(-score);
            else time_scale = str_val.to_float();
        } else {
            time_scale = static_cast<float>(ts_val);
        }
    } else if (consequences.has("change_time_scale")) {
        godot::Variant ts_val = consequences["change_time_scale"];
        if (ts_val.get_type() == godot::Variant::STRING) {
            godot::String str_val = ts_val;
            if (str_val == "score") time_scale += static_cast<float>(score);
            else if (str_val == "-score") time_scale -= static_cast<float>(score);
            else time_scale += str_val.to_float();
        } else {
            time_scale += static_cast<float>(ts_val);
        }
    }
    
    if (consequences.has("set_title")) {
        title = consequences["set_title"];
    }
    
    if (consequences.has("set_root_node")) {
        godot::Variant node_val = consequences["set_root_node"];
        if (node_val.get_type() == godot::Variant::OBJECT) {
            root_node = godot::Object::cast_to<godot::Node>(node_val);
        } else if (node_val.get_type() == godot::Variant::NODE_PATH) {
            root_node = get_node_or_null(node_val);
        }
    }
    
    if (consequences.has("exit_game")) {
        exit_game();
    }
    
    if (consequences.has("move_to_game")) {
        Game* new_game = godot::Object::cast_to<Game>(consequences["move_to_game"]);
        if (_game != new_game) {
            if (_game) exit_game();
            _game = new_game;
            if (_game) enter_game();
        }
    }
    
    if (consequences.has("set_gameplay_style")) {
        apply_gameplay_style(consequences["set_gameplay_style"]);
    }
}

void GameEntity::apply_gameplay_style(const godot::Ref<GameplayStyle>& newStyle) {
    if (newStyle.is_valid()) {
        _resolved_gameplay_style = godot::Ref<GameplayStyle>(godot::Object::cast_to<GameplayStyle>(newStyle->duplicate(true).ptr()));
        if (gameplay_style.is_valid()) {
            _resolved_gameplay_style->apply_style(gameplay_style); 
        }
    }
}

godot::Dictionary GameEntity::save_data() const {
    godot::Dictionary data;
    data["title"] = title;
    
    if (root_node) {
        data["root_node"] = get_path_to(root_node);
    }
    
    data["time_scale"] = time_scale;
    
    if (gameplay_style.is_valid()) {
        bool local_to_scene = gameplay_style->is_local_to_scene(); 
        if (local_to_scene) {
            data["style"] = gameplay_style->save_style(); 
        } else {
            data["style_path"] = gameplay_style->get_path();
        }
    }
    
    return data;
}

void GameEntity::load_data(const godot::Dictionary& data) {
    if (data.has("title")) {
        title = data["title"];
    }
    
    if (data.has("root_node")) {
        godot::NodePath path = data["root_node"];
        if (has_node(path)) {
            root_node = get_node_or_null(path);
        }
    }
    
    if (data.has("time_scale")) {
        time_scale = data["time_scale"];
    }
    
    if (data.has("style_path")) {
        gameplay_style = godot::ResourceLoader::get_singleton()->load(data["style_path"]);
    } else if (data.has("style")) {
        gameplay_style.instantiate();
        gameplay_style->setup_local_to_scene();
        gameplay_style->load_style(data["style"]);
    }
}

} // namespace ideam::godot_ext