// AUTO-GENERATED FILE
#include "../transform_logic_sub_registry.h"
#include "../../transform_logic/euler_integration_transform_logic.h" // Ensure the compiler sees the logic struct

namespace ideam::core {

// Explicitly instantiate the factory matrix for EulerIntegration
template struct TransformLogicSubRegistry<TransformLogicID::EulerIntegration>;

} // namespace ideam::core
