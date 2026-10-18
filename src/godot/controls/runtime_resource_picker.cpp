#include "runtime_resource_picker.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/texture2d.hpp>

using namespace godot;

namespace ideam::godot_ext {

void RuntimeResourcePicker::_bind_methods() {
    ADD_SIGNAL(MethodInfo("resource_selected", 
        PropertyInfo(Variant::STRING, "path"),
        PropertyInfo(Variant::STRING_NAME, "property_name")));

    ClassDB::bind_method(D_METHOD("_on_source_tree_item_selected"), &RuntimeResourcePicker::_on_source_tree_item_selected);
    ClassDB::bind_method(D_METHOD("_on_search_text_changed", "new_text"), &RuntimeResourcePicker::_on_search_text_changed);
    ClassDB::bind_method(D_METHOD("_on_resource_item_selected", "index"), &RuntimeResourcePicker::_on_resource_item_selected);
    ClassDB::bind_method(D_METHOD("_on_resource_item_activated", "index"), &RuntimeResourcePicker::_on_resource_item_activated);
    ClassDB::bind_method(D_METHOD("_on_mount_pack_pressed"), &RuntimeResourcePicker::_on_mount_pack_pressed);
    ClassDB::bind_method(D_METHOD("_on_pack_file_selected", "path"), &RuntimeResourcePicker::_on_pack_file_selected);
    ClassDB::bind_method(D_METHOD("_on_import_file_pressed"), &RuntimeResourcePicker::_on_import_file_pressed);
    ClassDB::bind_method(D_METHOD("_on_import_file_selected", "path"), &RuntimeResourcePicker::_on_import_file_selected);
    ClassDB::bind_method(D_METHOD("_on_dialog_confirmed"), &RuntimeResourcePicker::_on_dialog_confirmed);
    ClassDB::bind_method(D_METHOD("_on_dialog_canceled"), &RuntimeResourcePicker::_on_dialog_canceled);

    ClassDB::bind_method(D_METHOD("set_target_property", "prop_name", "hint_string"), &RuntimeResourcePicker::set_target_property);
    ClassDB::bind_method(D_METHOD("open_picker"), &RuntimeResourcePicker::open_picker);
}

RuntimeResourcePicker::RuntimeResourcePicker() {
    set_title("Select Resource");
    set_min_size(Vector2i(850, 550));
    set_wrap_controls(true);

    _build_ui();
    _connect_signals();
}

void RuntimeResourcePicker::_build_ui() {
    main_split = memnew(HSplitContainer);
    main_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    main_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(main_split);

    // ==========================================
    // 1. LEFT SIDEBAR: SOURCES & DIRECT ACTIONS
    // ==========================================
    sidebar_vbox = memnew(VBoxContainer);
    sidebar_vbox->set_custom_minimum_size(Vector2(220, 0));
    sidebar_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    main_split->add_child(sidebar_vbox);

    sources_title = memnew(Label);
    sources_title->set_text("Sources");
    sidebar_vbox->add_child(sources_title);

    source_tree = memnew(Tree);
    source_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    source_tree->set_hide_root(true);
    sidebar_vbox->add_child(source_tree);

    HSeparator* sidebar_sep = memnew(HSeparator);
    sidebar_vbox->add_child(sidebar_sep);

    mount_pack_btn = memnew(Button);
    mount_pack_btn->set_text("Mount Pack (.pck)...");
    sidebar_vbox->add_child(mount_pack_btn);

    import_file_btn = memnew(Button);
    import_file_btn->set_text("Import External File...");
    sidebar_vbox->add_child(import_file_btn);

    // ==========================================
    // 2. RIGHT PANE: SEARCH, ITEMS & PREVIEW
    // ==========================================
    content_vbox = memnew(VBoxContainer);
    content_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    content_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    main_split->add_child(content_vbox);

    filter_bar = memnew(HBoxContainer);
    filter_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    content_vbox->add_child(filter_bar);

    search_input = memnew(LineEdit);
    search_input->set_placeholder("Search resources...");
    search_input->set_clear_button_enabled(true);
    search_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    filter_bar->add_child(search_input);

    filter_type_btn = memnew(Button);
    filter_type_btn->set_text("All Matches");
    filter_bar->add_child(filter_type_btn);

    resource_item_list = memnew(ItemList);
    resource_item_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    resource_item_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    resource_item_list->set_select_mode(ItemList::SELECT_SINGLE);
    resource_item_list->set_max_columns(0);
    resource_item_list->set_same_column_width(true);
    resource_item_list->set_fixed_icon_size(Vector2i(48, 48));
    content_vbox->add_child(resource_item_list);

    detail_panel = memnew(PanelContainer);
    detail_panel->set_custom_minimum_size(Vector2(0, 72));
    content_vbox->add_child(detail_panel);

    HBoxContainer* detail_hbox = memnew(HBoxContainer);
    detail_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    detail_panel->add_child(detail_hbox);

    preview_rect = memnew(TextureRect);
    preview_rect->set_custom_minimum_size(Vector2(64, 64));
    preview_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    preview_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    detail_hbox->add_child(preview_rect);

    VBoxContainer* meta_vbox = memnew(VBoxContainer);
    meta_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    meta_vbox->set_alignment(BoxContainer::ALIGNMENT_CENTER);
    detail_hbox->add_child(meta_vbox);

    selected_name_lbl = memnew(Label);
    selected_name_lbl->set_text("No resource selected");
    meta_vbox->add_child(selected_name_lbl);

    selected_path_lbl = memnew(Label);
    selected_path_lbl->set_text("Path: -");
    selected_path_lbl->set_modulate(Color(0.7, 0.7, 0.7));
    meta_vbox->add_child(selected_path_lbl);

    selected_type_lbl = memnew(Label);
    selected_type_lbl->set_text("Type: -");
    selected_type_lbl->set_modulate(Color(0.5, 0.7, 1.0));
    meta_vbox->add_child(selected_type_lbl);

    // ==========================================
    // 3. PACK MOUNT DIALOG
    // ==========================================
    pack_file_dialog = memnew(FileDialog);
    pack_file_dialog->set_title("Mount Godot Resource Pack");
    pack_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    pack_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
    pack_file_dialog->add_filter("*.pck", "Godot Resource Pack");
    pack_file_dialog->add_filter("*.zip", "ZIP Resource Pack");
    add_child(pack_file_dialog);

    // ==========================================
    // 4. EXTERNAL FILE IMPORT DIALOG
    // ==========================================
    import_file_dialog = memnew(FileDialog);
    import_file_dialog->set_title("Import External File");
    import_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    import_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
    add_child(import_file_dialog);
}

void RuntimeResourcePicker::_connect_signals() {
    source_tree->connect("item_selected", Callable(this, "_on_source_tree_item_selected"));
    search_input->connect("text_changed", Callable(this, "_on_search_text_changed"));
    resource_item_list->connect("item_selected", Callable(this, "_on_resource_item_selected"));
    resource_item_list->connect("item_activated", Callable(this, "_on_resource_item_activated"));

    mount_pack_btn->connect("pressed", Callable(this, "_on_mount_pack_pressed"));
    pack_file_dialog->connect("file_selected", Callable(this, "_on_pack_file_selected"));
    import_file_btn->connect("pressed", Callable(this, "_on_import_file_pressed"));
    import_file_dialog->connect("file_selected", Callable(this, "_on_import_file_selected"));

    connect("confirmed", Callable(this, "_on_dialog_confirmed"));
    connect("canceled", Callable(this, "_on_dialog_canceled"));
}

void RuntimeResourcePicker::set_target_property(const StringName& p_prop_name, const String& p_hint_string) {
    target_property_name = p_prop_name;
    target_hint_string = p_hint_string;

    set_title(p_hint_string.is_empty() ? "Select Resource" : vformat("Select %s Resource", p_hint_string));
    filter_type_btn->set_text(p_hint_string.is_empty() ? "All Types" : p_hint_string);

    // Completely hide import capability if target type lacks a safe runtime importer
    import_file_btn->set_visible(_supports_runtime_import(p_hint_string));
}

void RuntimeResourcePicker::open_picker() {
    current_selected_path = "";
    current_search_query = "";
    current_folder_prefix = "res://";
    search_input->set_text("");

    selected_name_lbl->set_text("No resource selected");
    selected_path_lbl->set_text("Path: -");
    selected_type_lbl->set_text(vformat("Expected: %s", target_hint_string.is_empty() ? "Resource" : target_hint_string));
    preview_rect->set_texture(Ref<Texture2D>());

    _scan_vfs();
    _rebuild_source_tree();
    _filter_and_populate_items();

    popup_centered();
}

// ==========================================
// VFS INDEXING & TREE MANAGEMENT
// ==========================================

void RuntimeResourcePicker::_scan_vfs() {
    indexed_resource_paths.clear();
    _index_directory_recursive("res://", indexed_resource_paths);
}

void RuntimeResourcePicker::_index_directory_recursive(const String& p_dir_path, PackedStringArray& r_paths) {
    Ref<DirAccess> dir = DirAccess::open(p_dir_path);
    if (dir.is_null()) {
        return;
    }

    dir->list_dir_begin();
    String entry_name = dir->get_next();

    while (!entry_name.is_empty()) {
        if (dir->current_is_dir()) {
            if (!entry_name.begins_with(".")) {
                _index_directory_recursive(p_dir_path.path_join(entry_name), r_paths);
            }
        } else {
            String full_path = p_dir_path.path_join(entry_name);
            if (full_path.ends_with(".remap")) {
                full_path = full_path.trim_suffix(".remap");
            }

            if (_matches_type_filter(full_path)) {
                if (!r_paths.has(full_path)) {
                    r_paths.push_back(full_path);
                }
            }
        }
        entry_name = dir->get_next();
    }
    dir->list_dir_end();
}

bool RuntimeResourcePicker::_matches_type_filter(const String& p_path) const {
    String ext = p_path.get_extension().to_lower();
    if (ext == "import" || ext == "uid" || ext == "gd" || ext == "gdextension") {
        return false;
    }

    if (target_hint_string == "Image" || target_hint_string == "Texture2D") {
        if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp" || 
            ext == "svg" || ext == "hdr" || ext == "exr" || ext == "tga" || 
            ext == "bmp" || ext == "ctex" || ext == "res" || ext == "tres") {
            return true;
        }
        return false;
    }

