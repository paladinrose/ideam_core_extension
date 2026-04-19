import os
from pathlib import Path

def get_enum_name(base_name: str) -> str:
    """Converts a snake_case filename to the PascalCase LogicID enum."""
    if base_name == "aabb":
        return "AABB"
    # e.g., 'paged_to_tiled_bridge' -> 'PagedToTiledBridge'
    return "".join(word.capitalize() for word in base_name.split("_"))

def main():
    base_dir = Path("src/core/tasks")

    # The Configuration Dictionary for our 3 DOD axes
    targets = [
        {
            "name": "Query",
            "logic_dir": "query_logic",
            "reg_dir": "query_reg",
            "suffix": "_query_logic",
            "header_include": "query_logic_sub_registry.h",
            "enum_prefix": "QueryLogicID",
            "struct_prefix": "QueryLogicSubRegistry"
        },
        {
            "name": "Metadata",
            "logic_dir": "metadata_logic",
            "reg_dir": "metadata_reg",
            "suffix": "_metadata_logic",
            "header_include": "metadata_logic_sub_registry.h",
            "enum_prefix": "MetadataLogicID",
            "struct_prefix": "MetadataLogicSubRegistry"
        },
        {
            "name": "Transform",
            "logic_dir": "transform_logic",
            "reg_dir": "transform_reg",
            "suffix": "_transform_logic",
            "header_include": "transform_logic_sub_registry.h",
            "enum_prefix": "TransformLogicID",
            "struct_prefix": "TransformLogicSubRegistry"
        }
    ]

    total_generated = 0
    total_skipped = 0

    # Process each configuration sequentially
    for target in targets:
        print(f"\n--- Processing {target['name']} Registries ---")
        
        logic_dir = base_dir / target["logic_dir"]
        reg_dir = base_dir / "registration" / target["reg_dir"]

        # Ensure the target directory exists
        reg_dir.mkdir(parents=True, exist_ok=True)

        # Find all header files ending in the correct suffix
        search_pattern = f"*{target['suffix']}.h"
        logic_files = list(logic_dir.glob(search_pattern))
        
        if not logic_files:
            print(f"No logic headers found in {logic_dir}. Check your paths!")
            continue

        generated_count = 0
        skipped_count = 0

        for logic_path in logic_files:
            # Extract filename and base logical name
            filename = logic_path.stem
            base_name = filename.replace(target["suffix"], "")
            
            new_filename = f"{base_name}_sub_registry.cpp"
            dest_path = reg_dir / new_filename

            # Skip if it already exists
            if dest_path.exists():
                print(f"Skipping {new_filename} (Already exists)")
                skipped_count += 1
                continue

            enum_id = get_enum_name(base_name)

            # Generate the C++ boilerplate specifically tailored to this DOD axis
            cpp_content = f"""// AUTO-GENERATED FILE
#include "../{target['header_include']}"
#include "../../{target['logic_dir']}/{logic_path.name}" // Ensure the compiler sees the logic struct

namespace ideam::core {{

// Explicitly instantiate the factory matrix for {enum_id}
template struct {target['struct_prefix']}<{target['enum_prefix']}::{enum_id}>;

}} // namespace ideam::core
"""
            # Write to disk
            with open(dest_path, 'w', encoding='utf-8') as f:
                f.write(cpp_content)
            
            print(f"Generated: {new_filename} -> {target['enum_prefix']}::{enum_id}")
            generated_count += 1

        print(f"{target['name']} Generation Complete. Created: {generated_count} | Skipped: {skipped_count}")
        total_generated += generated_count
        total_skipped += skipped_count

    print("\n" + "=" * 45)
    print(f"GRAND TOTAL - Generated: {total_generated} | Skipped: {total_skipped}")

if __name__ == "__main__":
    main()