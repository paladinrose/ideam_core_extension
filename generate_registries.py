import os
from pathlib import Path
import re


def get_enum_name(base_name: str) -> str:
  """Converts a snake_case filename to the PascalCase LogicID enum."""
  if base_name == "aabb":
    return "AABB"
  # e.g., 'paged_to_tiled_bridge' -> 'PagedToTiledBridge'
  return "".join(word.capitalize() for word in base_name.split("_"))


def increment_manifest_version(manifest_path: Path) -> int | None:
  """Increments CURRENT_MANIFEST_VERSION in task_manifest.h by 1."""
  if not manifest_path.exists():
    print(f"\n[ERROR] Manifest header not found at: {manifest_path}")
    return None

  content = manifest_path.read_text(encoding="utf-8")

  # Matches CURRENT_MANIFEST_VERSION (or CURRENT_VERSION) declaration and captures the integer
  pattern = re.compile(
      r"(static\s+constexpr\s+int\s+CURRENT(?:_MANIFEST)?_VERSION\s*=\s*)(\d+)(;)"
  )
  match = pattern.search(content)

  if not match:
    print(
        f"\n[ERROR] Could not find CURRENT_MANIFEST_VERSION pattern in"
        f" {manifest_path.name}."
    )
    return None

  old_version = int(match.group(2))
  new_version = old_version + 1

  updated_content = pattern.sub(rf"\g<1>{new_version}\g<3>", content, count=1)
  manifest_path.write_text(updated_content, encoding="utf-8")

  print(
      f"[MANIFEST UPDATED] {manifest_path.name}: Version {old_version} ->"
      f" {new_version}"
  )
  return new_version


def main():
  base_dir = Path("src/core/tasks")
  manifest_path = base_dir / "registration" / "task_manifest.h"

  # The Configuration Dictionary for our 3 DOD axes
  targets = [
      {
          "name": "Query",
          "logic_dir": "query_logic",
          "reg_dir": "query_reg",
          "suffix": "_query_logic",
          "header_include": "query_logic_sub_registry.h",
          "enum_prefix": "QueryLogicID",
          "struct_prefix": "QueryLogicSubRegistry",
      },
      {
          "name": "Metadata",
          "logic_dir": "metadata_logic",
          "reg_dir": "metadata_reg",
          "suffix": "_metadata_logic",
          "header_include": "metadata_logic_sub_registry.h",
          "enum_prefix": "MetadataLogicID",
          "struct_prefix": "MetadataLogicSubRegistry",
      },
      {
          "name": "Transform",
          "logic_dir": "transform_logic",
          "reg_dir": "transform_reg",
          "suffix": "_transform_logic",
          "header_include": "transform_logic_sub_registry.h",
          "enum_prefix": "TransformLogicID",
          "struct_prefix": "TransformLogicSubRegistry",
      },
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
      with open(dest_path, "w", encoding="utf-8") as f:
        f.write(cpp_content)

      print(
          f"Generated: {new_filename} -> {target['enum_prefix']}::{enum_id}"
      )
      generated_count += 1

    print(
        f"{target['name']} Generation Complete. Created: {generated_count} |"
        f" Skipped: {skipped_count}"
    )
    total_generated += generated_count
    total_skipped += skipped_count

  print("\n" + "=" * 45)
  print(
      f"GRAND TOTAL - Generated: {total_generated} | Skipped: {total_skipped}"
  )

  # Increment CURRENT_MANIFEST_VERSION only when new files were actually generated
  if total_generated > 0:
    increment_manifest_version(manifest_path)
  else:
    print(
        "\nNo new registration files generated. CURRENT_MANIFEST_VERSION"
        " retained."
    )


if __name__ == "__main__":
  main()