    if (!target_hint_string.is_empty()) {
        PackedStringArray recognized = ResourceLoader::get_singleton()->get_recognized_extensions_for_type(target_hint_string);
        if (recognized.has(ext)) {
            return true;
        }
        if (ext == "res" || ext == "tres") {
            return true;
        }
        return false;
    }

    // Default catch-all for general resources
    return ext == "tres" || ext == "res" || ext == "material" || 
           ext == "mesh" || ext == "tscn" || ext == "scn" || 
           ext == "font" || ext == "theme" || ext == "audio";
}

bool RuntimeResourcePicker::_supports_runtime_import(const String& p_type) const {
    if (p_type.is_empty()) {
        return false; // Generic "Resource" targets are blocked to avoid untrusted .tres/.res deserialization
    }

    // Visual Art & Textures
    if (p_type == "Image" || p_type == "Texture2D") {
        return true;
    }
    // Audio Streams
    if (p_type == "AudioStream" || p_type == "AudioStreamMP3" || 
        p_type == "AudioStreamWAV" || p_type == "AudioStreamOggVorbis") {
        return true;
    }
    // Dynamic Typography
    if (p_type == "Font" || p_type == "FontFile") {
        return true;
    }
    // 3D Meshes & Scene Assets (GLTF runtime parser)
    if (p_type == "PackedScene" || p_type == "GLTFDocument") {
        return true;
    }
    // Video Streams
    if (p_type == "VideoStream" || p_type == "VideoStreamTheora") {
        return true;
    }
    // Plain Data & Structured Configurations
    if (p_type == "ConfigFile" || p_type == "JSON" || 
        p_type == "String" || p_type == "TextFile" || p_type == "CSV") {
        return true;
    }

    return false;
}

