#pragma once

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/button.hpp>
#include "memory_manager_resource.h"

namespace ideam::godot_ext {

class MemoryRibbon : public godot::BoxContainer {
    GDCLASS(MemoryRibbon, godot::BoxContainer)

public:
    enum BlockType {
        BLOCK_BUFFER = 0,
        BLOCK_PROFILE = 1,
        BLOCK_TRANSIENT = 2
    };

private:
    godot::Ref<MemoryManagerResource> memory_manager;

    void _clear_ribbon();
    void _build_ribbon();

protected:
    static void _bind_methods();

public:
    MemoryRibbon();
    ~MemoryRibbon();

    void sync_with_resource(godot::Ref<MemoryManagerResource> p_manager);
    
    void set_is_vertical(bool p_vertical);
    bool get_is_vertical() const;

    // Signal Handlers
    void _on_resource_changed();
    void _on_block_pressed(int p_block_type, int p_index);
};

} // namespace ideam::godot_ext