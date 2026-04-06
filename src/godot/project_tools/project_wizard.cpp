#include "project_wizard.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_data_container.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/gd_script.hpp>

namespace godot {

void ProjectWizard::_bind_methods() {
    ADD_SIGNAL(MethodInfo("started"));
    ADD_SIGNAL(MethodInfo("completed"));

    ClassDB::bind_method(D_METHOD("unpack_project_settings", "settings_path"), &ProjectWizard::unpack_project_settings);
    ClassDB::bind_method(D_METHOD("add_project_settings", "path", "other"), &ProjectWizard::add_project_settings);
    ClassDB::bind_method(D_METHOD("build_project_wizard"), &ProjectWizard::build_project_wizard);
    ClassDB::bind_method(D_METHOD("build_project_path_hierarchy"), &ProjectWizard::build_project_path_hierarchy);
}

ProjectWizard::ProjectWizard() {
    project_root.instantiate();
}

ProjectWizard::~ProjectWizard() {
    // Automatic memory cleanup via Ref<T>
}

// --- Helper Implementation for Deep Copying the RefCounted Structs ---
Ref<FolderNode> FolderNode::deep_copy() const {
    Ref<FolderNode> newNode;
    newNode.instantiate();
    
    newNode->root_control = root_control;
    newNode->previous_state = previous_state;
    newNode->check_box = check_box;
    newNode->content_container = content_container;
    newNode->message_label = message_label;

    for (auto const& [key, val] : scripts) {
        Ref<ScriptNode> sn;
        sn.instantiate();
        sn->root_control = val->root_control;
        sn->previous_state = val->previous_state;
        sn->check_box = val->check_box;
        sn->name_edit = val->name_edit;
        sn->message_label = val->message_label;
        sn->settings = val->settings.duplicate();
        newNode->scripts[key] = sn;
    }
    for (auto const& [key, val] : resources) {
        Ref<ResourceNode> rn;
        rn.instantiate();
        rn->root_control = val->root_control;
        rn->previous_state = val->previous_state;
        rn->check_box = val->check_box;
        rn->name_edit = val->name_edit;
        rn->message_label = val->message_label;
        rn->settings = val->settings.duplicate();
        rn->use_generated_check = val->use_generated_check;
        newNode->resources[key] = rn;
    }
    for (auto const& [key, val] : scenes) {
        Ref<SceneNode> scn;
        scn.instantiate();
        scn->root_control = val->root_control;
        scn->previous_state = val->previous_state;
        scn->check_box = val->check_box;
        scn->name_edit = val->name_edit;
        scn->message_label = val->message_label;
        scn->settings = val->settings.duplicate();
        scn->use_generated_check = val->use_generated_check;
        scn->drop_content = val->drop_content;
        newNode->scenes[key] = scn;
    }
    for (auto const& [key, val] : sub_folders) {
        newNode->sub_folders[key] = val->deep_copy();
    }
    return newNode;
}

void ProjectWizard::unpack_project_settings(const String& settings_path) {
    Ref<Resource> res = ResourceLoader::get_singleton()->load(settings_path);
    if (res.is_null()) return;

    Dictionary settings;
    Array folder_paths;
    Array script_settings;
    Array resource_settings;
    Array scene_settings;

    Array raw_folders = res->get("folder_paths");
    for (int i = 0; i < raw_folders.size(); ++i) {
        folder_paths.append(raw_folders[i]);
    }
    settings["folder_paths"] = folder_paths;

    Array raw_scripts = res->get("script_settings");
    for (int i = 0; i < raw_scripts.size(); ++i) {
        Dictionary ss = raw_scripts[i];
        Dictionary script_setting;
        script_setting["generated_script_path"] = ss["generated_script_path"];
        script_setting["base_type"] = ss["base_type"];
        script_settings.append(script_setting);
    }
    settings["script_settings"] = script_settings;

    Array raw_resources = res->get("resource_settings");
    for (int i = 0; i < raw_resources.size(); ++i) {
        Dictionary rs = raw_resources[i];
        Dictionary resource_setting;
        resource_setting["generated_resource_path"] = rs["generated_resource_path"];
        resource_setting["title"] = rs["title"];
        resource_setting["class"] = rs["class"];
        resource_setting["file_extension"] = rs["file_extension"];
        resource_setting["properties"] = rs["properties"].duplicate();
        resource_settings.append(resource_setting);
    }
    settings["resource_settings"] = resource_settings;

    Array raw_scenes = res->get("scene_settings");
    for (int i = 0; i < raw_scenes.size(); ++i) {
        Dictionary scs = raw_scenes[i];
        Dictionary scene_setting;
        scene_setting["generated_scene_path"] = scs["generated_scene_path"];
        scene_setting["title"] = scs["title"];
        scene_setting["node"] = unpack_node_setting(scs["node"]);
        scene_settings.append(scene_setting);
    }
    settings["scene_settings"] = scene_settings;

    add_project_settings(settings_path, settings);
}

Dictionary ProjectWizard::unpack_node_setting(const Variant& packed) {
    Dictionary node_setting;
    node_setting["name"] = packed.get("name");
    node_setting["class"] = packed.get("class");
    node_setting["properties"] = packed.get("properties").duplicate();

    Array sub_nodes;
    Array packed_nodes = packed.get("sub_nodes");
    for (int i = 0; i < packed_nodes.size(); ++i) {
        sub_nodes.append(unpack_node_setting(packed_nodes[i]));
    }
    node_setting["sub_nodes"] = sub_nodes;

    return node_setting;
}

void ProjectWizard::add_project_settings(const String& path, const Dictionary& other) {
    if (project_settings_files.has(path)) return;

    project_settings_files[path] = other;

    if (project_settings.is_empty()) {
        project_settings = other.duplicate(true);
        build_total_folder_hierarchy = true;
        build_folder_hierarchy();
        return;
    }

    add_folder_paths(other["folder_paths"]);
    add_script_settings(other["script_settings"]);
    add_resource_settings(other["resource_settings"]);
    add_scene_settings(other["scene_settings"]);

    build_total_folder_hierarchy = true;
}

void ProjectWizard::add_folder_paths(const Array& other) {
    Array paths = project_settings["folder_paths"];
    for (int i = 0; i < other.size(); ++i) {
        if (paths.find(other[i]) == -1) paths.append(other[i]);
    }
}

void ProjectWizard::add_script_settings(const Array& other) {
    Array settings = project_settings["script_settings"];
    for (int i = 0; i < other.size(); ++i) {
        if (settings.find(other[i]) == -1) settings.append(other[i]);
    }
}

void ProjectWizard::add_resource_settings(const Array& other) {
    Array settings = project_settings["resource_settings"];
    for (int i = 0; i < other.size(); ++i) {
        if (settings.find(other[i]) == -1) settings.append(other[i]);
    }
}

void ProjectWizard::add_scene_settings(const Array& other) {
    Array settings = project_settings["scene_settings"];
    for (int i = 0; i < other.size(); ++i) {
        if (settings.find(other[i]) == -1) settings.append(other[i]);
    }
}

bool ProjectWizard::settings_has_folder_path(const String& path, int skip_id) {
    Array keys = project_settings_files.keys();
    for (int i = 0; i < keys.size(); ++i) {
        if (i == skip_id) continue;
        Dictionary file = project_settings_files[keys[i]];
        Array paths = file["folder_paths"];
        if (paths.has(path)) return true;
    }
    return false;
}

bool ProjectWizard::settings_has_script_settings(const Dictionary& ss, int skip_id) {
    Array keys = project_settings_files.keys();
    for (int i = 0; i < keys.size(); ++i) {
        if (i == skip_id) continue;
        Dictionary file = project_settings_files[keys[i]];
        Array settings = file["script_settings"];
        if (settings.has(ss)) return true;
    }
    return false;
}

bool ProjectWizard::settings_has_resource_settings(const Dictionary& rs, int skip_id) {
    Array keys = project_settings_files.keys();
    for (int i = 0; i < keys.size(); ++i) {
        if (i == skip_id) continue;
        Dictionary file = project_settings_files[keys[i]];
        Array settings = file["resource_settings"];
        if (settings.has(rs)) return true;
    }
    return false;
}

bool ProjectWizard::settings_has_scene_settings(const Dictionary& scs, int skip_id) {
    Array keys = project_settings_files.keys();
    for (int i = 0; i < keys.size(); ++i) {
        if (i == skip_id) continue;
        Dictionary file = project_settings_files[keys[i]];
        Array settings = file["scene_settings"];
        if (settings.has(scs)) return true;
    }
    return false;
}

void ProjectWizard::remove_settings(const Dictionary& other) {
    int id = -1;
    Array values = project_settings_files.values();
    for (int i = 0; i < values.size(); ++i) {
        if (values[i] == other) {
            id = i;
            break;
        }
    }

    if (id < 0) return;

    remove_folder_paths(other["folder_paths"], id);
    remove_script_settings(other["script_settings"], id);
    remove_resource_settings(other["resource_settings"], id);
    remove_scene_settings(other["scene_settings"], id);

    project_settings_files.erase(project_settings_files.keys()[id]);
    build_total_folder_hierarchy = true;
}

void ProjectWizard::remove_folder_paths(const Array& other, int id) {
    Array paths = project_settings["folder_paths"];
    for (int i = 0; i < other.size(); ++i) {
        if (settings_has_folder_path(other[i], id)) continue;
        int idx = paths.find(other[i]);
        if (idx >= 0) paths.remove_at(idx);
    }
}

void ProjectWizard::remove_script_settings(const Array& other, int id) {
    Array settings = project_settings["script_settings"];
    for (int i = 0; i < other.size(); ++i) {
        if (settings_has_script_settings(other[i], id)) continue;
        int idx = settings.find(other[i]);
        if (idx >= 0) settings.remove_at(idx);
    }
}

void ProjectWizard::remove_resource_settings(const Array& other, int id) {
    Array settings = project_settings["resource_settings"];
    for (int i = 0; i < other.size(); ++i) {
        if (settings_has_resource_settings(other[i], id)) continue;
        int idx = settings.find(other[i]);
        if (idx >= 0) settings.remove_at(idx);
    }
}

void ProjectWizard::remove_scene_settings(const Array& other, int id) {
    Array settings = project_settings["scene_settings"];
    for (int i = 0; i < other.size(); ++i) {
        if (settings_has_scene_settings(other[i], id)) continue;
        int idx = settings.find(other[i]);
        if (idx >= 0) settings.remove_at(idx);
    }
}

Ref<FolderNode> ProjectWizard::find_built_folder(const String& settings_path, Ref<FolderNode> folder) {
    if (folder.is_null()) return Ref<FolderNode>();

    PackedStringArray broken_path = settings_path.split("/");
    Ref<FolderNode> current = folder;

    for (int i = 0; i < broken_path.size(); ++i) {
        String path = broken_path[i];
        if (current->sub_folders.count(path)) {
            current = current->sub_folders[path];
        } else {
            return Ref<FolderNode>();
        }
    }
    return current;
}

void ProjectWizard::build_project_wizard() {
    clear_project_wizard();
    build_settings_files_list();
    build_folder_hierarchy();
}

void ProjectWizard::clear_project_wizard() {
    if (!content) return;
    TypedArray<Node> children = content->get_children();
    for (int i = 0; i < children.size(); ++i) {
        Node* child = Object::cast_to<Node>(children[i]);
        child->queue_free();
    }
    
    // Automatically drops the ref count of the old one and generates a fresh one
    project_root.instantiate(); 
}

void ProjectWizard::build_settings_files_list() {
    Array keys = project_settings_files.keys();
    for (int i = 0; i < keys.size(); ++i) {
        String file = keys[i];
        Control* file_check = Object::cast_to<Control>(project_settings_check_template->instantiate());
        project_settings_container->add_child(file_check);

        CheckBox* file_checkbox = Object::cast_to<CheckBox>(file_check->find_child("CheckBox", true, false));
        String label = file.substr(file.rfind("/") + 1);
        file_checkbox->set_text(label);

        file_checkbox->connect("toggled", Callable(this, "select_settings_file").bind(i));
    }
}

void ProjectWizard::build_folder_hierarchy() {
    Control* folder_inst = Object::cast_to<Control>(folder_check_template->instantiate());
    folder_inst->set_name("Root Folder");
    content->add_child(folder_inst);

    Control* root_content = Object::cast_to<Control>(folder_inst->find_child("Content", true, false));
    CheckBox* checkBox = Object::cast_to<CheckBox>(folder_inst->find_child("CheckBox", true, false));
    checkBox->set_text("(Project Path)");
    path_check = checkBox;

    Label* message = Object::cast_to<Label>(folder_inst->find_child("Message", true, false));

    project_root->root_control = folder_inst;
    project_root->check_box = checkBox;
    project_root->content_container = root_content;
    project_root->message_label = message;

    Array folder_paths = project_settings["folder_paths"];
    for (int i = 0; i < folder_paths.size(); ++i) {
        build_settings_path(folder_paths[i], project_root, root_content);
    }

    Array scripts = project_settings["script_settings"];
    for (int i = 0; i < scripts.size(); ++i) {
        Dictionary script_setting = scripts[i];
        Ref<FolderNode> folder = build_settings_path(script_setting["generated_script_path"], project_root, root_content);
        
        Control* script_inst = Object::cast_to<Control>(script_check_template->instantiate());
        script_inst->set_name("Script_" + String(script_setting["base_type"]));
        folder->content_container->add_child(script_inst);

        Ref<ScriptNode> sn;
        sn.instantiate();
        sn->root_control = script_inst;
        sn->check_box = Object::cast_to<CheckBox>(script_inst->find_child("CheckBox", true, false));
        sn->check_box->set_text(script_setting["base_type"]);
        sn->name_edit = Object::cast_to<LineEdit>(script_inst->find_child("LineEdit", true, false));
        sn->message_label = Object::cast_to<Label>(script_inst->find_child("Message", true, false));
        sn->settings = script_setting;

        folder->scripts[script_setting["base_type"]] = sn;
    }

    Array resources = project_settings["resource_settings"];
    for (int i = 0; i < resources.size(); ++i) {
        Dictionary resource_setting = resources[i];
        Ref<FolderNode> folder = build_settings_path(resource_setting["generated_resource_path"], project_root, root_content);

        Control* res_inst = Object::cast_to<Control>(resource_check_template->instantiate());
        res_inst->set_name("Resource_" + String(resource_setting["title"]));
        folder->content_container->add_child(res_inst);

        Ref<ResourceNode> rn;
        rn.instantiate();
        rn->root_control = res_inst;
        rn->check_box = Object::cast_to<CheckBox>(res_inst->find_child("CheckBox", true, false));
        rn->check_box->set_text(resource_setting["title"]);
        rn->name_edit = Object::cast_to<LineEdit>(res_inst->find_child("LineEdit", true, false));
        rn->message_label = Object::cast_to<Label>(res_inst->find_child("Message", true, false));

        Control* drop_content = Object::cast_to<Control>(res_inst->find_child("Content", true, false));
        rn->use_generated_check = memnew(CheckBox);
        rn->use_generated_check->set_name("Use_Generated_Check");
        rn->use_generated_check->set_text("Use Generated Script");
        drop_content->add_child(rn->use_generated_check);
        drop_content->move_child(rn->use_generated_check, 0);

        Control* prop_container = Object::cast_to<Control>(res_inst->find_child("Properties_Container", true, false));
        build_setting_properties(prop_container, resource_setting["properties"]);

        rn->settings = resource_setting;
        folder->resources[resource_setting["title"]] = rn;
    }

    Array scenes = project_settings["scene_settings"];
    for (int i = 0; i < scenes.size(); ++i) {
        Dictionary scene_setting = scenes[i];
        Ref<FolderNode> folder = build_settings_path(scene_setting["generated_scene_path"], project_root, root_content);

        Control* scene_inst = Object::cast_to<Control>(scene_check_template->instantiate());
        scene_inst->set_name("Scene_" + String(scene_setting["title"]));
        folder->content_container->add_child(scene_inst);

        Ref<SceneNode> scn;
        scn.instantiate();
        scn->root_control = scene_inst;
        scn->check_box = Object::cast_to<CheckBox>(scene_inst->find_child("CheckBox", true, false));
        scn->check_box->set_text(scene_setting["title"]);
        scn->name_edit = Object::cast_to<LineEdit>(scene_inst->find_child("LineEdit", true, false));
        scn->message_label = Object::cast_to<Label>(scene_inst->find_child("Message", true, false));
        scn->drop_content = Object::cast_to<Control>(scene_inst->find_child("Content", true, false));

        scn->use_generated_check = memnew(CheckBox);
        scn->use_generated_check->set_name("Use_Generated_Check");
        scn->use_generated_check->set_text("Use Generated Script");
        scn->drop_content->add_child(scn->use_generated_check);
        scn->drop_content->move_child(scn->use_generated_check, 0);

        Control* scene_node_ui = Object::cast_to<Control>(scene_inst->find_child("Scene_Node", true, false));
        build_node_setting(scene_node_ui, scene_setting["node"]);

        scn->settings = scene_setting;
        folder->scenes[scene_setting["title"]] = scn;
    }

    build_path_folders = true;
}

Ref<FolderNode> ProjectWizard::build_settings_path(const String& settings_path, Ref<FolderNode> folder, Control* folder_content) {
    PackedStringArray broken_path = settings_path.split("/");
    Ref<FolderNode> current = folder;
    Control* current_content = folder_content;

    for (int i = 0; i < broken_path.size(); ++i) {
        String path = broken_path[i];
        if (path.is_empty()) continue;

        if (current->sub_folders.count(path)) {
            current = current->sub_folders[path];
            current_content = current->content_container;
        } else {
            Control* inst = Object::cast_to<Control>(folder_check_template->instantiate());
            inst->set_name("Folder_" + path);
            current_content->add_child(inst);

            Ref<FolderNode> new_node;
            new_node.instantiate();
            new_node->root_control = inst;
            new_node->check_box = Object::cast_to<CheckBox>(inst->find_child("CheckBox", true, false));
            new_node->check_box->set_text(path);
            new_node->content_container = Object::cast_to<Control>(inst->find_child("Content", true, false));
            new_node->message_label = Object::cast_to<Label>(inst->find_child("Message", true, false));

            current->sub_folders[path] = new_node;
            current = new_node;
            current_content = new_node->content_container;
        }
    }
    return current;
}

void ProjectWizard::build_setting_properties(Control* prop_container, const Dictionary& prop_settings) {
    Array keys = prop_settings.keys();
    for (int i = 0; i < keys.size(); ++i) {
        String prop_name = keys[i];
        Control* prop_field = Object::cast_to<Control>(property_field_template->instantiate());
        prop_container->add_child(prop_field);

        LineEdit* name_field = Object::cast_to<LineEdit>(prop_field->find_child("Name_Field", true, false));
        name_field->set_text(prop_name);

        LineEdit* value_field = Object::cast_to<LineEdit>(prop_field->find_child("Value_Field", true, false));
        value_field->set_text(prop_settings[prop_name]);
    }
}

void ProjectWizard::build_node_setting(Control* node_field, const Dictionary& node_setting) {
    LineEdit* class_field = Object::cast_to<LineEdit>(node_field->find_child("Class_Field", true, false));
    class_field->set_text(node_setting["class"]);

    Control* prop_container = Object::cast_to<Control>(node_field->find_child("Properties_Container", true, false));
    build_setting_properties(prop_container, node_setting["properties"]);

    Control* sub_node_container = Object::cast_to<Control>(node_field->find_child("Sub_Node_Container", true, false));
    Array sub_nodes = node_setting["sub_nodes"];
    for (int i = 0; i < sub_nodes.size(); ++i) {
        Control* sub_node_inst = Object::cast_to<Control>(node_field_template->instantiate());
        sub_node_container->add_child(sub_node_inst);
        build_node_setting(sub_node_inst, sub_nodes[i]);
    }
}

void ProjectWizard::build_project_path_hierarchy() {
    path_folders.instantiate();

    String project_path = project_path_field->get_text();
    PackedStringArray root_breakdown = project_path.split("/");
    
    path_root = "res://" + root_breakdown[0];
    project_path = project_path.trim_prefix(root_breakdown[0] + String("/"));

    Control* folder_inst = Object::cast_to<Control>(folder_check_template->instantiate());
    folder_inst->set_name(root_breakdown[0]);
    content->add_child(folder_inst);
    folder_inst->hide();

    Control* folder_content = Object::cast_to<Control>(folder_inst->find_child("Content", true, false));
    CheckBox* checkBox = Object::cast_to<CheckBox>(folder_inst->find_child("CheckBox", true, false));
    checkBox->set_text(path_root);
    checkBox->set_pressed(true);

    path_folders->root_control = folder_inst;
    path_folders->check_box = checkBox;
    path_folders->content_container = folder_content;
    path_folders->message_label = Object::cast_to<Label>(folder_inst->find_child("Message", true, false));

    if (root_breakdown.size() > 1) {
        build_settings_path(project_path, path_folders, folder_content);
        auto_include_sub_folders(path_folders);
    }

    build_path_folders = false;
    build_total_folder_hierarchy = true;
}

void ProjectWizard::auto_include_sub_folders(Ref<FolderNode> folder) {
    if (folder.is_null()) return;
    for (auto const& [name, sub] : folder->sub_folders) {
        if (sub->check_box) sub->check_box->set_pressed(true);
        auto_include_sub_folders(sub);
    }
}


void ProjectWizard::build_total_hierarchy() {
    if (path_folders.is_null() || path_folders->sub_folders.empty()) {
        UtilityFunctions::print("build_total_hierarchy: No sub folders!");
        
        total_folder_hierarchy = project_root->deep_copy();
        
        build_total_folder_hierarchy = false;
        return;
    }

    total_folder_hierarchy = path_folders->deep_copy();
    
    Ref<FolderNode> last_path_folder = total_folder_hierarchy;
    String lastKey = "";

    if (!last_path_folder->sub_folders.empty()) {
        bool has_sub_folders = true;
        while (has_sub_folders) {
            auto it = last_path_folder->sub_folders.begin();
            if (it != last_path_folder->sub_folders.end()) {
                lastKey = it->first;
                Ref<FolderNode> f = it->second;

                progress_total += 1;
                last_path_folder = f;

                if (f->sub_folders.empty()) {
                    has_sub_folders = false;
                }
            } else {
                has_sub_folders = false;
            }
        }
    } else {
        lastKey = project_path_field->get_text();
    }

    // Assign the project_root clone to the leaf of the path hierarchy
    last_path_folder->sub_folders[lastKey] = project_root->deep_copy();
    build_total_folder_hierarchy = false;
}

void ProjectWizard::update_path_check(const String& new_text) {
    if (path_check) {
        path_check->set_text(new_text);
    }
}

void ProjectWizard::select_settings_file(int id) {
    Array values = project_settings_files.values();
    if (id < 0 || id >= values.size()) return;

    Dictionary settings_file = values[id];
    
    Array folder_paths = settings_file["folder_paths"];
    for (int i = 0; i < folder_paths.size(); ++i) {
        Ref<FolderNode> folder = find_built_folder(folder_paths[i], project_root);
        if (folder.is_valid() && folder->root_control) folder->root_control->show();
    }

    Array script_settings = settings_file["script_settings"];
    for (int i = 0; i < script_settings.size(); ++i) {
        Dictionary ss = script_settings[i];
        Ref<FolderNode> folder = find_built_folder(ss["generated_script_path"], project_root);
        String base_type = ss["base_type"];
        if (folder.is_valid() && folder->scripts.count(base_type)) {
            folder->scripts[base_type]->root_control->show();
        }
    }

    Array resource_settings = settings_file["resource_settings"];
    for (int i = 0; i < resource_settings.size(); ++i) {
        Dictionary rs = resource_settings[i];
        Ref<FolderNode> folder = find_built_folder(rs["generated_resource_path"], project_root);
        String title = rs["title"];
        if (folder.is_valid() && folder->resources.count(title)) {
            folder->resources[title]->root_control->show();
        }
    }

    Array scene_settings = settings_file["scene_settings"];
    for (int i = 0; i < scene_settings.size(); ++i) {
        Dictionary scs = scene_settings[i];
        Ref<FolderNode> folder = find_built_folder(scs["generated_scene_path"], project_root);
        String title = scs["title"];
        if (folder.is_valid() && folder->scenes.count(title)) {
            folder->scenes[title]->root_control->show();
        }
    }
}

void ProjectWizard::unselect_settings_file(int id) {
    Array values = project_settings_files.values();
    if (id < 0 || id >= values.size()) return;

    Dictionary settings_file = values[id];

    Array folder_paths = settings_file["folder_paths"];
    for (int i = 0; i < folder_paths.size(); ++i) {
        String path = folder_paths[i];
        if (settings_has_folder_path(path, id)) continue;
        Ref<FolderNode> folder = find_built_folder(path, project_root);
        if (folder.is_valid() && folder->root_control) folder->root_control->hide();
    }

    Array script_settings = settings_file["script_settings"];
    for (int i = 0; i < script_settings.size(); ++i) {
        Dictionary ss = script_settings[i];
        if (settings_has_script_settings(ss, id)) continue;
        Ref<FolderNode> folder = find_built_folder(ss["generated_script_path"], project_root);
        String base_type = ss["base_type"];
        if (folder.is_valid() && folder->scripts.count(base_type)) {
            folder->scripts[base_type]->root_control->hide();
        }
    }

    Array resource_settings = settings_file["resource_settings"];
    for (int i = 0; i < resource_settings.size(); ++i) {
        Dictionary rs = resource_settings[i];
        if (settings_has_resource_settings(rs, id)) continue;
        Ref<FolderNode> folder = find_built_folder(rs["generated_resource_path"], project_root);
        String title = rs["title"];
        if (folder.is_valid() && folder->resources.count(title)) {
            folder->resources[title]->root_control->hide();
        }
    }

    Array scene_settings = settings_file["scene_settings"];
    for (int i = 0; i < scene_settings.size(); ++i) {
        Dictionary scs = scene_settings[i];
        if (settings_has_scene_settings(scs, id)) continue;
        Ref<FolderNode> folder = find_built_folder(scs["generated_scene_path"], project_root);
        String title = scs["title"];
        if (folder.is_valid() && folder->scenes.count(title)) {
            folder->scenes[title]->root_control->hide();
        }
    }
}

bool ProjectWizard::validate_project_details() {
    if (build_path_folders) {
        build_project_path_hierarchy();
    }

    if (build_total_folder_hierarchy) {
        build_total_hierarchy();
    }

    progress_total = 0;

    return validate_folder(total_folder_hierarchy, path_root);
}

bool ProjectWizard::validate_folder(Ref<FolderNode> folder, const String& folder_path) {
    if (folder.is_null()) return true;

    bool folder_is_valid = true;
    Label* message = folder->message_label;

    progress_total += 1;

    if (DirAccess::dir_exists_absolute(folder_path)) {
        if (message) message->set_text("Folder Already Exists");
    } else {
        if (message) message->set_text("");
    }

    // Validate Scripts
    for (auto const& [name, script] : folder->scripts) {
        Dictionary script_setting = script->settings;
        String script_file_path = folder_path + String("/") + name + ".gd";

        Label* script_message = script->message_label;

        if (FileAccess::file_exists(script_file_path)) {
            folder_is_valid = false;
            if (script_message) script_message->set_text("A file with this name already exists in that location.");
        } else {
            if (script_message) script_message->set_text("");
        }
        progress_total += 1;
    }

    // Validate Resources
    for (auto const& [name, resource] : folder->resources) {
        Dictionary resource_setting = resource->settings;
        String ext = resource_setting["file_extension"];
        String resource_file_path = folder_path + String("/") + name + ext;

        Label* resource_message = resource->message_label;

        if (FileAccess::file_exists(resource_file_path)) {
            folder_is_valid = false;
            if (resource_message) resource_message->set_text("A file with this name already exists in that location.");
        } else {
            if (resource_message) resource_message->set_text("");
        }
        progress_total += 1;
    }

    // Validate Scenes
    for (auto const& [name, scene] : folder->scenes) {
        String scene_file_path = folder_path + String("/") + name + ".tscn";

        Label* scene_message = scene->message_label;

        if (FileAccess::file_exists(scene_file_path)) {
            folder_is_valid = false;
            if (scene_message) scene_message->set_text("A file with this name already exists in that location.");
        } else {
            if (scene_message) scene_message->set_text("");
        }
        progress_total += 1;
    }

    // Recursively Validate Sub-folders
    for (auto const& [name, sub_folder] : folder->sub_folders) {
        String sub_folder_path = folder_path + String("/") + name;
        if (!validate_folder(sub_folder, sub_folder_path)) {
            folder_is_valid = false;
        }
    }

    return folder_is_valid;
}

void ProjectWizard::generate_project() {
    if (!validate_project_details()) return;

    if (project_page) project_page->hide();
    if (working_page) working_page->show();

    emit_signal("started");

    generate_folders();
}

void ProjectWizard::generate_folders() {
    if (work_message) work_message->set_text("Generating Folders");
    
    generate_folder_recursive(total_folder_hierarchy, path_root);
    
    generate_scripts(); 
}

void ProjectWizard::generate_folder_recursive(Ref<FolderNode> folder, const String& folder_path) {
    if (folder.is_null()) return;

    if (!DirAccess::dir_exists_absolute(folder_path)) {
        DirAccess::make_dir_absolute(folder_path);
    }

    current_progress += 1;
    if (progressBar) progressBar->set_value(current_progress);

    for (auto const& [name, sub_folder] : folder->sub_folders) {
        if (sub_folder->check_box && !sub_folder->check_box->is_pressed()) {
            continue;
        }

        String sub_path = folder_path + String("/") + name;
        generate_folder_recursive(sub_folder, sub_path);
    }
}

void ProjectWizard::check_all_folders(bool check) {
    if (all_resources_check) {
        all_resources_check->set_disabled(!check);
    }
    if (all_scripts_check) {
        all_scripts_check->set_disabled(!check);
    }
    if (all_scenes_check) {
        all_scenes_check->set_disabled(!check);
    }

    all_folders_pass(project_root, path_root, check);
}

void ProjectWizard::all_folders_pass(Ref<FolderNode> folder, const String& folder_path, bool check) {
    if (folder.is_null() || !folder->check_box) return;

    CheckBox* cb = folder->check_box;

    if (check) {
        folder->previous_state = cb->is_pressed();
        cb->set_pressed(true);
        cb->set_disabled(true);
    } else {
        cb->set_pressed(folder->previous_state);
        cb->set_disabled(false);
    }

    for (auto const& [name, sub_folder] : folder->sub_folders) {
        String sub_path = folder_path + String("/") + name;
        all_folders_pass(sub_folder, sub_path, check);
    }
}

void ProjectWizard::generate_scripts() {
    if (work_message) work_message->set_text("Generating Scripts");
    
    generated_scripts.clear();
    
    generate_scripts_in_folder(total_folder_hierarchy, path_root);
    
    generate_resources();
}

void ProjectWizard::generate_scripts_in_folder(Ref<FolderNode> folder, const String& folder_path) {
    if (folder.is_null()) return;

    for (auto const& [title, script] : folder->scripts) {
        if (script->check_box && !script->check_box->is_pressed()) {
            continue;
        }

        String script_name = script->name_edit->get_text();
        String base_type = script->settings["base_type"];
        generate_script_file(script_name, folder_path, base_type);
    }

    for (auto const& [name, sub_folder] : folder->sub_folders) {
        if (sub_folder->check_box && !sub_folder->check_box->is_pressed()) {
            continue;
        }

        String sub_path = folder_path + String("/") + name;
        generate_scripts_in_folder(sub_folder, sub_path);
    }
}

void ProjectWizard::generate_script_file(const String& script_name, const String& scripts_path, const String& base_type) {
    String sanitized_name = script_name.replace(" ", "_");
    
    current_progress += 1;
    if (progressBar) progressBar->set_value(current_progress);

    String template_path = "res://script_templates/" + base_type + "/default_template.gd";
    
    if (!FileAccess::file_exists(template_path)) {
        template_path = "res://script_templates/Node/default_template.gd";
    }

    String script_path = scripts_path + String("/") + sanitized_name.to_lower() + ".gd";

    if (FileAccess::file_exists(script_path)) {
        return;
    }

    Ref<FileAccess> f_template = FileAccess::open(template_path, FileAccess::READ);
    if (f_template.is_null()) return;

    String script_content = f_template->get_as_text();
    script_content = script_content.replace("_BASE_", base_type);
    script_content = script_content.replace("_CLASS_", sanitized_name);
    script_content = "class_name " + sanitized_name + "\n" + script_content;

    Ref<FileAccess> f_script = FileAccess::open(script_path, FileAccess::WRITE);
    if (f_script.is_valid()) {
        f_script->store_string(script_content);
        f_script->flush();
    }

    Ref<Resource> script_object = ResourceLoader::get_singleton()->load(script_path);
    generated_scripts[base_type] = script_object;
}

void ProjectWizard::check_all_scripts(bool check) {
    all_scripts_pass(project_root, path_root, check);
}

void ProjectWizard::all_scripts_pass(Ref<FolderNode> folder, const String& folder_path, bool check) {
    if (folder.is_null()) return;

    for (auto const& [name, script] : folder->scripts) {
        CheckBox* cb = script->check_box;
        if (!cb) continue;

        if (check) {
            script->previous_state = cb->is_pressed();
            cb->set_pressed(true);
            cb->set_disabled(true);
        } else {
            cb->set_pressed(script->previous_state);
            cb->set_disabled(false);
        }
    }

    for (auto const& [name, sub_folder] : folder->sub_folders) {
        String sub_path = folder_path + String("/") + name;
        all_scripts_pass(sub_folder, sub_path, check);
    }
}

void ProjectWizard::generate_resources() {
    if (work_message) work_message->set_text("Generating Resources");
    
    generated_resources.clear();
    
    generate_resources_in_folder(total_folder_hierarchy, path_root);
    
    generate_scenes();
}

void ProjectWizard::generate_resources_in_folder(Ref<FolderNode> folder, const String& folder_path) {
    if (folder.is_null()) return;

    for (auto const& [title, resource] : folder->resources) {
        if (resource->check_box && !resource->check_box->is_pressed()) {
            continue;
        }

        String resource_name = resource->name_edit->get_text();
        bool use_gen = resource->use_generated_check && resource->use_generated_check->is_pressed();
        
        generate_resource_file(resource_name, folder_path, resource->settings, use_gen);
    }

    for (auto const& [name, sub_folder] : folder->sub_folders) {
        if (sub_folder->check_box && !sub_folder->check_box->is_pressed()) {
            continue;
        }

        String sub_path = folder_path + String("/") + name;
        generate_resources_in_folder(sub_folder, sub_path);
    }
}

void ProjectWizard::generate_resource_file(const String& res_name, const String& res_path, const Dictionary& settings, bool use_gen_scripts) {
    Ref<Resource> res;
    String class_name = settings["class"];

    // 1. Attempt to instantiate via generated script
    if (use_gen_scripts && generated_scripts.has(class_name)) {
        Ref<GDScript> gd_script = generated_scripts[class_name];
        if (gd_script.is_valid()) {
            Variant inst = gd_script->new_();
            res = Ref<Resource>(Object::cast_to<Resource>(inst));
        }
    }

    // 2. Fallback to engine ClassDB
    if (res.is_null() && ClassDB::class_exists(class_name) && ClassDB::can_instantiate(class_name)) {
        res = Ref<Resource>(Object::cast_to<Resource>(ClassDB::instantiate(class_name)));
    }

    // 3. Fallback to Global Class List
    if (res.is_null()) {
        Array global_classes = ProjectSettings::get_singleton()->get_global_class_list();
        for (int i = 0; i < global_classes.size(); ++i) {
            Dictionary item = global_classes[i];
            if (item["class"] == class_name) {
                Ref<Resource> loaded_res = ResourceLoader::get_singleton()->load(item["path"]);
                if (loaded_res.is_valid() && loaded_res->has_method("new")) {
                    res = Ref<Resource>(Object::cast_to<Resource>(loaded_res->call("new")));
                }
                break;
            }
        }
    }

    if (res.is_null()) return;

    if (settings.has("properties")) {
        Dictionary props = settings["properties"];
        Array keys = props.keys();
        for (int i = 0; i < keys.size(); ++i) {
            String p_name = keys[i];
            Variant p_val = props[p_name];

            Variant current_val = res->get(p_name);
            if (current_val.get_type() == Variant::OBJECT) {
                String path_str = p_val;
                if (generated_resources.has(path_str)) {
                    p_val = generated_resources[path_str];
                } else if (ResourceLoader::get_singleton()->exists(path_str)) {
                    p_val = ResourceLoader::get_singleton()->load(path_str);
                }
            }
            res->set(p_name, p_val);
        }
    }

    String sanitized_name = res_name.replace(" ", "_");
    current_progress += 1;
    if (progressBar) progressBar->set_value(current_progress);

    String save_path = res_path + String("/") + sanitized_name + "." + String(settings["file_extension"]);
    ResourceSaver::get_singleton()->save(res, save_path);
    
    generated_resources[sanitized_name] = res;
}

void ProjectWizard::check_all_resources(bool check) {
    all_resources_pass(project_root, path_root, check);
}

void ProjectWizard::all_resources_pass(Ref<FolderNode> folder, const String& folder_path, bool check) {
    if (folder.is_null()) return;

    for (auto const& [name, resource] : folder->resources) {
        CheckBox* cb = resource->check_box;
        if (!cb) continue;

        if (check) {
            resource->previous_state = cb->is_pressed();
            cb->set_pressed(true);
            cb->set_disabled(true);
        } else {
            cb->set_pressed(resource->previous_state);
            cb->set_disabled(false);
        }
    }

    for (auto const& [name, sub_folder] : folder->sub_folders) {
        String sub_path = folder_path + String("/") + name;
        all_resources_pass(sub_folder, sub_path, check);
    }
}
void ProjectWizard::generate_scenes() {
    if (work_message) work_message->set_text("Generating Scenes");

    generated_scenes.clear();

    generate_scenes_in_folder(total_folder_hierarchy, path_root);

    finish_generation();
}

void ProjectWizard::generate_scenes_in_folder(Ref<FolderNode> folder, const String& folder_path) {
    if (folder.is_null()) return;

    for (auto const& [title, scene] : folder->scenes) {
        if (scene->check_box && !scene->check_box->is_pressed()) {
            continue;
        }

        String scene_name = scene->name_edit->get_text();
        bool use_gen = scene->use_generated_check && scene->use_generated_check->is_pressed();

        generate_scene_file(scene_name, folder_path, scene->settings, use_gen);
    }

    for (auto const& [name, sub_folder] : folder->sub_folders) {
        if (sub_folder->check_box && !sub_folder->check_box->is_pressed()) {
            continue;
        }

        String sub_path = folder_path + String("/") + name;
        generate_scenes_in_folder(sub_folder, sub_path);
    }
}

void ProjectWizard::generate_scene_file(const String& scn_name, const String& scn_path, const Dictionary& settings, bool use_gen_scripts) {
    String sanitized_name = scn_name.replace(" ", "_");
    current_progress += 1;
    if (progressBar) progressBar->set_value(current_progress);

    String save_path = scn_path + String("/") + sanitized_name + ".tscn";
    Dictionary node_settings = settings["node"];

    // make_node safely returns a Node* because Godot's Tree hierarchy automatically handles memory
    Node* scene_root = make_node(node_settings, use_gen_scripts);
    if (!scene_root) return;

    Ref<PackedScene> packed = memnew(PackedScene);
    packed->pack(scene_root);

    ResourceSaver::get_singleton()->save(packed, save_path);
    generated_scenes[sanitized_name] = packed;

    // Cleanup the temporary node used for packing
    scene_root->queue_free();
}

Node* ProjectWizard::make_node(const Dictionary& node_settings, bool use_gen_scripts) {
    String node_class_name = node_settings["class"];
    Node* node = nullptr;

    // 1. Attempt via generated scripts
    if (use_gen_scripts && generated_scripts.has(node_class_name)) {
        Ref<GDScript> gd_script = generated_scripts[node_class_name];
        if (gd_script.is_valid()) {
            Variant inst = gd_script->new_();
            node = Object::cast_to<Node>(inst);
        }
    }

    // 2. Fallback to ClassDB
    if (!node && ClassDB::class_exists(node_class_name) && ClassDB::can_instantiate(node_class_name)) {
        node = Object::cast_to<Node>(ClassDB::instantiate(node_class_name));
    }

    // 3. Fallback to Global Class List
    if (!node) {
        Array global_classes = ProjectSettings::get_singleton()->get_global_class_list();
        for (int i = 0; i < global_classes.size(); ++i) {
            Dictionary item = global_classes[i];
            if (item["class"] == node_class_name) {
                Ref<Resource> res = ResourceLoader::get_singleton()->load(item["path"]);
                if (res.is_valid() && res->has_method("new")) {
                    node = Object::cast_to<Node>(res->call("new"));
                }
                break;
            }
        }
    }

    if (!node) return nullptr;

    node->set_name(node_settings["name"]);

    // Set Properties
    if (node_settings.has("properties")) {
        Dictionary props = node_settings["properties"];
        Array keys = props.keys();
        for (int i = 0; i < keys.size(); ++i) {
            String p_name = keys[i];
            Variant p_val = props[p_name];

            if (p_val.get_type() == Variant::STRING) {
                String path_str = p_val;
                if (generated_resources.has(path_str)) {
                    p_val = generated_resources[path_str];
                } else if (ResourceLoader::get_singleton()->exists(path_str)) {
                    p_val = ResourceLoader::get_singleton()->load(path_str);
                }
            }
            node->set(p_name, p_val);
        }
    }

    // Recurse Sub-nodes
    if (node_settings.has("sub_nodes")) {
        Array sub_nodes = node_settings["sub_nodes"];
        for (int i = 0; i < sub_nodes.size(); ++i) {
            Node* sub_node = make_node(sub_nodes[i], use_gen_scripts);
            if (sub_node) {
                node->add_child(sub_node);
                sub_node->set_owner(node); 
            }
        }
    }

    return node;
}

void ProjectWizard::check_all_scenes(bool check) {
    all_scenes_pass(project_root, path_root, check);
}

void ProjectWizard::all_scenes_pass(Ref<FolderNode> folder, const String& folder_path, bool check) {
    if (folder.is_null()) return;

    for (auto const& [name, scene] : folder->scenes) {
        CheckBox* cb = scene->check_box;
        if (!cb) continue;

        if (check) {
            scene->previous_state = cb->is_pressed();
            cb->set_pressed(true);
            cb->set_disabled(true);
        } else {
            cb->set_pressed(scene->previous_state);
            cb->set_disabled(false);
        }
    }

    for (auto const& [name, sub_folder] : folder->sub_folders) {
        String sub_path = folder_path + String("/") + name;
        all_scenes_pass(sub_folder, sub_path, check);
    }
}

void ProjectWizard::finish_generation() {
    if (work_message) work_message->set_text("Project Generation Complete.");
    if (progressBar) progressBar->set_value(100.0);

    EditorInterface::get_singleton()->get_resource_filesystem()->scan();

    emit_signal("completed");
}

} // namespace godot