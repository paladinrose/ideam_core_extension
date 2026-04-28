// selection_list.cpp
#include "selection_list.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void SelectionList::_bind_methods() {
    ADD_SIGNAL(godot::MethodInfo("selection_changed", godot::PropertyInfo(godot::Variant::INT, "id")));

    godot::ClassDB::bind_method(godot::D_METHOD("validate_list_elements"), &SelectionList::validate_list_elements);
    godot::ClassDB::bind_method(godot::D_METHOD("build_list", "entries"), &SelectionList::build_list);
    godot::ClassDB::bind_method(godot::D_METHOD("rebuild_list"), &SelectionList::rebuild_list);
    godot::ClassDB::bind_method(godot::D_METHOD("set_selection", "id"), &SelectionList::set_selection);
    godot::ClassDB::bind_method(godot::D_METHOD("_select", "id"), &SelectionList::_select);
    godot::ClassDB::bind_method(godot::D_METHOD("open_list"), &SelectionList::open_list);

    godot::ClassDB::bind_method(godot::D_METHOD("set_label", "label"), &SelectionList::set_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_label"), &SelectionList::get_label);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "label", godot::PROPERTY_HINT_NODE_TYPE, "Label"), "set_label", "get_label");

    godot::ClassDB::bind_method(godot::D_METHOD("set_selection_button", "selection_button"), &SelectionList::set_selection_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_selection_button"), &SelectionList::get_selection_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "selection_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_selection_button", "get_selection_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_list_popup", "list_popup"), &SelectionList::set_list_popup);
    godot::ClassDB::bind_method(godot::D_METHOD("get_list_popup"), &SelectionList::get_list_popup);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "list_popup", godot::PROPERTY_HINT_NODE_TYPE, "PopupPanel"), "set_list_popup", "get_list_popup");

    godot::ClassDB::bind_method(godot::D_METHOD("set_list", "list"), &SelectionList::set_list);
    godot::ClassDB::bind_method(godot::D_METHOD("get_list"), &SelectionList::get_list);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "list", godot::PROPERTY_HINT_NODE_TYPE, "ItemList"), "set_list", "get_list");

    godot::ClassDB::bind_method(godot::D_METHOD("set_close_on_select", "close_on_select"), &SelectionList::set_close_on_select);
    godot::ClassDB::bind_method(godot::D_METHOD("get_close_on_select"), &SelectionList::get_close_on_select);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "close_on_select"), "set_close_on_select", "get_close_on_select");

    godot::ClassDB::bind_method(godot::D_METHOD("set_text", "text"), &SelectionList::set_text);
    godot::ClassDB::bind_method(godot::D_METHOD("get_text"), &SelectionList::get_text);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "text"), "set_text", "get_text");
}

SelectionList::SelectionList() {
    validate_list_elements();
}

SelectionList::~SelectionList() {}

void SelectionList::_ready() {
    validate_list_elements();
    
    if (list_entries.size() > 0) {
        rebuild_list();
    }
}

void SelectionList::set_label(godot::Label* p_label) { label = p_label; }
godot::Label* SelectionList::get_label() const { return label; }

void SelectionList::set_selection_button(godot::Button* p_button) { selection_button = p_button; }
godot::Button* SelectionList::get_selection_button() const { return selection_button; }

void SelectionList::set_list_popup(godot::PopupPanel* p_popup) { list_popup = p_popup; }
godot::PopupPanel* SelectionList::get_list_popup() const { return list_popup; }

void SelectionList::set_list(godot::ItemList* p_list) { list = p_list; }
godot::ItemList* SelectionList::get_list() const { return list; }

void SelectionList::set_close_on_select(bool p_close) { close_on_select = p_close; }
bool SelectionList::get_close_on_select() const { return close_on_select; }

void SelectionList::set_text(const godot::String& p_text) {
    if (text == p_text) {
        return;
    }
    text = p_text;
    if (label != nullptr) {
        label->set_text(text);
    }
}

void SelectionList::validate_list_elements() {
    godot::Node* new_owner = get_owner();
    
    if (label == nullptr) {
        label = memnew(godot::Label);
        add_child(label);
        label->set_name("Label");
        label->set_owner(new_owner);
        label->set_h_size_flags(godot::Control::SIZE_SHRINK_BEGIN);
        label->set_text(text);
    }
    
    if (selection_button == nullptr) {
        selection_button = memnew(godot::Button);
        add_child(selection_button);
        selection_button->set_name("Selection_Button");
        selection_button->set_text(selection_text);
        selection_button->set_owner(new_owner);
        selection_button->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    }
    
    if (selection_button != nullptr && !selection_button->is_connected("pressed", godot::Callable(this, "open_list"))) {
        selection_button->connect("pressed", godot::Callable(this, "open_list"));
    }
    
    if (list_popup == nullptr) {
        list_popup = memnew(godot::PopupPanel);
        add_child(list_popup);
        list_popup->set_name("Popup");
        list_popup->hide();
        list_popup->set_owner(new_owner);
    }
    
    if (list == nullptr) {
        list = memnew(godot::ItemList);
        if (list_popup != nullptr) {
            list_popup->add_child(list);
        }
        list->set_name("List");
        list->set_owner(new_owner);
    }
    
    if (list != nullptr && !list->is_connected("item_selected", godot::Callable(this, "_select"))) {
        list->connect("item_selected", godot::Callable(this, "_select"));
    }
}

void SelectionList::build_list(const godot::TypedArray<godot::String>& entries) {
    list_entries = entries.duplicate();
    if (list != nullptr) {
        rebuild_list();
    }
}

void SelectionList::rebuild_list() {
    if (list == nullptr) {
        return;
    }
        
    list->clear();
    list->add_item("<None>");
    
    for (int i = 0; i < list_entries.size(); ++i) {
        list->add_item(list_entries[i]);
    }
}

void SelectionList::set_selection(int id) {
    id += 1;
    _select(id);
}

void SelectionList::_select(int id) {
    id -= 1;
    if (id < 0) {
        selection_text = "<None>";
    } else {
        if (id < list_entries.size()) {
            selection_text = list_entries[id];
        }
    }
    
    if (selection_button == nullptr) {
        return;
    }
    
    selection_button->set_text(selection_text);
    emit_signal("selection_changed", id);
    
    if (close_on_select && list_popup != nullptr) {
        list_popup->hide();
    }
}

void SelectionList::open_list() {
    if (list_popup == nullptr) {
        return;
    }

    godot::Vector2i pos = get_screen_position();
    pos.y += get_size().y;
    godot::Rect2i rect = godot::Rect2i(pos, godot::Vector2i(get_size().x, 150));
    list_popup->popup(rect);
}

} // namespace ideam::godot_ext