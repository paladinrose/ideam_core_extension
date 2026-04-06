#!/usr/bin/env python
import os
import sys

# Inherit the base environment from godot-cpp
env = SConscript("godot-cpp/SConstruct")

# --- DATA OWNERSHIP & IDENTITY ---
project_folder = "godot-project" 
out_filename   = "ideam_core_extension"    # Change this for different tools
out_dir        = "#" + project_folder + "/addons/" + out_filename + "/lib/"

# --- COMPILER & LANGUAGE STANDARDS ---
# Targeting C++26 explicitly.
if env.get("is_msvc", False):
    # MSVC uses /std:c++latest for bleeding edge / C++26.
    # /Zc:__cplusplus is REQUIRED so MSVC correctly sets the __cplusplus macro for C++26 features.
    env.Append(CXXFLAGS=["/std:c++latest", "/Zc:__cplusplus"])
else:
    # GCC 14+ / Clang 18+ support. 
    # Godot-cpp might inject its own -std=c++17 or c++20, so we append this to override.
    env.Append(CXXFLAGS=["-std=c++26"])

# --- INCLUDE PATHS ---
env.Append(CPPPATH=["src/"])

# --- SOURCE DISCOVERY ---
# Using recursive globbing to ensure nested folders are included in the rebuild.
# We merge and remove potential duplicates to keep the build tree clean.
raw_sources = env.Glob("src/*.cpp") + env.Glob("src/**/*.cpp")
# Ensure uniqueness just in case 'src/**/*.cpp' caught the root 'src/*.cpp' files too
sources = list(set(raw_sources))

# --- BUILD LOGIC ---
if env["platform"] == "macos":
    # Using format for the nested framework structure
    lib_path = "{}{}.{}.{}.framework/{}.{}.{}".format(
        out_dir, out_filename, env["platform"], env["target"], 
        out_filename, env["platform"], env["target"]
    )
    library = env.SharedLibrary(lib_path, source=sources)

elif env["platform"] == "ios":
    suffix = ".simulator.a" if env["ios_simulator"] else ".a"
    lib_name = "{}.{}.{}{}".format(out_filename, env["platform"], env["target"], suffix)
    library = env.StaticLibrary(out_dir + lib_name, source=sources)

else:
    # Windows / Linux logic
    # suffix: e.g., .windows.template_debug.x86_64
    # SHLIBSUFFIX: .dll or .so
    library = env.SharedLibrary(
        "{}{}{}{}".format(out_dir, out_filename, env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)