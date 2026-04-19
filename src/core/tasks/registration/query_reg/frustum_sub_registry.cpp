// AUTO-GENERATED FILE
#include "../query_logic_sub_registry.h"
#include "../../query_logic/frustum_query_logic.h" // Ensure the compiler sees the logic struct

namespace ideam::core {

// Explicitly instantiate the 4D factory matrix for Frustum
template struct QueryLogicSubRegistry<QueryLogicID::Frustum>;

} // namespace ideam::core
