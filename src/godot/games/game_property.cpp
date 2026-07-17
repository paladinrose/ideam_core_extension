#include "game_property.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "game_property_modifier.h"

namespace ideam::godot_ext {

void GameProperty::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("used_as_resource", godot::PropertyInfo(godot::Variant::INT, "useValue"), godot::PropertyInfo(godot::Variant::INT, "actionValue")));
    ADD_SIGNAL(godot::MethodInfo("value_changed", godot::PropertyInfo(godot::Variant::INT, "oldValue"), godot::PropertyInfo(godot::Variant::INT, "newValue")));
    ADD_SIGNAL(godot::MethodInfo("value_restored"));
    ADD_SIGNAL(godot::MethodInfo("value_exhausted"));
    ADD_SIGNAL(godot::MethodInfo("value_no_longer_exhausted"));
    ADD_SIGNAL(godot::MethodInfo("modifier_added", godot::PropertyInfo(godot::Variant::OBJECT, "mod", godot::PROPERTY_HINT_NODE_TYPE, "GamePropertyModifier")));
    ADD_SIGNAL(godot::MethodInfo("modifier_removed", godot::PropertyInfo(godot::Variant::OBJECT, "mod", godot::PROPERTY_HINT_NODE_TYPE, "GamePropertyModifier")));
    ADD_SIGNAL(godot::MethodInfo("relabeled", godot::PropertyInfo(godot::Variant::STRING, "new_label")));
    ADD_SIGNAL(godot::MethodInfo("max_value_changed", godot::PropertyInfo(godot::Variant::INT, "oldValue"), godot::PropertyInfo(godot::Variant::INT, "newValue")));
    ADD_SIGNAL(godot::MethodInfo("max_modifier_added", godot::PropertyInfo(godot::Variant::OBJECT, "maxMod", godot::PROPERTY_HINT_NODE_TYPE, "GamePropertyModifier")));
    ADD_SIGNAL(godot::MethodInfo("max_modifier_removed", godot::PropertyInfo(godot::Variant::OBJECT, "maxMod", godot::PROPERTY_HINT_NODE_TYPE, "GamePropertyModifier")));
    ADD_SIGNAL(godot::MethodInfo("min_value_changed", godot::PropertyInfo(godot::Variant::INT, "oldValue"), godot::PropertyInfo(godot::Variant::INT, "newValue")));
    ADD_SIGNAL(godot::MethodInfo("min_modifier_added", godot::PropertyInfo(godot::Variant::OBJECT, "minMod", godot::PROPERTY_HINT_NODE_TYPE, "GamePropertyModifier")));
    ADD_SIGNAL(godot::MethodInfo("min_modifier_removed", godot::PropertyInfo(godot::Variant::OBJECT, "minMod", godot::PROPERTY_HINT_NODE_TYPE, "GamePropertyModifier")));

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_title", "title"), &GameProperty::set_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_title"), &GameProperty::get_title);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "title"), "set_title", "get_title");
    ADD_SIGNAL(godot::MethodInfo("title_changed", godot::PropertyInfo(godot::Variant::STRING, "title")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_restrict_value_to_current_max_value", "restrict"), &GameProperty::set_restrict_value_to_current_max_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_restrict_value_to_current_max_value"), &GameProperty::get_restrict_value_to_current_max_value);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "restrict_value_to_current_max_value"), "set_restrict_value_to_current_max_value", "get_restrict_value_to_current_max_value");

    godot::ClassDB::bind_method(godot::D_METHOD("set_restrict_value_to_current_min_value", "restrict"), &GameProperty::set_restrict_value_to_current_min_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_restrict_value_to_current_min_value"), &GameProperty::get_restrict_value_to_current_min_value);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "restrict_value_to_current_min_value"), "set_restrict_value_to_current_min_value", "get_restrict_value_to_current_min_value");

    godot::ClassDB::bind_method(godot::D_METHOD("set_allow_value_set", "allow"), &GameProperty::set_allow_value_set);
    godot::ClassDB::bind_method(godot::D_METHOD("get_allow_value_set"), &GameProperty::get_allow_value_set);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "allow_value_set"), "set_allow_value_set", "get_allow_value_set");

    godot::ClassDB::bind_method(godot::D_METHOD("set_allow_value_modifiers", "allow"), &GameProperty::set_allow_value_modifiers);
    godot::ClassDB::bind_method(godot::D_METHOD("get_allow_value_modifiers"), &GameProperty::get_allow_value_modifiers);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "allow_value_modifiers"), "set_allow_value_modifiers", "get_allow_value_modifiers");

    godot::ClassDB::bind_method(godot::D_METHOD("set_value", "cv"), &GameProperty::set_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_value"), &GameProperty::get_value);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "value"), "set_value", "get_value");

    godot::ClassDB::bind_method(godot::D_METHOD("set_allow_max_value_set", "allow"), &GameProperty::set_allow_max_value_set);
    godot::ClassDB::bind_method(godot::D_METHOD("get_allow_max_value_set"), &GameProperty::get_allow_max_value_set);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "allow_max_value_set"), "set_allow_max_value_set", "get_allow_max_value_set");

    godot::ClassDB::bind_method(godot::D_METHOD("set_allow_max_modifiers", "allow"), &GameProperty::set_allow_max_modifiers);
    godot::ClassDB::bind_method(godot::D_METHOD("get_allow_max_modifiers"), &GameProperty::get_allow_max_modifiers);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "allow_max_modifiers"), "set_allow_max_modifiers", "get_allow_max_modifiers");

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_value", "mv"), &GameProperty::set_max_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_value"), &GameProperty::get_max_value);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_value"), "set_max_value", "get_max_value");

    godot::ClassDB::bind_method(godot::D_METHOD("set_allow_min_value_set", "allow"), &GameProperty::set_allow_min_value_set);
    godot::ClassDB::bind_method(godot::D_METHOD("get_allow_min_value_set"), &GameProperty::get_allow_min_value_set);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "allow_min_value_set"), "set_allow_min_value_set", "get_allow_min_value_set");

    godot::ClassDB::bind_method(godot::D_METHOD("set_allow_min_modifiers", "allow"), &GameProperty::set_allow_min_modifiers);
    godot::ClassDB::bind_method(godot::D_METHOD("get_allow_min_modifiers"), &GameProperty::get_allow_min_modifiers);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "allow_min_modifiers"), "set_allow_min_modifiers", "get_allow_min_modifiers");

    godot::ClassDB::bind_method(godot::D_METHOD("set_min_value", "mv"), &GameProperty::set_min_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_min_value"), &GameProperty::get_min_value);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "min_value"), "set_min_value", "get_min_value");

    
    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("get_modded_value"), &GameProperty::get_modded_value);
    godot::ClassDB::bind_method(godot::D_METHOD("send_modded_value_label"), &GameProperty::send_modded_value_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_passive_value"), &GameProperty::get_passive_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_modded_max_value"), &GameProperty::get_modded_max_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_modded_min_value"), &GameProperty::get_modded_min_value);
    godot::ClassDB::bind_method(godot::D_METHOD("add_modifier", "mod"), &GameProperty::add_modifier);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_modifier", "modifier"), &GameProperty::remove_modifier);
    godot::ClassDB::bind_method(godot::D_METHOD("add_max_modifier", "mod"), &GameProperty::add_max_modifier);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_max_modifier", "modifier"), &GameProperty::remove_max_modifier);
    godot::ClassDB::bind_method(godot::D_METHOD("add_min_modifier", "mod"), &GameProperty::add_min_modifier);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_min_modifier", "modifier"), &GameProperty::remove_min_modifier);
    godot::ClassDB::bind_method(godot::D_METHOD("game_process", "delta"), &GameProperty::game_process);
    godot::ClassDB::bind_method(godot::D_METHOD("game_process_clear"), &GameProperty::game_process_clear);
    godot::ClassDB::bind_method(godot::D_METHOD("use_as_resource", "useValue"), &GameProperty::use_as_resource);
    godot::ClassDB::bind_method(godot::D_METHOD("action_consequences", "score", "consequences"), &GameProperty::action_consequences);
    godot::ClassDB::bind_method(godot::D_METHOD("save_data"), &GameProperty::save_data);
    godot::ClassDB::bind_method(godot::D_METHOD("load_data", "data"), &GameProperty::load_data);
}

