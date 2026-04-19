#include "../metadata_logic_sub_registry.h"
#include "../../metadata_logic/lod_metadata_logic.h"

namespace ideam::core {
template struct MetadataLogicSubRegistry<MetadataLogicID::LOD_1Level>;
template struct MetadataLogicSubRegistry<MetadataLogicID::LOD_2Level>;
template struct MetadataLogicSubRegistry<MetadataLogicID::LOD_3Level>;
template struct MetadataLogicSubRegistry<MetadataLogicID::LOD_4Level>;
}