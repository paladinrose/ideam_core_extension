#include "query_task_registry.h"
#include "query_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

std::array<const QueryTaskRegistry::SubMatrix*, QueryTaskRegistry::L_COUNT> QueryTaskRegistry::logic_matrices = {};
godot::Dictionary* QueryTaskRegistry::ui_query_matrix = nullptr;

namespace { // TU Firewall
    // --- Execution Routing (Fast Path) ---
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (QueryLogicSubRegistry<static_cast<QueryLogicID>(Is)>::init_execution_routing(), ...);
        ((QueryTaskRegistry::logic_matrices[Is] = &QueryLogicSubRegistry<static_cast<QueryLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (QueryLogicSubRegistry<static_cast<QueryLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    // --- UI Matrices (Heavy Path) ---
    template <size_t... Is>
    void generate_ui_all_sub_registries(std::index_sequence<Is...>) {
        (QueryLogicSubRegistry<static_cast<QueryLogicID>(Is)>::generate_ui_matrices(), ...);
    }
} // namespace

void QueryTaskRegistry::init_execution_routing() {
    // Compiles into consecutive fast-path init calls and pointer assignments
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void QueryTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void QueryTaskRegistry::generate_ui_matrices() {
    if (!ui_query_matrix) {
        ui_query_matrix = new godot::Dictionary();
    }
    
    // Rips through valid template permutations to build the Godot Inspector UI
    generate_ui_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void QueryTaskRegistry::cleanup_ui_matrices() {
    if (ui_query_matrix) {
        delete ui_query_matrix;
        ui_query_matrix = nullptr;
    }
}

// --- The O(1) Dispatcher ---
std::unique_ptr<INativeTask> QueryTaskRegistry::create(uint32_t p_op_id, uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id) {
    if (p_op_id >= O_COUNT || p_logic_id >= L_COUNT || p_view_id >= V_COUNT || p_strategy_id >= S_COUNT || p_type_id >= T_COUNT) {
        return nullptr;
    }

    // 1. O(1) Cache-line hit for the sub-registry pointer
    const SubMatrix* sub_matrix = logic_matrices[p_logic_id];
    if (!sub_matrix) return nullptr;

    // 2. O(1) Flat Index Math (Dropped the L dimension)
    size_t flat_idx = 
        p_op_id * (V_COUNT * S_COUNT * T_COUNT) +
        p_view_id * (S_COUNT * T_COUNT) + 
        p_strategy_id * (T_COUNT) + 
        p_type_id;

    // 3. Hot-path instantiation
    if ((*sub_matrix)[flat_idx]) {
        return std::unique_ptr<INativeTask>((*sub_matrix)[flat_idx]());
    }

    return nullptr;
}

} // namespace ideam::core