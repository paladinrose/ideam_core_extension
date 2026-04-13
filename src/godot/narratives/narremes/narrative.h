#pragma once

#include "../narreme.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Character;
class Prop;
class Location;
class Incident;
class Plot;
class Relationship;

enum BestNarrativeFit : int8_t {
    FIT_BASIC = 0,
    FIT_SYMBOLIC = 1,
    FIT_LITERAL = 2,
    FIT_THOROUGH = 3
};

enum NarrativeReplacement : int8_t {
    REPLACE_SYMBOLIC = 0,
    REPLACE_LITERAL = 1,
    REPLACE_THOROUGH = 2
};

class Narrative : public Narreme {
    GDCLASS(Narrative, Narreme)

private:
    bool begin_narrative_on_ready = true;

    // DOD NOTE: Maintaining separate typed arrays for literals and symbols, 
    // bound together by parallel integer arrays (_assignments), is functional 
    // for authoring but fragments the heap. 
    // In IDEAM Core, these relationships are managed natively via `SparseSetView` 
    // (for entity mapping) or `BridgeView` (for parent-to-child symbolic assignments), 
    // completely eliminating the need to manually sync integer indices.

    // Characters
    godot::TypedArray<Character> characters;
    godot::TypedArray<Character> roles;
    godot::TypedArray<int> role_assignments;

    // Props
    godot::TypedArray<Prop> props;
    godot::TypedArray<Prop> symbols;
    godot::TypedArray<int> symbol_assignments;

    // Locations
    godot::TypedArray<Location> locations;
    godot::TypedArray<Location> places;
    godot::TypedArray<int> place_assignments;

    // Incidents
    godot::TypedArray<Incident> incidents;
    godot::TypedArray<Incident> moments;
    godot::TypedArray<int> moment_assignments;

    // Plots
    godot::TypedArray<Plot> plots;
    godot::TypedArray<Plot> stories;
    godot::TypedArray<int> story_assignments;

protected:
    static void _bind_methods();

public:
    Narrative();
    ~Narrative();

    virtual void initialize() override;

    // Properties Getters & Setters
    void set_begin_narrative_on_ready(bool p_val);
    bool get_begin_narrative_on_ready() const;

    void set_characters(const godot::TypedArray<Character> &p_arr);
    godot::TypedArray<Character> get_characters() const;
    void set_roles(const godot::TypedArray<Character> &p_arr);
    godot::TypedArray<Character> get_roles() const;
    void set_role_assignments(const godot::TypedArray<int> &p_arr);
    godot::TypedArray<int> get_role_assignments() const;

    void set_props(const godot::TypedArray<Prop> &p_arr);
    godot::TypedArray<Prop> get_props() const;
    void set_symbols(const godot::TypedArray<Prop> &p_arr);
    godot::TypedArray<Prop> get_symbols() const;
    void set_symbol_assignments(const godot::TypedArray<int> &p_arr);
    godot::TypedArray<int> get_symbol_assignments() const;

    void set_locations(const godot::TypedArray<Location> &p_arr);
    godot::TypedArray<Location> get_locations() const;
    void set_places(const godot::TypedArray<Location> &p_arr);
    godot::TypedArray<Location> get_places() const;
    void set_place_assignments(const godot::TypedArray<int> &p_arr);
    godot::TypedArray<int> get_place_assignments() const;

    void set_incidents(const godot::TypedArray<Incident> &p_arr);
    godot::TypedArray<Incident> get_incidents() const;
    void set_moments(const godot::TypedArray<Incident> &p_arr);
    godot::TypedArray<Incident> get_moments() const;
    void set_moment_assignments(const godot::TypedArray<int> &p_arr);
    godot::TypedArray<int> get_moment_assignments() const;

    void set_plots(const godot::TypedArray<Plot> &p_arr);
    godot::TypedArray<Plot> get_plots() const;
    void set_stories(const godot::TypedArray<Plot> &p_arr);
    godot::TypedArray<Plot> get_stories() const;
    void set_story_assignments(const godot::TypedArray<int> &p_arr);
    godot::TypedArray<int> get_story_assignments() const;

    // Narrative Base Functions
    void setup_narrative();
    void begin_narrative();
    void end_narrative();
    godot::TypedArray<godot::String> get_symbolic_forms() const;
    godot::TypedArray<godot::String> get_symbolic_lists() const;
    godot::Ref<Narreme> find_literal_from_symbolic(const godot::Ref<Narreme> &narreme) const;
    godot::Ref<Narreme> find_narreme_from_symbolic_relationship(const godot::Ref<Relationship> &relationship) const;

    // Character / Role Functions
    godot::TypedArray<godot::String> gather_character_names() const;
    int add_character(const godot::Ref<Character> &new_character);
    int get_character_id(const godot::Ref<Character> &character) const;
    bool has_character(const godot::Ref<Character> &character) const;
    bool remove_character(const godot::Ref<Character> &character);
    bool remove_character_at(int character_ID);
    godot::TypedArray<int> get_all_roles_character_is_filling(int character_ID) const;