GameProperty::GameProperty() {}
GameProperty::~GameProperty() {}

// Setters / Getters
void GameProperty::set_title(const godot::String& p_title) { 
    if (p_title == title) return;
    title = p_title; 
    emit_signal("title_changed", title);
}
godot::String GameProperty::get_title() const { return title; }

void GameProperty::set_restrict_value_to_current_max_value(bool p_restrict) { 
    if (p_restrict == restrict_value_to_current_max_value) return;
    restrict_value_to_current_max_value = p_restrict; 
}
bool GameProperty::get_restrict_value_to_current_max_value() const { return restrict_value_to_current_max_value; }

void GameProperty::set_restrict_value_to_current_min_value(bool p_restrict) { 
    if (p_restrict == restrict_value_to_current_min_value) return;
    restrict_value_to_current_min_value = p_restrict; 
}
bool GameProperty::get_restrict_value_to_current_min_value() const { return restrict_value_to_current_min_value; }

void GameProperty::set_allow_value_set(bool p_allow) { 
    if (p_allow == allow_value_set) return;
    allow_value_set = p_allow; 
}
bool GameProperty::get_allow_value_set() const { return allow_value_set; }

void GameProperty::set_allow_value_modifiers(bool p_allow) { 
    if (p_allow == allow_value_modifiers) return;
    allow_value_modifiers = p_allow; 
}
bool GameProperty::get_allow_value_modifiers() const { return allow_value_modifiers; }

