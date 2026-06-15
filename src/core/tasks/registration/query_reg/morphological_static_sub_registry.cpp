// AUTO-GENERATED FILE
#include "../query_logic_sub_registry.h"
#include "../../query_logic/morphological_static_query_logic.h" // Ensure the compiler sees the logic struct

namespace ideam::core {

// Explicitly instantiate the 4D factory matrix for MorphologicalStatic
template struct QueryLogicSubRegistry<QueryLogicID::Morphological_Static_Moore_R1>;
template struct QueryLogicSubRegistry<QueryLogicID::Morphological_Static_Moore_R2>;
template struct QueryLogicSubRegistry<QueryLogicID::Morphological_Static_Moore_R3>;
template struct QueryLogicSubRegistry<QueryLogicID::Morphological_Static_VonNeumann_R1>;
template struct QueryLogicSubRegistry<QueryLogicID::Morphological_Static_VonNeumann_R2>;
template struct QueryLogicSubRegistry<QueryLogicID::Morphological_Static_VonNeumann_R3>;

} // namespace ideam::core

