#ifndef IDEAM_EDITOR_PLUGIN_H
#define IDEAM_EDITOR_PLUGIN_H

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <string_view>

namespace godot {

/**
 * @class IdeamEditorPlugin
 * @brief Abstract base class for the Ideam plugin ecosystem.
 * Handles filesystem standardization and cross-plugin registry discovery.
 */
class IdeamEditorPlugin : public EditorPlugin {
	GDCLASS(IdeamEditorPlugin, EditorPlugin)

protected:
	static void _bind_methods();

	// C++26 Static Constants
	static constexpr std::string_view SCRIPT_TEMPLATE_PATH = "res://script_templates/";
	static constexpr std::string_view GDIGNORE_FILENAME = ".gdignore";
	static constexpr std::string_view REGISTRY_PATH = "res://.ideam_registry.cfg";

	/**
	 * @brief Ensures the project has a valid, ignored script_templates directory.
	 */
	void validate_script_templates();

public:
    IdeamEditorPlugin() = default;
    ~IdeamEditorPlugin() = default;

    // --- Plugin Lifecycle & Lateral Handshake ---
    
    /**
     * @brief Broadcasts the active state of this plugin to the ecosystem.
     */
    void set_plugin_active(const String &p_plugin_name, bool p_active);
    
    /**
     * @brief Checks if another plugin in the Ideam ecosystem is currently active.
     */
    bool is_plugin_active(const String &p_plugin_name) const;

	// --- Universal Ecosystem Registry API ---
	// Exposed to GDScript so standalone plugins can inject their tools.
	
	/**
	 * @brief Writes a value to the universal .ideam_registry.cfg
	 * Use this to inject tools into the Project Wizard or register file paths.
	 */
	void register_to_ecosystem(const String &p_section, const String &p_key, const Variant &p_value);

	/**
	 * @brief Retrieves a value from the registry.
	 */
	Variant get_from_ecosystem(const String &p_section, const String &p_key, const Variant &p_default = Variant()) const;

	/**
	 * @brief Retrieves all keys registered under a specific section (e.g., "WizardTools").
	 */
	TypedArray<String> get_ecosystem_keys(const String &p_section) const;
	
	/**
	 * @brief Retrieves a dictionary of all Key-Value pairs in a section. Useful for building UI lists.
	 */
	Dictionary get_ecosystem_section(const String &p_section) const;
};

} // namespace godot

#endif // IDEAM_EDITOR_PLUGIN_H