void GameProperty::set_allow_max_value_set(bool p_allow) { 
    if (p_allow == allow_max_value_set) return;
    allow_max_value_set = p_allow; 
}
bool GameProperty::get_allow_max_value_set() const { return allow_max_value_set; }

void GameProperty::set_allow_max_modifiers(bool p_allow) { 
    if (p_allow == allow_max_modifiers) return;
    allow_max_modifiers = p_allow; 
}
bool GameProperty::get_allow_max_modifiers() const { return allow_max_modifiers; }

void GameProperty::set_allow_min_value_set(bool p_allow) { 
    if (p_allow == allow_min_value_set) return;
    allow_min_value_set = p_allow; 
}
bool GameProperty::get_allow_min_value_set() const { return allow_min_value_set; }

void GameProperty::set_allow_min_modifiers(bool p_allow) { 
    if (p_allow == allow_min_modifiers) return;
    allow_min_modifiers = p_allow; 
}
bool GameProperty::get_allow_min_modifiers() const { return allow_min_modifiers; }

void GameProperty::set_locked(bool p_locked) { 
    if (p_locked == locked) return;
    locked = p_locked; }
bool GameProperty::get_locked() const { return locked; }

void GameProperty::set_is_exhausted(bool p_exhausted) { 
    if (p_exhausted == is_exhausted) return;
    is_exhausted = p_exhausted; }
bool GameProperty::get_is_exhausted() const { return is_exhausted; }

void GameProperty::set_value(int cv) {
    if (locked || !allow_value_set || cv == _value) return;
    int modded_max = get_modded_max_value();
    int modded_min = get_modded_min_value();
    if (cv >= modded_max) {
        _value = modded_max;
        if (_value < cv) emit_signal("value_restored");
    } else if (cv <= modded_min) {
        _value = modded_min;
        if (_value > modded_min) {
            is_exhausted = true;
            emit_signal("value_exhausted");
        }
    } else if (_value == modded_min && cv > modded_min) {
        emit_signal("value_changed", _value, cv);
        _value = cv;
        is_exhausted = false;
        emit_signal("value_no_longer_exhausted");
    } else {
        emit_signal("value_changed", _value, cv);
        _value = cv;
    }
}
int GameProperty::get_value() const { return _value; }

void GameProperty::set_max_value(int mv) {
    if (locked || !allow_max_value_set || mv == _max_value) return;
    int old_value = _max_value;
    _max_value = mv;
    emit_signal("max_value_changed", old_value, _max_value);
}
int GameProperty::get_max_value() const { return _max_value; }

void GameProperty::set_min_value(int mv) {
    if (locked || !allow_min_value_set || mv == _min_value) return;
    _min_value = mv;
    int old_value = _min_value;
    _min_value = mv;
    emit_signal("min_value_changed", old_value, _min_value);
}
int GameProperty::get_min_value() const { return _min_value; }

