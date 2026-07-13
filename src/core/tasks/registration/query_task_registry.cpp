#include "query_task_registry.h"
#include "query_logic_sub_registry.h"
#include <utility>

namespace ideam::core {
namespace { // Translation Unit Firewall
    // --- UI Labels for OptionButtons ---
    constexpr const char* color_labels[] = { "RGBA", "HSVA" };
    constexpr const char* morphological_static_labels[] = { 
        "Moore (R1)", "Moore (R2)", "Moore (R3)", 
        "Von Neumann (R1)", "Von Neumann (R2)", "Von Neumann (R3)" 
    };
} // namespace

// --- Populate the Variant Map ---
const std::array<LogicVariantGroup, QueryTaskRegistry::L_COUNT> QueryTaskRegistry::logic_variants = {{
    // AABB
    { true, 1, nullptr },
    // Archetype
    { true, 1, nullptr },
    // Bitmask
    { true, 1, nullptr },
    // Boolean
    { true, 1, nullptr },
    
    // ColorRGBA
    { true, 2, color_labels },
    // ColorHSVA
    { false, 0, nullptr },
    
    // Component
    { true, 1, nullptr },
    // DataComparison
    { true, 1, nullptr },
    // DataRange
    { true, 1, nullptr },
    // Directional
    { true, 1, nullptr },
    // Distance
    { true, 1, nullptr },
    // EventRingBridge
    { true, 1, nullptr },
    // Frustum
    { true, 1, nullptr },
    // HierarchicalBridge
    { true, 1, nullptr },
    // Limit
    { true, 1, nullptr },
    // Morphological
    { true, 1, nullptr },
    
    // Morphological_Static_Moore_R1
    { true, 6, morphological_static_labels },
    // Morphological_Static_Moore_R2, R3
    { false, 0, nullptr },
    { false, 0, nullptr },
    // Morphological_Static_VonNeumann_R1, R2, R3
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },
    
    // PagedToTiledBridge
    { true, 1, nullptr },
    // Predicate
    { true, 1, nullptr },
    // RelationalBridge
    { true, 1, nullptr },
    // SpatialInclusionBridge
    { true, 1, nullptr },
    // SpatialProjectionBridge
    { true, 1, nullptr },
    // StencilDilationBridge
    { true, 1, nullptr },
    // Stochastic
    { true, 1, nullptr },
    // SwapEruptionBridge
    { true, 1, nullptr }
}};

std::array<const QueryTaskRegistry::SubMatrix*, QueryTaskRegistry::L_COUNT> QueryTaskRegistry::logic_matrices = {};

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
    void generate_ui_all_sub_registries(godot::Dictionary& p_matrix, std::index_sequence<Is...>) {
        (QueryLogicSubRegistry<static_cast<QueryLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
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

void QueryTaskRegistry::generate_ui_matrices(godot::Dictionary& p_matrix) {
    IdeamTaskRegistry::log_with_registry("  -> QueryTaskRegistry: Unrolling templates for L_COUNT = " + godot::itos(L_COUNT));
    // Rips through valid template permutations to build the Godot Inspector UI
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
    IdeamTaskRegistry::log_with_registry("  -> QueryTaskRegistry: Template unrolling complete.");
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