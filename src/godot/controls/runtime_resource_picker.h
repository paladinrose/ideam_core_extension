#pragma once

#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace ideam::godot_ext {

class RuntimeResourcePicker : public godot::ConfirmationDialog {
    GDCLASS(RuntimeResourcePicker, godot::ConfirmationDialog)

private:
    // Layout containers
    godot::HSplitContainer* main_split = nullptr;
    godot::VBoxContainer* sidebar_vbox = nullptr;
    godot::VBoxContainer* content_vbox = nullptr;

    // Sidebar controls
    godot::Label* sources_title = nullptr;
    godot::Tree* source_tree = nullptr;
    godot::Button* mount_pack_btn = nullptr;
    godot::Button* import_file_btn = nullptr;

    // Content & Filter controls
    godot::HBoxContainer* filter_bar = nullptr;
    godot::LineEdit* search_input = nullptr;
    godot::Button* filter_type_btn = nullptr;
    godot::ItemList* resource_item_list = nullptr;

    // Footer preview & metadata
    godot::PanelContainer* detail_panel = nullptr;
    godot::TextureRect* preview_rect = nullptr;
    godot::Label* selected_name_lbl = nullptr;
    godot::Label* selected_path_lbl = nullptr;
    godot::Label* selected_type_lbl = nullptr;

    // Runtime Dialogs
    godot::FileDialog* pack_file_dialog = nullptr;
    godot::FileDialog* import_file_dialog = nullptr;

    // State trackers
    godot::StringName target_property_name;
    godot::String target_hint_string;
    godot::String current_selected_path;
    godot::String current_search_query;
    godot::String current_folder_prefix = "res://";

    // VFS Caches
    godot::PackedStringArray indexed_resource_paths;
    godot::PackedStringArray mounted_pack_paths;

protected:
    static void _bind_methods();

    void _build_ui();
    void _connect_signals();

    // VFS Indexing and Filtering
    void _scan_vfs();
    void _index_directory_recursive(const godot::String& p_dir_path, godot::PackedStringArray& r_paths);
    bool _matches_type_filter(const godot::String& p_path) const;
    void _rebuild_source_tree();
    void _build_tree_branch(godot::TreeItem* p_parent, const godot::String& p_dir_path);
    void _filter_and_populate_items();
    void _update_preview(const godot::String& p_path);
    bool _supports_runtime_import(const godot::String& p_type) const;
    void _configure_import_dialog_filters();

    // Event callbacks
    void _on_source_tree_item_selected();
    void _on_search_text_changed(const godot::String& p_new_text);
    void _on_resource_item_selected(int p_index);
    void _on_resource_item_activated(int p_index);
    void _on_mount_pack_pressed();
    void _on_pack_file_selected(const godot::String& p_path);
    void _on_import_file_selected(const godot::String& p_path);
    void _on_import_file_pressed();
    void _on_dialog_confirmed();
    void _on_dialog_canceled();
public:
    RuntimeResourcePicker();
    virtual ~RuntimeResourcePicker() override = default;

    void set_target_property(const godot::StringName& p_prop_name, const godot::String& p_hint_string);
    
    godot::StringName get_target_property_name() const { return target_property_name; }
    godot::String get_target_hint_string() const { return target_hint_string; }

    void open_picker();
};

} // namespace ideam::godot_ext