void GameProperty::set_modifiers(const godot::TypedArray<GamePropertyModifier>& p_modifiers) { 
    if (p_modifiers == modifiers) return;
    modifiers = p_modifiers; 
}
godot::TypedArray<GamePropertyModifier> GameProperty::get_modifiers() const { return modifiers; }

void GameProperty::set_max_modifiers(const godot::TypedArray<GamePropertyModifier>& p_modifiers) { 
    if (p_modifiers == max_modifiers) return;
    max_modifiers = p_modifiers; 
}
godot::TypedArray<GamePropertyModifier> GameProperty::get_max_modifiers() const { return max_modifiers; }

void GameProperty::set_min_modifiers(const godot::TypedArray<GamePropertyModifier>& p_modifiers) { 
    if (p_modifiers == min_modifiers) return;
    min_modifiers = p_modifiers; 
}
godot::TypedArray<GamePropertyModifier> GameProperty::get_min_modifiers() const { return min_modifiers; }

int GameProperty::get_modded_value() const {
    int cv = _value;
    for (int i = 0; i < modifiers.size(); ++i) {
        if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(modifiers[i])) {
            cv += mod->get_value();
        }
    }
    return cv;
}

void GameProperty::send_modded_value_label() {
    emit_signal("relabeled", godot::Variant(get_modded_value()).operator godot::String());
}

int GameProperty::get_passive_value() const {
    int passive = _value / 2;
    for (int i = 0; i < modifiers.size(); ++i) {
        if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(modifiers[i])) {
            passive += mod->get_value();
        }
    }
    return passive;
}

int GameProperty::get_modded_max_value() const {
    int cv = _max_value;
    for (int i = 0; i < max_modifiers.size(); ++i) {
        if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(max_modifiers[i])) {
            cv += mod->get_value();
        }
    }
    return cv;
}

int GameProperty::get_modded_min_value() const {
    int cv = _min_value;
    for (int i = 0; i < min_modifiers.size(); ++i) {
        if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(min_modifiers[i])) {
            cv += mod->get_value();
        }
    }
    return cv;
}

void GameProperty::add_modifier(GamePropertyModifier* mod) {
    if (locked || !allow_value_modifiers || !mod) return;
    if (!modifiers.has(mod)) {
        modifiers.append(mod);
        mod->modifier_start();
        mod->connect("stopped_modifying", godot::Callable(this, "remove_modifier"));
        emit_signal("modifier_added", mod);
    }
}

int GameProperty::get_modifier_id(GamePropertyModifier* modifier) const { return modifiers.find(modifier); }

void GameProperty::remove_modifier(GamePropertyModifier* modifier) {
    int id = get_modifier_id(modifier);
    if (id >= 0) remove_modifier_at(id);
}

void GameProperty::remove_modifier_at(int modID) {
    if (modID >= 0 && modID < modifiers.size()) {
        godot::Object* mod = modifiers[modID];
        emit_signal("modifier_removed", mod);
        modifiers.remove_at(modID);
    }
}

void GameProperty::add_max_modifier(GamePropertyModifier* mod) {
    if (locked || !allow_max_modifiers || !mod) return;
    if (!max_modifiers.has(mod)) {
        max_modifiers.append(mod);
        mod->modifier_start();
        mod->connect("stopped_modifying", godot::Callable(this, "remove_max_modifier"));
        emit_signal("max_modifier_added", mod);
    }
}

int GameProperty::get_max_modifier_id(GamePropertyModifier* modifier) const { return max_modifiers.find(modifier); }

void GameProperty::remove_max_modifier(GamePropertyModifier* modifier) {
    int id = get_max_modifier_id(modifier);
    if (id >= 0) remove_max_modifier_at(id);
}

void GameProperty::remove_max_modifier_at(int modID) {
    if (modID >= 0 && modID < max_modifiers.size()) {
        if (restrict_value_to_current_max_value) {
            int mV = get_modded_max_value();
            if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(max_modifiers[modID])) {
                mV -= mod->get_value();
            }
            if (_value > mV) set_value(mV);
        }
        godot::Object* mod = max_modifiers[modID];
        emit_signal("max_modifier_removed", mod);
        max_modifiers.remove_at(modID);
    }
}

