#include "metadata_task_registry.h"
#include "metadata_task.h"

// --- Logics ---
#include "metadata_logic/dsu_cluster_metadata_logic.h"
#include "metadata_logic/group_mask_metadata_logic.h"
#include "metadata_logic/lod_metadata_logic.h"
#include "metadata_logic/partition_metadata_logic.h"

// --- Views ---
#include "../memory/views/single_element_view.h"
#include "../memory/views/static_stencil_view.h"

// --- Strategies ---
#include "../memory/views/strategies.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/array.hpp>
#include <utility>

namespace ideam::core {

// ============================================================================
// THE TRANSLATION UNIT FIREWALL
// Everything inside this anonymous namespace is invisible to the rest of the 
// program. It executes entirely during compilation, bakes into the .rdata 
// segment, and leaves the global symbol table completely clean.
// ============================================================================
namespace { 

    // --- 1. The Tuples (Mapping Types to Enum IDs by Index Order) ---
    // [FUTURE NOTE FOR QUERY/TRANSFORM REGISTRIES]:
    // Metadata tasks have fixed output types, so we used `float` as a baseline here to 
    // satisfy the compiler. For Query and Transform tasks, the Logic depends heavily on 
    // the Buffer Type (e.g., StochasticQuery<Vector3> vs StochasticQuery<int>).
    // For those registries, you MUST add a 4th Dimension: `DataTypeID`.
    // You will iterate over a `CoreDataTypes` tuple first, and dynamically construct
    // `T_Logic<T>` instead of hardcoding `float`.
    using LogicTuple = std::tuple<
        DSUClusterMetadataLogic<float, 9>,
        GroupMaskMetadataLogic<float, 1>,
        LODMetadataLogic<float, 1>,
        PartitionMetadataLogic<float, 1>
    >;

    using StratTuple = std::tuple<
        FlatStrategy,
        Spatial2DStrategy,
        Spatial3DStrategy,
        Spatial4DStrategy
    >;

    // --- 2. The Hole Puncher (Constexpr Matrix Builder) ---
    template <size_t L_ID, size_t V_ID, size_t S_ID>
    consteval MetadataTaskFactoryFn build_single_factory() {
        // Extract the raw types from the tuples using the Enum integer coordinates
        using T_Logic = std::tuple_element_t<L_ID, LogicTuple>;
        using T_Strat = std::tuple_element_t<S_ID, StratTuple>;
        using VType   = typename T_Logic::ValueType;

        // Map View ID to Type (Note: 9 is used as a baseline point count for Stencils)
        using T_View = std::conditional_t<V_ID == static_cast<size_t>(MetadataViewID::SingleElement), 
            SingleElementView<VType, T_Strat>, 
            StaticStencilView<VType, T_Strat, 9> 
        >;

        // VALIDATION: This is where we save Megabytes of binary bloat.
        // We check if the View provides the capabilities the Logic demands.
        constexpr ViewCapability CAPS = ViewTraits<T_View>::capabilities;
        constexpr bool is_valid = MetadataLogicValidator::validate(
            T_Logic::requirements, T_Logic::supported_layouts, CAPS, BufferLayoutType::ANY_LINEAR
        );

        if constexpr (is_valid) {
            // The combination is geometrically sound. Generate the instantiation pointer.
            return []() -> INativeTask* { 
                return new MetadataTask<T_Logic, T_View, T_Strat>(T_Logic{}); 
            };
        } else {
            // THE HOLE: This combination makes no sense (e.g., Spatial Strategy on a Non-Spatial Logic).
            // Do not compile the class. Just drop a null pointer.
            return nullptr;
        }
    }

    // --- 3. Flat Array Expansion ---
    constexpr size_t L_COUNT = static_cast<size_t>(MetadataLogicID::Count);
    constexpr size_t V_COUNT = static_cast<size_t>(MetadataViewID::Count);
    constexpr size_t S_COUNT = static_cast<size_t>(MetadataStrategyID::Count);
    constexpr size_t TOTAL_COMBINATIONS = L_COUNT * V_COUNT * S_COUNT;

