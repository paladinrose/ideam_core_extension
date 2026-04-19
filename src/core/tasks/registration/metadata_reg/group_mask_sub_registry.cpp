#include "../metadata_logic_sub_registry.h"
#include "../../metadata_logic/group_mask_metadata_logic.h"

namespace ideam::core {
template struct MetadataLogicSubRegistry<MetadataLogicID::GroupMask_1Bit>;
template struct MetadataLogicSubRegistry<MetadataLogicID::GroupMask_2Bit>;
template struct MetadataLogicSubRegistry<MetadataLogicID::GroupMask_3Bit>;
template struct MetadataLogicSubRegistry<MetadataLogicID::GroupMask_4Bit>;
}