void GameProperty::add_min_modifier(GamePropertyModifier* mod) {
    if (locked || !allow_min_modifiers || !mod) return;
    if (!min_modifiers.has(mod)) {
        min_modifiers.append(mod);
        mod->modifier_start();
        mod->connect("stopped_modifying", godot::Callable(this, "remove_min_modifier"));
        emit_signal("min_modifier_added", mod);
    }
}

int GameProperty::get_min_modifier_id(GamePropertyModifier* modifier) const { return min_modifiers.find(modifier); }

void GameProperty::remove_min_modifier(GamePropertyModifier* modifier) {
    int id = get_min_modifier_id(modifier);
    if (id >= 0) remove_min_modifier_at(id);
}

void GameProperty::remove_min_modifier_at(int modID) {
    if (modID >= 0 && modID < min_modifiers.size()) {
        if (restrict_value_to_current_min_value) {
            int mV = get_modded_min_value();
            if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(min_modifiers[modID])) {
                mV -= mod->get_value();
            }
            if (_value < mV) set_value(mV);
        }
        godot::Object* mod = min_modifiers[modID];
        emit_signal("min_modifier_removed", mod);
        min_modifiers.remove_at(modID);
    }
}

void GameProperty::game_process(double delta) {
    if (game_processed) return;
    for (int i = 0; i < max_modifiers.size(); ++i) {
        if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(max_modifiers[i])) mod->game_process(delta);
    }
    for (int i = 0; i < min_modifiers.size(); ++i) {
        if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(min_modifiers[i])) mod->game_process(delta);
    }
    for (int i = 0; i < modifiers.size(); ++i) {
        if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(modifiers[i])) mod->game_process(delta);
    }
    game_processed = true;
}

void GameProperty::game_process_clear() {
    for (int i = 0; i < max_modifiers.size(); ++i) {
        if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(max_modifiers[i])) mod->game_process_clear();
    }
    for (int i = 0; i < min_modifiers.size(); ++i) {
        if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(min_modifiers[i])) mod->game_process_clear();
    }
    for (int i = 0; i < modifiers.size(); ++i) {
        if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(modifiers[i])) mod->game_process_clear();
    }
    game_processed = false;
}

int GameProperty::use_as_resource(int useValue) {
    if (locked) return 0;
    int resourceValue = 0, overexertion = 0, diff = _value - useValue, min = get_modded_min_value();
    if (diff <= min) {
        int p2e = _value - min;
        overexertion = useValue - p2e;
        useValue = p2e;
    }
    if (useValue == 0) resourceValue = get_passive_value();
    else resourceValue += get_modded_value() * useValue;
    resourceValue -= (_value * overexertion);
    set_value(_value - useValue);
    emit_signal("used_as_resource", useValue, resourceValue);
    return resourceValue;
}

