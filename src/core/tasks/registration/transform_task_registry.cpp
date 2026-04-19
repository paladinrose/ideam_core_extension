#include "transform_task_registry.h"
#include "transform_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

std::array<const TransformTaskRegistry::SubMatrix*, TransformTaskRegistry::L_COUNT> TransformTaskRegistry::logic_matrices = {};
godot::Dictionary* TransformTaskRegistry::ui_transform_matrix = nullptr;

namespace { // Translation Unit Firewall
    // --- C++20 Fold Expression Auto-Registration ---
    template <size_t... Is>
    void init_all_sub_registries(std::index_sequence<Is...>) {
        (TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::init(), ...);
        ((TransformTaskRegistry::logic_matrices[Is] = &TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_all_sub_registries(std::index_sequence<Is...>) {
        (TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::cleanup(), ...);
    }
} // namespace

void TransformTaskRegistry::init() {
    if (!ui_transform_matrix) {
        ui_transform_matrix = new godot::Dictionary();
    }
    
    // Compiles into 14 consecutive init() calls and pointer assignments
    init_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void TransformTaskRegistry::cleanup() {
    cleanup_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);

    if (ui_transform_matrix) {
        delete ui_transform_matrix;
        ui_transform_matrix = nullptr;
    }
}

// --- The O(1) Dispatcher ---
std::unique_ptr<INativeTask> TransformTaskRegistry::create(uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id) {
    if (p_logic_id >= L_COUNT || p_view_id >= V_COUNT || p_strategy_id >= S_COUNT || p_type_id >= T_COUNT) {
        return nullptr;
    }

    // 1. O(1) Cache-line hit for the sub-registry pointer
    const SubMatrix* sub_matrix = logic_matrices[p_logic_id];
    if (!sub_matrix) return nullptr;

    // 2. O(1) Flat Index Math (Dropped the L dimension)
    size_t flat_idx = 
        p_view_id * (S_COUNT * T_COUNT) +
        p_strategy_id * (T_COUNT) +
        p_type_id;

    if ((*sub_matrix)[flat_idx]) {
        return std::unique_ptr<INativeTask>((*sub_matrix)[flat_idx]());
    }

    return nullptr;
}

} // namespace ideam::core