void RuntimeResourcePicker::_configure_import_dialog_filters() {
    import_file_dialog->clear_filters();

    if (target_hint_string == "Image" || target_hint_string == "Texture2D") {
        import_file_dialog->add_filter("*.png, *.jpg, *.jpeg, *.webp, *.svg, *.hdr, *.exr, *.tga, *.bmp", "Supported Image Files");
    } else if (target_hint_string == "AudioStream" || target_hint_string == "AudioStreamMP3" || 
               target_hint_string == "AudioStreamWAV" || target_hint_string == "AudioStreamOggVorbis") {
        import_file_dialog->add_filter("*.mp3, *.wav, *.ogg", "All Supported Audio");
        import_file_dialog->add_filter("*.wav", "Waveform Audio (*.wav)");
        import_file_dialog->add_filter("*.mp3", "MP3 Audio (*.mp3)");
        import_file_dialog->add_filter("*.ogg", "Ogg Vorbis Audio (*.ogg)");
    } else if (target_hint_string == "Font" || target_hint_string == "FontFile") {
        import_file_dialog->add_filter("*.ttf, *.otf, *.woff, *.woff2", "TrueType/OpenType Fonts");
    } else if (target_hint_string == "PackedScene" || target_hint_string == "GLTFDocument") {
        import_file_dialog->add_filter("*.gltf, *.glb", "GLTF 3D Models / Scenes");
    } else if (target_hint_string == "VideoStream" || target_hint_string == "VideoStreamTheora") {
        import_file_dialog->add_filter("*.ogv", "Ogg Theora Video (*.ogv)");
    } else if (target_hint_string == "ConfigFile") {
        import_file_dialog->add_filter("*.cfg, *.ini", "Configuration Files (*.cfg, *.ini)");
    } else if (target_hint_string == "JSON") {
        import_file_dialog->add_filter("*.json", "JSON Files (*.json)");
    } else if (target_hint_string == "CSV") {
        import_file_dialog->add_filter("*.csv, *.tsv", "Tabular Data (*.csv, *.tsv)");
    } else if (target_hint_string == "String" || target_hint_string == "TextFile") {
        import_file_dialog->add_filter("*.txt", "Plain Text Files (*.txt)");
    }
}

