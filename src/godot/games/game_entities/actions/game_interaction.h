#pragma once

#include "../../game_entity.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameAgent;
class GamePiece;
class GameAgentAction;

// DOD NOTE: GameInteraction heavily relies on loosely-typed Godot Arrays 
// containing mixed types (e.g., `[GameEntity*, int action_id]`). This forces 
// Variant boxing, memory indirection, and runtime type checking. 
// In a high-performance C++26 architecture, this should be refactored into a 
// tightly packed Structure of Arrays (SoA) holding explicit `Participant` structs 
// composed purely of integer handles (EntityID, ActionID) to guarantee L1 cache hits.
class GameInteraction : public GameEntity {
    GDCLASS(GameInteraction, GameEntity)

protected:
    static void _bind_methods();

private:
    godot::Array instigator;
    godot::Array cooperation;
    godot::Array competition;

    bool interaction_active = false;
    
    // DOD NOTE: Hashing objects (GameAgentAction) to Callables inside a Dictionary 
    // is highly inefficient for real-time event routing. Use a flat `std::vector` 
    // or an ECS signaling mechanism to avoid associative lookup overhead.
    godot::Dictionary sequence_connections;

    int instigator_value = 0;
    int competition_value = 0;

public:
    GameInteraction();
    ~GameInteraction();

    // Setters / Getters
    void set_instigator(const godot::Array& p_instigator);
    godot::Array get_instigator() const;

    void set_cooperation(const godot::Array& p_cooperation);
    godot::Array get_cooperation() const;

    void set_competition(const godot::Array& p_competition);
    godot::Array get_competition() const;

    // Class Functions
    void begin_interacting(const godot::Array& inst);
    
    void join_cooperation(const godot::Array& c);
    int cooperation_id(godot::Object* entity) const;
    void leave_cooperation(int cid);

    void join_competition(const godot::Array& c);
    int competition_id(godot::Object* entity) const;
    void leave_competition(int cid);

    void leave(godot::Object* entity);

    void prep_participant(const godot::Array& participant);
    
    void evaluate_interaction(GameAgent* stepped_agent, int sequence_id);
    int score_participant(const godot::Array& participant) const;
    void resolve_participant(const godot::Array& participant, int score_diff);
    
    // DOD NOTE: Applying consequences by parsing Dictionary String keys 
    // ("participant", "all", "cooperation") causes massive branching and string allocation.
    // Replace string-based targeting with a `uint8_t target_mask` and use a data-driven 
    // Command Buffer to resolve consequences without string parsing.
    void apply_consequences(GameEntity* participant, int score, const godot::Dictionary& consequences);
    
    void stop_interacting();
    void stop_participant(const godot::Array& participant);
};

} // namespace ideam::godot_ext