    // This magic function turns a 1D sequence (0 to N) into 3D coordinates via stride math,
    // feeding them into the Hole Puncher to build the final flat array.
    template <size_t... Indices>
    consteval std::array<MetadataTaskFactoryFn, TOTAL_COMBINATIONS> build_flat_matrix(std::index_sequence<Indices...>) {
        return { build_single_factory<
            (Indices / (V_COUNT * S_COUNT)) % L_COUNT, // Extract L_ID
            (Indices / S_COUNT) % V_COUNT,             // Extract V_ID
            Indices % S_COUNT                          // Extract S_ID
        >()... };
    }

    // Force the compiler to build the array right now.
    constexpr auto INTERNAL_FACTORIES = build_flat_matrix(std::make_index_sequence<TOTAL_COMBINATIONS>{});

    // --- 4. String Maps for the UI Dictionary ---
    constexpr const char* LOGIC_NAMES[] = { "DSUCluster", "GroupMask", "LOD", "Partition" };
    constexpr const char* VIEW_NAMES[] = { "SingleElement", "StaticStencil" };
    constexpr const char* STRATEGY_NAMES[] = { "Flat", "Spatial2D", "Spatial3D", "Spatial4D" };

} // namespace (End Firewall)

// ============================================================================
// REGISTRY IMPLEMENTATION
// ============================================================================

// Assign the constexpr internal array to the public-facing static member
const std::array<MetadataTaskFactoryFn, TOTAL_COMBINATIONS> MetadataTaskRegistry::factories = INTERNAL_FACTORIES;

godot::Dictionary* MetadataTaskRegistry::ui_metadata_matrix = nullptr;

void MetadataTaskRegistry::init() {
    if (!ui_metadata_matrix) {
        ui_metadata_matrix = new godot::Dictionary();
    }

    // We crawl the compiled array to build the Godot UI.
    // The UI is built ENTIRELY around the "Holes" left by the compiler.
    for (uint32_t l = 0; l < L_COUNT; ++l) {
        godot::Array valid_views;
        godot::Array valid_strats;

        for (uint32_t v = 0; v < V_COUNT; ++v) {
            for (uint32_t s = 0; s < S_COUNT; ++s) {
                size_t flat_idx = (l * V_COUNT * S_COUNT) + (v * S_COUNT) + s;
                
                // If the pointer isn't null, it survived compilation pruning!
                if (factories[flat_idx] != nullptr) {
                    if (!valid_views.has(VIEW_NAMES[v])) valid_views.push_back(VIEW_NAMES[v]);
                    if (!valid_strats.has(STRATEGY_NAMES[s])) valid_strats.push_back(STRATEGY_NAMES[s]);
                }
            }
        }

        // Only register Logics that have at least one valid View/Strategy path
        if (valid_views.size() > 0) {
            godot::Dictionary dict;
            dict["views"] = valid_views;
            dict["strategies"] = valid_strats;
            (*ui_metadata_matrix)[LOGIC_NAMES[l]] = dict;
        }
    }
}

void MetadataTaskRegistry::cleanup() {
    delete ui_metadata_matrix; 
    ui_metadata_matrix = nullptr;
}

std::unique_ptr<INativeTask> MetadataTaskRegistry::create(uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id) {
    // Zero-overhead O(1) lookup. No string hashing, no HashMaps.
    size_t flat_idx = (p_logic_id * V_COUNT * S_COUNT) + (p_view_id * S_COUNT) + p_strategy_id;
    
    if (flat_idx < TOTAL_COMBINATIONS && factories[flat_idx] != nullptr) {
        // Instantiate the specific templated class and wrap it
        return std::unique_ptr<INativeTask>(factories[flat_idx]());
    }

    godot::UtilityFunctions::printerr(
        "MetadataTaskRegistry: Attempted to instantiate an invalid task combination! "
        "Logic: ", p_logic_id, " View: ", p_view_id, " Strat: ", p_strategy_id
    );
    return nullptr;
}

} // namespace ideam::core