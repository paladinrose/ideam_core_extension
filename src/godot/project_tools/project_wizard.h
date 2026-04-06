#ifndef PROJECT_WIZARD_H
#define PROJECT_WIZARD_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/progress_bar.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <map>

namespace godot {

class ScriptNode : public RefCounted {
    GDCLASS(ScriptNode, RefCounted)
public:
    Control* root_control = nullptr;
    bool previous_state = false;
    CheckBox* check_box = nullptr;
    LineEdit* name_edit = nullptr;
    Label* message_label = nullptr;
    Dictionary settings;

    ScriptNode() = default;
    ~ScriptNode() = default;
};

class ResourceNode : public RefCounted {
    GDCLASS(ResourceNode, RefCounted)
public:
    Control* root_control = nullptr;
    bool previous_state = false;
    CheckBox* check_box = nullptr;
    LineEdit* name_edit = nullptr;
    Label* message_label = nullptr;
    Dictionary settings;
    CheckBox* use_generated_check = nullptr;

    ResourceNode() = default;
    ~ResourceNode() = default;
};

class SceneNode : public RefCounted {
    GDCLASS(SceneNode, RefCounted)
public:
    Control* root_control = nullptr;
    bool previous_state = false;
    CheckBox* check_box = nullptr;
    LineEdit* name_edit = nullptr;
    Label* message_label = nullptr;
    Dictionary settings;
    CheckBox* use_generated_check = nullptr;
    Control* drop_content = nullptr;

    SceneNode() = default;
    ~SceneNode() = default;
};

class FolderNode : public RefCounted {
    GDCLASS(FolderNode, RefCounted)
public:
    Control* root_control = nullptr;
    bool previous_state = false;
    CheckBox* check_box = nullptr;
    Control* content_container = nullptr;
    Label* message_label = nullptr;

    std::map<String, Ref<ScriptNode>> scripts;
    std::map<String, Ref<ResourceNode>> resources;
    std::map<String, Ref<SceneNode>> scenes;
    std::map<String, Ref<FolderNode>> sub_folders;

    FolderNode() = default;
    ~FolderNode() = default;

    Ref<FolderNode> deep_copy() const;
};

class ProjectWizard : public Control {
    GDCLASS(ProjectWizard, Control)

private:
    static void _bind_methods();

    // UI Exports
    Control* project_page = nullptr;
    Control* content = nullptr;
    LineEdit* project_path_field = nullptr;
    Label* project_path_message = nullptr;
    Control* project_settings_container = nullptr;

    Control* working_page = nullptr;
    ProgressBar* progressBar = nullptr;
    Label* work_message = nullptr;

    CheckBox* all_settings_files = nullptr;
    CheckBox* all_folders_check = nullptr;
    CheckBox* all_scripts_check = nullptr;
    CheckBox* all_resources_check = nullptr;
    CheckBox* all_scenes_check = nullptr;

    Ref<PackedScene> project_settings_check_template;
    Ref<PackedScene> folder_check_template;
    Ref<PackedScene> resource_check_template;
    Ref<PackedScene> script_check_template;
    Ref<PackedScene> scene_check_template;
    Ref<PackedScene> property_field_template;
    Ref<PackedScene> node_field_template;

    // Data Structures
    Ref<FolderNode> project_root;
    Ref<FolderNode> path_folders;
    Ref<FolderNode> total_folder_hierarchy;
    
    Dictionary project_settings_files;
    Dictionary project_settings;
    String path_root;

    bool build_path_folders = true;
    bool build_total_folder_hierarchy = true;

    int progress_total = 0;
    int current_progress = 0;

    CheckBox* path_check = nullptr;

    Dictionary generated_scripts;
    Dictionary generated_resources;
    Dictionary generated_scenes;

    // Internal Logic
    Dictionary unpack_node_setting(const Variant& packed);
    void add_folder_paths(const Array& other);
    void add_script_settings(const Array& other);
    void add_resource_settings(const Array& other);
    void add_scene_settings(const Array& other);

    bool settings_has_folder_path(const String& path, int skip_id = -1);
    bool settings_has_script_settings(const Dictionary& script_settings, int skip_id = -1);
    bool settings_has_resource_settings(const Dictionary& resource_settings, int skip_id = -1);
    bool settings_has_scene_settings(const Dictionary& scene_settings, int skip_id = -1);

    void remove_folder_paths(const Array& other, int id = -1);
    void remove_script_settings(const Array& other, int id = -1);
    void remove_resource_settings(const Array& other, int id = -1);
    void remove_scene_settings(const Array& other, int id = -1);

    Ref<FolderNode> find_built_folder(const String& settings_path, Ref<FolderNode> folder);
    
    // UI Helpers
    void build_settings_files_list();
    Ref<FolderNode> build_settings_path(const String& settings_path, Ref<FolderNode> folder, Control* folder_content);
    void build_setting_properties(Control* prop_container, const Dictionary& prop_settings);
    void build_node_setting(Control* node_field, const Dictionary& node_setting);
    void auto_include_sub_folders(Ref<FolderNode> folder);

    // Validation & Generation
    void build_total_hierarchy();
    bool validate_folder(Ref<FolderNode> folder, const String& folder_path);
    void generate_folders();
    void generate_folder_recursive(Ref<FolderNode> folder, const String& folder_path);
    void generate_scripts();
    void generate_scripts_in_folder(Ref<FolderNode> folder, const String& folder_path);
    void generate_script_file(const String& script_name, const String& scripts_path, const String& base_type);
    void generate_resources();
    void generate_resources_in_folder(Ref<FolderNode> folder, const String& folder_path);
    void generate_resource_file(const String& res_name, const String& res_path, const Dictionary& settings, bool use_gen_scripts);
    void generate_scenes();
    void generate_scenes_in_folder(Ref<FolderNode> folder, const String& folder_path);
    void generate_scene_file(const String& scn_name, const String& scn_path, const Dictionary& settings, bool use_gen_scripts);
    Node* make_node(const Dictionary& node_settings, bool use_gen_scripts);
    void finish_generation();

    // Mass selection
    void all_folders_pass(Ref<FolderNode> folder, const String& folder_path, bool check);
    void all_scripts_pass(Ref<FolderNode> folder, const String& folder_path, bool check);
    void all_resources_pass(Ref<FolderNode> folder, const String& folder_path, bool check);
    void all_scenes_pass(Ref<FolderNode> folder, const String& folder_path, bool check);

public:
    ProjectWizard();
    ~ProjectWizard();

    void unpack_project_settings(const String& settings_path);
    void add_project_settings(const String& path, const Dictionary& other);
    void remove_settings(const Dictionary& other);
    
    void build_project_wizard();
    void clear_project_wizard();
    void build_folder_hierarchy();
    void build_project_path_hierarchy();
    void update_path_check(const String& new_text);

    void select_settings_file(int id);
    void unselect_settings_file(int id);

    bool validate_project_details();
    void generate_project();

    void check_all_folders(bool check);
    void check_all_scripts(bool check);
    void check_all_resources(bool check);
    void check_all_scenes(bool check);
};

} // namespace godot

#endif // PROJECT_WIZARD_H