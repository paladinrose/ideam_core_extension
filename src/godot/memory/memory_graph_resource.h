#pragma once

#include "../graphs/ideam_graph_resource.h"
#include "../../core/memory/memory_graph_dod.h"
#include "memory_graph_node_resource.h"
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <memory>

namespace ideam::godot_ext {

class MemoryGraphResource : public IdeamGraphResource {
    GDCLASS(MemoryGraphResource, IdeamGraphResource)

private:
    // Tracks the projected size of the Grant Registry Buffer allocated on the Memory Manager.
    // Represents the maximum number of GrantParts (requirements) this graph can hold.
    int volatile_requirement_capacity = 4096; 

protected:
    static void _bind_methods();

    godot::Ref<ManagedBufferResource> meta_buffer;
    godot::Ref<ManagedBufferResource> registry_buffer;

    // Virtual pipeline override to inject MemoryGraph-specific utility footprints
    virtual void _ensure_managed_buffers() override;
    virtual void _gather_managed_buffers(godot::TypedArray<ManagedBufferResource>& r_buffers) const override;

    godot::Ref<MemoryGraphNodeResource> _get_node_by_name(const godot::StringName& p_name) const;

public:
    MemoryGraphResource() = default;
    virtual ~MemoryGraphResource() override = default;

    void set_volatile_requirement_capacity(int p_cap);
    int get_volatile_requirement_capacity() const;

    void set_meta_buffer(const godot::Ref<ManagedBufferResource>& p_buffer);
    godot::Ref<ManagedBufferResource> get_meta_buffer() const;

    void set_registry_buffer(const godot::Ref<ManagedBufferResource>& p_buffer);
    godot::Ref<ManagedBufferResource> get_registry_buffer() const;
    
    std::shared_ptr<core::MemoryGraphDOD> compile_to_memory_graph(
        core::MemoryManagerDOD* p_manager, 
        godot::HashMap<godot::StringName, core::NodeID>& r_ui_to_dod_map) const;
};

} // namespace ideam::godot_ext

 // IDEAM_GODOT_MEMORY_GRAPH_RESOURCE_H