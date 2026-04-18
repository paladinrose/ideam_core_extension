import os
import re

def modernize_headers(directory):
    # Matches the standard #ifndef NAME \n #define NAME block
    guard_pattern = re.compile(r"#ifndef\s+([A-Za-z0-9_]+)\s*\n#define\s+\1\s*\n")

    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".h") or file.endswith(".hpp"):
                filepath = os.path.join(root, file)
                
                with open(filepath, "r", encoding="utf-8") as f:
                    content = f.read()

                # Skip if it already has pragma once
                if "#pragma once" in content:
                    continue

                # If we find the old guard
                if guard_pattern.search(content):
                    # Replace the top guard with #pragma once
                    content = guard_pattern.sub("#pragma once\n\n", content, count=1)
                    
                    # Strip the very last #endif in the file
                    parts = content.rsplit("#endif", 1)
                    if len(parts) == 2:
                        content = parts[0] + parts[1]

                    with open(filepath, "w", encoding="utf-8") as f:
                        f.write(content)
                        
                    print(f"Upgraded to #pragma once: {filepath}")

# Run the script in the current directory
if __name__ == "__main__":
    print("Initiating DOD-approved header cleanup...")
    modernize_headers(".")
    print("Done!")