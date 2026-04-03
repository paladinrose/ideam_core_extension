#ifndef IDEAM_CORE_DATA_SCATTER_TRANSFORM_LOGIC_H
#define IDEAM_CORE_DATA_SCATTER_TRANSFORM_LOGIC_H

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include <vector>

namespace ideam::core {

/**
 * DataScatterTransformLogic<T>
 * Writes data from a Source Buffer into the Target Buffer according to an Index Map.
 * Restores physical SoA cache contiguity after logical sorting or spatial hashing.
 */
template <typename T>
struct alignas(64) DataScatterTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr TransformRequirement requirements = TransformRequirement::NONE;
    static constexpr BufferLayoutType supported_layouts = BufferLayoutType::ANY_LINEAR;
    static constexpr size_t transient_workspace_bytes = 0;

    // --- Configuration ---
    uint32_t target_buffer_id = INVALID_ID; 
    uint32_t source_buffer_id = INVALID_ID;
    
    // The sorted index map provided by DataSortTransformLogic or SpatialHash task
    const std::vector<int64_t>* input_index_map = nullptr;

    [[nodiscard]] inline uint32_t get_primary_buffer_id() const {
        return target_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD& context, T_View& main_view) const {
        if (!input_index_map || input_index_map->empty()) return;

        const GrantPartPOD* src_part = context.get_grant_part(source_buffer_id);
        if (!src_part) return;

        const T* source_data = reinterpret_cast<const T*>(src_part->raw_base_ptr);
        const int64_t count = static_cast<int64_t>(input_index_map->size());
        const int64_t* indices = input_index_map->data();

        // The Hot Loop: Random read (source), Linear write (main_view).
        // This is why we scatter: to pay the random-access penalty exactly once per frame.
        for (int64_t i = 0; i < count; ++i) {
            main_view[i] = source_data[indices[i]];
        }
    }
};

} // namespace ideam::core
#endif // IDEAM_CORE_DATA_SCATTER_TRANSFORM_LOGIC_H