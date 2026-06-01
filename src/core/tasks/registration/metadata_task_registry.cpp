#include "metadata_task_registry.h"
#include "metadata_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

std::array<const MetadataTaskRegistry::SubMatrix*, MetadataTaskRegistry::L_COUNT> MetadataTaskRegistry::logic_matrices = {};


namespace { // Translation Unit Firewall
    // --- Execution Routing (Fast Path) ---
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (MetadataLogicSubRegistry<static_cast<MetadataLogicID>(Is)>::init_execution_routing(), ...);
        ((MetadataTaskRegistry::logic_matrices[Is] = &MetadataLogicSubRegistry<static_cast<MetadataLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (MetadataLogicSubRegistry<static_cast<MetadataLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    // --- UI Matrices (Heavy Path) ---
    template <size_t... Is>
    void generate_ui_all_sub_registries(godot::Dictionary& p_matrix, std::index_sequence<Is...>) {
        (MetadataLogicSubRegistry<static_cast<MetadataLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
    }
} // namespace

void MetadataTaskRegistry::init_execution_routing() {
    // Compiles into consecutive fast-path init calls and pointer assignments
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void MetadataTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void MetadataTaskRegistry::generate_ui_matrices(godot::Dictionary& p_matrix) {
    IdeamTaskRegistry::log_with_registry("  -> MetadataTaskRegistry: Unrolling templates for L_COUNT = " + godot::itos(L_COUNT));
    // Rips through valid template permutations to build the Godot Inspector UI
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
    IdeamTaskRegistry::log_with_registry("  -> MetadataTaskRegistry: Template unrolling complete.");
}


// --- The O(1) Dispatcher ---
std::unique_ptr<INativeTask> MetadataTaskRegistry::create(uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id) {
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