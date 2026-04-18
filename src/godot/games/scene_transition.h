#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

// DOD NOTE: While Scene Transitions inherently deal with heavy asset loading, 
// maintaining a node-based, polling architecture for background tasks creates 
// unnecessary cache pressure. In a fully Data-Oriented system, scene loading 
// status should be monitored via a dense integer-indexed state array, updated 
// by a background thread and processed linearly without pointer chasing.
class SceneTransition : public godot::Node {
    GDCLASS(SceneTransition, godot::Node)

protected:
    static void _bind_methods();

private:
    bool _transition_in_progress = false;
    
    godot::Node* _from = nullptr;
    godot::String _to_path;
    godot::Node* _to = nullptr;

public:
    SceneTransition();
    ~SceneTransition();

    // DOD NOTE: Polling a singleton (ResourceLoader) with a string key every 
    // frame forces a string hash map lookup, obliterating L1 cache. In a C++26 
    // optimized architecture, background tasks should push completion flags to a 
    // contiguous lock-free ring buffer (std::atomic arrays) to eliminate polling overhead.
    virtual void _process(double delta) override;

    // Setters / Getters
    void set_transition_in_progress(bool p_in_progress);
    bool get_transition_in_progress() const;

    void set_from(godot::Node* p_from);
    godot::Node* get_from() const;

    void set_to_path(const godot::String& p_to_path);
    godot::String get_to_path() const;

    void set_to(godot::Node* p_to);
    godot::Node* get_to() const;

    // Class Functions
    void start_transition(const godot::String& to_path, godot::Node* from = nullptr);
    void transition();
    
    // DOD NOTE: Instantiating PackedScenes on-the-fly dynamically allocates 
    // an unpredictable tree of variants across the heap. To minimize memory 
    // fragmentation, target objects should ideally be constructed into a pre-allocated 
    // linear allocator (std::pmr::monotonic_buffer_resource) designated for the incoming scene.
    void to_scene_load_complete();
    void complete_transition();
};

} // namespace ideam::godot_ext