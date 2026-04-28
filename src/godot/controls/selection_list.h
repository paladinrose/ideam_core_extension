// selection_list.h
#pragma once

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/rect2i.hpp>

namespace ideam::godot_ext {

class SelectionList : public godot::HBoxContainer {
    GDCLASS(SelectionList, godot::HBoxContainer)

protected:
    static void _bind_methods();

private:
    godot::Label* label = nullptr;
    godot::Button* selection_button = nullptr;
    godot::PopupPanel* list_popup = nullptr;
    godot::ItemList* list = nullptr;

    bool close_on_select = true;
    godot::String text = "";
    
    godot::TypedArray<godot::String> list_entries;
    godot::String selection_text = "<None>";

public:
    SelectionList();
    ~SelectionList();

    virtual void _ready() override;

    void set_label(godot::Label* p_label);
    godot::Label* get_label() const;

    void set_selection_button(godot::Button* p_button);
    godot::Button* get_selection_button() const;

    void set_list_popup(godot::PopupPanel* p_popup);
    godot::PopupPanel* get_list_popup() const;

    void set_list(godot::ItemList* p_list);
    godot::ItemList* get_list() const;

    void set_close_on_select(bool p_close);
    bool get_close_on_select() const;

    void set_text(const godot::String& p_text);
    godot::String get_text() const;

    void validate_list_elements();
    void build_list(const godot::TypedArray<godot::String>& entries);
    void rebuild_list();
    void set_selection(int id);
    void _select(int id);
    void open_list();
};

} // namespace ideam::godot_ext