void RuntimeResourcePicker::_rebuild_source_tree() {
    source_tree->clear();
    TreeItem* root = source_tree->create_item();

    TreeItem* project_item = source_tree->create_item(root);
    project_item->set_text(0, "Project (res://)");
    project_item->set_metadata(0, "res://");
    project_item->set_collapsed(false);

    _build_tree_branch(project_item, "res://");

    TreeItem* packs_item = source_tree->create_item(root);
    packs_item->set_text(0, "Mounted Packs");
    packs_item->set_metadata(0, "packs_root");
    packs_item->set_collapsed(false);

    for (int i = 0; i < mounted_pack_paths.size(); ++i) {
        String pack_path = mounted_pack_paths[i];
        TreeItem* pack_subitem = source_tree->create_item(packs_item);
        pack_subitem->set_text(0, pack_path.get_file());
        pack_subitem->set_metadata(0, pack_path);
    }

    project_item->select(0);
}

void RuntimeResourcePicker::_build_tree_branch(TreeItem* p_parent, const String& p_dir_path) {
    Ref<DirAccess> dir = DirAccess::open(p_dir_path);
    if (dir.is_null()) {
        return;
    }

    dir->list_dir_begin();
    String entry_name = dir->get_next();

    while (!entry_name.is_empty()) {
        if (dir->current_is_dir() && !entry_name.begins_with(".")) {
            String full_sub_dir = p_dir_path.path_join(entry_name);
            TreeItem* sub_item = source_tree->create_item(p_parent);
            sub_item->set_text(0, entry_name);
            sub_item->set_metadata(0, full_sub_dir);
            sub_item->set_collapsed(true);

            _build_tree_branch(sub_item, full_sub_dir);
        }
        entry_name = dir->get_next();
    }
    dir->list_dir_end();
}

void RuntimeResourcePicker::_filter_and_populate_items() {
    resource_item_list->clear();

    String search_lower = current_search_query.to_lower().strip_edges();

    for (int i = 0; i < indexed_resource_paths.size(); ++i) {
        String path = indexed_resource_paths[i];

        if (current_folder_prefix.begins_with("res://") && !path.begins_with(current_folder_prefix)) {
            continue;
        }

        if (!search_lower.is_empty() && !path.to_lower().contains(search_lower)) {
            continue;
        }

        int item_idx = resource_item_list->add_item(path.get_file());
        resource_item_list->set_item_metadata(item_idx, path);
        resource_item_list->set_item_tooltip(item_idx, path);
    }
}

