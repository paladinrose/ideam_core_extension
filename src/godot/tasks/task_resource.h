#pragma once

#include "../memory/memory_graph_node_resource.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace ideam::godot_ext {

enum TaskType : uint32_t {
    TASK_GODOT_REFLECTION = 0,
    TASK_NATIVE_CPU       = 1,
    TASK_COMPUTE_GPU      = 2,
    TASK_QUERY_CULLER     = 3
};

class TaskResource : public MemoryGraphNodeResource {
    GDCLASS(TaskResource, MemoryGraphNodeResource)

public:
    enum ExecutionMode {
        EXEC_SERIAL = 0,
        EXEC_PARALLEL = 1
    };

private:
    ExecutionMode exec_mode = EXEC_PARALLEL;
    bool is_active = true;
    TaskType task_type = TASK_NATIVE_CPU;
    
    godot::Dictionary task_properties; 
    godot::StringName task_name;

    godot::Ref<godot::Texture2D> functional_texture;
    godot::Ref<godot::Image> snapshot_image;
    bool preview_folded = false;
   
    int32_t max_preview_size = 256;
    bool snapshot_requested = false;

protected:
    uint32_t logic_id = 0;
    uint32_t base_logic_id = 0; 
    godot::PackedStringArray logic_variant_labels;

    static void _bind_methods();

public:
    TaskResource() = default;
    ~TaskResource() override = default;

    // --- Scheduling Configuration ---
    void set_execution_mode(int p_mode);
    int get_execution_mode() const;

    void set_is_active(bool p_active);
    bool get_is_active() const;

    void set_task_type(int p_type);
    int get_task_type() const;

    void set_task_name(const godot::StringName& p_name);
    godot::StringName get_task_name() const;

    void set_task_properties(const godot::Dictionary& p_props);
    virtual godot::Dictionary get_task_properties() const;

    void set_logic_id(uint32_t p_id);
    uint32_t get_logic_id() const;

    void set_base_logic_id(uint32_t p_id);
    uint32_t get_base_logic_id() const;
    
    void set_logic_variant_labels(const godot::PackedStringArray& p_labels);
    godot::PackedStringArray get_logic_variant_labels() const;

    void set_functional_texture(const godot::Ref<godot::Texture2D>& p_texture);
    godot::Ref<godot::Texture2D> get_functional_texture() const;

    void set_snapshot_image(const godot::Ref<godot::Image>& p_image);
    godot::Ref<godot::Image> get_snapshot_image() const;

    void set_max_preview_size(int32_t p_size);
    int32_t get_max_preview_size() const;

    void request_snapshot();
    bool is_snapshot_requested() const;
    void clear_snapshot_request();

    void set_preview_folded(bool p_folded);
    bool is_preview_folded() const;

    bool validate_for_compilation() const override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::TaskType);
VARIANT_ENUM_CAST(ideam::godot_ext::TaskResource::ExecutionMode);