void GameProperty::action_consequences(int score, const godot::Dictionary& consequences) {
    if (consequences.has("restrict_value_to_current_max")) {
        godot::Variant v = consequences["restrict_value_to_current_max"];
        restrict_value_to_current_max_value = (v.get_type() == godot::Variant::STRING) ? (v.operator godot::String() == "score" ? (bool)score : (v.operator godot::String() == "-score" ? (bool)-score : (bool)v)) : (bool)v;
    }
    if (consequences.has("restrict_value_to_current_min")) {
        godot::Variant v = consequences["restrict_value_to_current_min"];
        restrict_value_to_current_min_value = (v.get_type() == godot::Variant::STRING) ? (v.operator godot::String() == "score" ? (bool)score : (v.operator godot::String() == "-score" ? (bool)-score : (bool)v)) : (bool)v;
    }
    if (consequences.has("set_value")) {
        godot::Variant v = consequences["set_value"];
        if (v.get_type() == godot::Variant::STRING) set_value(v.operator godot::String() == "score" ? score : (v.operator godot::String() == "-score" ? -score : (int)v));
        else set_value((int)v);
    }
    if (consequences.has("change_value")) {
        godot::Variant v = consequences["change_value"];
        if (v.get_type() == godot::Variant::STRING) set_value(_value + (v.operator godot::String() == "score" ? score : (v.operator godot::String() == "-score" ? -score : (int)v)));
        else set_value(_value + (int)v);
    }
    if (consequences.has("allow_modifiers")) allow_value_modifiers = consequences["allow_modifiers"];
    if (consequences.has("add_modifier") && allow_value_modifiers) {
        godot::TypedArray<GamePropertyModifier> mods = consequences["add_modifier"];
        for (int i = 0; i < mods.size(); ++i) add_modifier(godot::Object::cast_to<GamePropertyModifier>(mods[i]));
    }
    if (consequences.has("remove_modifier") && allow_value_modifiers) {
        godot::TypedArray<GamePropertyModifier> mods = consequences["remove_modifier"];
        for (int i = 0; i < mods.size(); ++i) remove_modifier(godot::Object::cast_to<GamePropertyModifier>(mods[i]));
    }
    if (consequences.has("allow_max_modifiers")) allow_max_modifiers = consequences["allow_max_modifiers"];
    
    // Fixed: Routed through set_max_value
    if (consequences.has("set_max_value")) {
        godot::Variant v = consequences["set_max_value"];
        if (v.get_type() == godot::Variant::STRING) set_max_value(v.operator godot::String() == "score" ? score : (v.operator godot::String() == "-score" ? -score : (int)v));
        else set_max_value((int)v);
    }
    
    // Fixed: Routed through set_max_value and get_max_value
    if (consequences.has("change_max_value")) {
        godot::Variant v = consequences["change_max_value"];
        if (v.get_type() == godot::Variant::STRING) set_max_value(get_max_value() + (v.operator godot::String() == "score" ? score : (v.operator godot::String() == "-score" ? -score : (int)v)));
        else set_max_value(get_max_value() + (int)v);
    }
    
    if (consequences.has("add_max_modifier") && allow_max_modifiers) {
        godot::TypedArray<GamePropertyModifier> mods = consequences["add_max_modifier"];
        for (int i = 0; i < mods.size(); ++i) add_max_modifier(godot::Object::cast_to<GamePropertyModifier>(mods[i]));
    }
    if (consequences.has("remove_max_modifier") && allow_max_modifiers) {
        godot::TypedArray<GamePropertyModifier> mods = consequences["remove_modifier"];
        for (int i = 0; i < mods.size(); ++i) remove_max_modifier(godot::Object::cast_to<GamePropertyModifier>(mods[i]));
    }
    if (consequences.has("allow_min_modifiers")) allow_min_modifiers = consequences["allow_min_modifiers"];
    
    // Fixed: Routed through set_min_value
    if (consequences.has("set_min_value")) {
        godot::Variant v = consequences["set_min_value"];
        if (v.get_type() == godot::Variant::STRING) set_min_value(v.operator godot::String() == "score" ? score : (v.operator godot::String() == "-score" ? -score : (int)v));
        else set_min_value((int)v);
    }
    
    // Fixed: Routed through set_min_value and get_min_value
    if (consequences.has("change_min_value")) {
        godot::Variant v = consequences["change_min_value"];
        if (v.get_type() == godot::Variant::STRING) set_min_value(get_min_value() + (v.operator godot::String() == "score" ? score : (v.operator godot::String() == "-score" ? -score : (int)v)));
        else set_min_value(get_min_value() + (int)v);
    }
    
    if (consequences.has("add_min_modifier") && allow_min_modifiers) {
        godot::TypedArray<GamePropertyModifier> mods = consequences["add_min_modifier"];
        for (int i = 0; i < mods.size(); ++i) add_min_modifier(godot::Object::cast_to<GamePropertyModifier>(mods[i]));
    }
    if (consequences.has("remove_min_modifier") && allow_min_modifiers) {
        godot::TypedArray<GamePropertyModifier> mods = consequences["remove_modifier"];
        for (int i = 0; i < mods.size(); ++i) remove_min_modifier(godot::Object::cast_to<GamePropertyModifier>(mods[i]));
    }
}

