#include "../metadata_logic_sub_registry.h"
#include "../../metadata_logic/dsu_cluster_metadata_logic.h"

namespace ideam::core {
template struct MetadataLogicSubRegistry<MetadataLogicID::DSUCluster_Moore_R1>;
template struct MetadataLogicSubRegistry<MetadataLogicID::DSUCluster_VonNeumann_R1>;
}