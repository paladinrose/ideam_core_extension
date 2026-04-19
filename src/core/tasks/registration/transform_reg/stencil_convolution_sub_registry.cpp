#include "../transform_logic_sub_registry.h"
#include "../../transform_logic/stencil_convolution_transform_logic.h"

namespace ideam::core {
template struct TransformLogicSubRegistry<TransformLogicID::Stencil_Moore_R1>;
template struct TransformLogicSubRegistry<TransformLogicID::Stencil_Moore_R2>;
template struct TransformLogicSubRegistry<TransformLogicID::Stencil_Moore_R3>;
template struct TransformLogicSubRegistry<TransformLogicID::Stencil_VonNeumann_R1>;
template struct TransformLogicSubRegistry<TransformLogicID::Stencil_VonNeumann_R2>;
template struct TransformLogicSubRegistry<TransformLogicID::Stencil_VonNeumann_R3>;
}