    void gather_roles();
    godot::TypedArray<godot::String> gather_role_names() const;
    int add_role(const godot::Ref<Character> &new_role);
    int get_role_id(const godot::Ref<Character> &role) const;
    bool has_role(const godot::Ref<Character> &role) const;
    bool remove_role(const godot::Ref<Character> &role);
    bool remove_role_at(int role_ID);
    int get_best_fit_character_for_role(int role_ID, const godot::TypedArray<int> &character_mask, BestNarrativeFit best_fit);
    bool fill_role(int role_ID, int character_ID, NarrativeReplacement replace_type);
    bool vacate_role(int role_ID);

    // Prop / Symbol Functions
    godot::TypedArray<godot::String> gather_prop_names() const;
    int add_prop(const godot::Ref<Prop> &new_prop);
    int get_prop_id(const godot::Ref<Prop> &prop) const;
    bool has_prop(const godot::Ref<Prop> &prop) const;
    bool remove_prop(const godot::Ref<Prop> &prop);
    bool remove_prop_at(int prop_ID);
    godot::TypedArray<int> get_all_symbols_prop_is_filling(int prop_ID) const;

    void gather_symbols();
    godot::TypedArray<godot::String> gather_symbol_names() const;
    int add_symbol(const godot::Ref<Prop> &new_symbol);
    int get_symbol_id(const godot::Ref<Prop> &symbol) const;
    bool has_symbol(const godot::Ref<Prop> &symbol) const;
    bool remove_symbol(const godot::Ref<Prop> &symbol);
    bool remove_symbol_at(int symbol_ID);
    int get_best_fit_prop_for_symbol(int symbol_ID, const godot::TypedArray<int> &prop_mask, BestNarrativeFit best_fit);
    bool fill_symbol(int symbol_ID, int prop_ID, NarrativeReplacement replace_type);
    bool vacate_symbol(int symbol_ID);

    // Location / Place Functions
    godot::TypedArray<godot::String> gather_location_names() const;
    int add_location(const godot::Ref<Location> &new_location);
    int get_location_id(const godot::Ref<Location> &location) const;
    bool has_location(const godot::Ref<Location> &location) const;
    bool remove_location(const godot::Ref<Location> &location);
    bool remove_location_at(int location_ID);
    godot::TypedArray<int> get_all_places_location_is_filling(int location_ID) const;

    void gather_places();
    godot::TypedArray<godot::String> gather_place_names() const;
    int add_place(const godot::Ref<Location> &new_place);
    int get_place_id(const godot::Ref<Location> &place) const;
    bool has_place(const godot::Ref<Location> &place) const;
    bool remove_place(const godot::Ref<Location> &place);
    bool remove_place_at(int place_ID);
    int get_best_fit_location_for_place(int place_ID, const godot::TypedArray<int> &location_mask, BestNarrativeFit best_fit);
    bool fill_place(int place_ID, int location_ID, NarrativeReplacement replace_type);
    bool vacate_place(int place_ID);

    // Incident / Moment Functions
    godot::TypedArray<godot::String> gather_incident_names() const;
    int add_incident(const godot::Ref<Incident> &new_incident);
    int get_incident_id(const godot::Ref<Incident> &incident) const;
    bool has_incident(const godot::Ref<Incident> &incident) const;
    bool remove_incident(const godot::Ref<Incident> &incident);
    bool remove_incident_at(int incident_ID);
    godot::TypedArray<int> get_all_moments_incident_is_filling(int incident_ID) const;

    void gather_moments();
    godot::TypedArray<godot::String> gather_moment_names() const;
    int add_moment(const godot::Ref<Incident> &new_moment);
    int get_moment_id(const godot::Ref<Incident> &moment) const;
    bool has_moment(const godot::Ref<Incident> &moment) const;
    bool remove_moment(const godot::Ref<Incident> &moment);
    bool remove_moment_at(int moment_ID);
    int get_best_fit_incident_for_moment(int moment_ID, const godot::TypedArray<int> &incident_mask, BestNarrativeFit best_fit);
    bool fill_moment(int moment_ID, int incident_ID, NarrativeReplacement replace_type);
    bool vacate_moment(int moment_ID);

    // Plot / Story Functions
    godot::TypedArray<godot::String> gather_plot_names() const;
    int add_plot(const godot::Ref<Plot> &new_plot);
    int get_plot_id(const godot::Ref<Plot> &plot) const;
    bool has_plot(const godot::Ref<Plot> &plot) const;
    bool remove_plot(const godot::Ref<Plot> &plot);
    bool remove_plot_at(int plot_ID);
    godot::TypedArray<int> get_all_storys_plot_is_filling(int plot_ID) const;

    void gather_stories();
    godot::TypedArray<godot::String> gather_story_names() const;
    int add_story(const godot::Ref<Plot> &new_story);
    int get_story_id(const godot::Ref<Plot> &story) const;
    bool has_story(const godot::Ref<Plot> &story) const;
    bool remove_story(const godot::Ref<Plot> &story);
    bool remove_story_at(int story_ID);
    int get_best_fit_plot_for_story(int story_ID, const godot::TypedArray<int> &plot_mask, BestNarrativeFit best_fit);
    bool fill_story(int story_ID, int plot_ID, NarrativeReplacement replace_type);
    bool vacate_story(int story_ID);

    // Utilities / Overrides
    void path_has_changed(const godot::Ref<Narreme> &node);
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const override;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const override;
    virtual godot::String get_class_name_str() const override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::BestNarrativeFit);
VARIANT_ENUM_CAST(ideam::godot_ext::NarrativeReplacement);