void RuntimeResourcePicker::_update_preview(const String& p_path) {
    preview_rect->set_texture(Ref<Texture2D>());

    String ext = p_path.get_extension().to_lower();
    bool is_image = (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp" || 
                     ext == "svg" || ext == "hdr" || ext == "exr" || ext == "tga" || ext == "bmp");

    if (is_image) {
        Ref<Image> img = Image::load_from_file(p_path);
        if (img.is_valid() && !img->is_empty()) {
            Ref<ImageTexture> tex = ImageTexture::create_from_image(img);
            preview_rect->set_texture(tex);
        }
    } else if (p_path.begins_with("res://")) {
        // Only load packed/project resources; never pass arbitrary external paths to ResourceLoader
        Ref<Resource> res = ResourceLoader::get_singleton()->load(p_path);
        if (res.is_valid()) {
            Ref<Texture2D> tex = Object::cast_to<Texture2D>(res.ptr());
            if (tex.is_valid()) {
                preview_rect->set_texture(tex);
            }
        }
    }

    if (FileAccess::file_exists(p_path)) {
        Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
        if (f.is_valid()) {
            uint64_t file_len = f->get_length();
            double kb = file_len / 1024.0;
            selected_type_lbl->set_text(vformat("Type: %s | Size: %.1f KB", 
                target_hint_string.is_empty() ? ext.to_upper() : target_hint_string, kb));
            return;
        }
    }

    selected_type_lbl->set_text(vformat("Type: %s", target_hint_string.is_empty() ? ext.to_upper() : target_hint_string));
}

// ==========================================
// EVENT HANDLERS
// ==========================================

void RuntimeResourcePicker::_on_source_tree_item_selected() {
    TreeItem* selected = source_tree->get_selected();
    if (!selected) {
        return;
    }

    Variant meta = selected->get_metadata(0);
    if (meta.get_type() == Variant::STRING) {
        String meta_str = meta;
        if (meta_str == "packs_root") {
            current_folder_prefix = "res://";
        } else if (meta_str.begins_with("res://")) {
            current_folder_prefix = meta_str;
        }
    }

    _filter_and_populate_items();
}

void RuntimeResourcePicker::_on_search_text_changed(const String& p_new_text) {
    current_search_query = p_new_text;
    _filter_and_populate_items();
}

void RuntimeResourcePicker::_on_resource_item_selected(int p_index) {
    current_selected_path = resource_item_list->get_item_metadata(p_index);
    String item_title = resource_item_list->get_item_text(p_index);

    selected_name_lbl->set_text(item_title);
    selected_path_lbl->set_text(vformat("Path: %s", current_selected_path));
    _update_preview(current_selected_path);
}

void RuntimeResourcePicker::_on_resource_item_activated(int p_index) {
    _on_resource_item_selected(p_index);
    _on_dialog_confirmed();
    hide();
}

void RuntimeResourcePicker::_on_mount_pack_pressed() {
    pack_file_dialog->popup_centered(Vector2i(700, 480));
}

void RuntimeResourcePicker::_on_pack_file_selected(const String& p_path) {
    bool success = ProjectSettings::get_singleton()->load_resource_pack(p_path, false);
    if (!success) {
        UtilityFunctions::printerr(vformat("[RuntimeResourcePicker] Failed to load resource pack: %s", p_path));
        return;
    }

    if (!mounted_pack_paths.has(p_path)) {
        mounted_pack_paths.push_back(p_path);
    }

    _scan_vfs();
    _rebuild_source_tree();
    _filter_and_populate_items();
}

void RuntimeResourcePicker::_on_import_file_pressed() {
    _configure_import_dialog_filters();
    import_file_dialog->popup_centered(Vector2i(750, 500));
}

void RuntimeResourcePicker::_on_import_file_selected(const String& p_path) {
    current_selected_path = p_path;
    selected_name_lbl->set_text(p_path.get_file());
    selected_path_lbl->set_text(vformat("Path: %s (External)", p_path));
    _update_preview(p_path);
}

void RuntimeResourcePicker::_on_dialog_confirmed() {
    if (current_selected_path.is_empty()) {
        return;
    }

    emit_signal("resource_selected", current_selected_path, target_property_name);
}

void RuntimeResourcePicker::_on_dialog_canceled() {
    current_selected_path = "";
}

} // namespace ideam::godot_ext