godot::Dictionary GameProperty::save_data() const {
    godot::Dictionary data;
    data["title"] = title;
    data["restrict_value_to_current_max_value"] = restrict_value_to_current_max_value;
    data["restrict_value_to_current_min_value"] = restrict_value_to_current_min_value;
    data["allow_value_set"] = allow_value_set;
    data["allow_value_modifiers"] = allow_value_modifiers;
    if (allow_value_modifiers) {
        godot::Array modAr;
        for (int i = 0; i < modifiers.size(); ++i) {
            if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(modifiers[i])) modAr.append(mod->save_data());
        }
        data["modifiers"] = modAr;
    }
    data["allow_max_value_set"] = allow_max_value_set;
    data["allow_max_modifiers"] = allow_max_modifiers;
    if (allow_max_modifiers) {
        godot::Array modAr;
        for (int i = 0; i < max_modifiers.size(); ++i) {
            if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(max_modifiers[i])) modAr.append(mod->save_data());
        }
        data["max_modifiers"] = modAr;
    }
    data["max_value"] = _max_value;
    data["allow_min_value_set"] = allow_min_value_set;
    data["allow_min_modifiers"] = allow_min_modifiers;
    if (allow_min_modifiers) {
        godot::Array modAr;
        for (int i = 0; i < min_modifiers.size(); ++i) {
            if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(min_modifiers[i])) modAr.append(mod->save_data());
        }
        data["min_modifiers"] = modAr;
    }
    data["min_value"] = _min_value;
    return data;
}

void GameProperty::load_data(const godot::Dictionary& data) {
    if (data.has("title")) title = data["title"];
    if (data.has("restrict_value_to_current_max_value")) restrict_value_to_current_max_value = data["restrict_value_to_current_max_value"];
    if (data.has("restrict_value_to_current_min_value")) restrict_value_to_current_min_value = data["restrict_value_to_current_min_value"];
    if (data.has("allow_value_set")) allow_value_set = data["allow_value_set"];
    if (data.has("allow_value_modifiers")) allow_value_modifiers = data["allow_value_modifiers"];
    
    if (data.has("modifiers")) {
        for (int i = 0; i < modifiers.size(); ++i) { 
            // Fixed: Cast to Node* to access queue_free()
            if (godot::Node* n = godot::Object::cast_to<godot::Node>(modifiers[i])) n->queue_free(); 
        }
        modifiers.clear();
        godot::Array modAr = data["modifiers"];
        for (int i = 0; i < modAr.size(); ++i) {
            GamePropertyModifier* mod = memnew(GamePropertyModifier);
            mod->load_data(modAr[i]);
            add_child(mod);
            modifiers.append(mod);
        }
    }
    
    if (data.has("allow_max_value_set")) allow_max_value_set = data["allow_max_value_set"];
    if (data.has("allow_max_modifiers")) allow_max_modifiers = data["allow_max_modifiers"];
    
    if (data.has("max_modifiers")) {
        for (int i = 0; i < max_modifiers.size(); ++i) { 
            // Fixed: Cast to Node* to access queue_free()
            if (godot::Node* n = godot::Object::cast_to<godot::Node>(max_modifiers[i])) n->queue_free(); 
        }
        max_modifiers.clear();
        godot::Array modAr = data["max_modifiers"];
        for (int i = 0; i < modAr.size(); ++i) {
            GamePropertyModifier* mod = memnew(GamePropertyModifier);
            mod->load_data(modAr[i]);
            add_child(mod);
            max_modifiers.append(mod);
        }
    }
    
    if (data.has("max_value")) _max_value = data["max_value"];
    if (data.has("allow_min_value_set")) allow_min_value_set = data["allow_min_value_set"];
    if (data.has("allow_min_modifiers")) allow_min_modifiers = data["allow_min_modifiers"];
    
    if (data.has("min_modifiers")) {
        for (int i = 0; i < min_modifiers.size(); ++i) { 
            // Fixed: Cast to Node* to access queue_free()
            if (godot::Node* n = godot::Object::cast_to<godot::Node>(min_modifiers[i])) n->queue_free(); 
        }
        min_modifiers.clear();
        godot::Array modAr = data["min_modifiers"];
        for (int i = 0; i < modAr.size(); ++i) {
            GamePropertyModifier* mod = memnew(GamePropertyModifier);
            mod->load_data(modAr[i]);
            add_child(mod);
            min_modifiers.append(mod);
        }
    }
    
    if (data.has("min_value")) _min_value = data["min_value"];
}

} // namespace ideam::godot_ext