#include "../metadata_logic_sub_registry.h"
#include "../../metadata_logic/partition_metadata_logic.h"

namespace ideam::core {
template struct MetadataLogicSubRegistry<MetadataLogicID::Partition_1>;
template struct MetadataLogicSubRegistry<MetadataLogicID::Partition_2>;
template struct MetadataLogicSubRegistry<MetadataLogicID::Partition_3>;
template struct MetadataLogicSubRegistry<MetadataLogicID::Partition_4>;
}