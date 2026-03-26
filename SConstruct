#!/usr/bin/env python
import os
import sys

# Inherit the base environment from godot-cpp
env = SConscript("godot-cpp/SConstruct")

# --- DATA OWNERSHIP & IDENTITY ---
project_folder = "godot-project" 
out_filename   = "ideam_core"    # Change this for different tools
out_dir        = "#" + project_folder + "/addons/" + out_filename + "/lib/"

# --- COMPILER & LANGUAGE STANDARDS ---
# Targeting C++26. Note: MSVC uses /std:c++latest, GCC/Clang use -std=c++26 or -std=c++2c
if env.get("is_msvc", False):
    env.Append(CPPFLAGS=["/std:c++latest"])
else:
    # GCC 14+ / Clang 18+ support
    env.Append(CXXFLAGS=["-std=c++26"])

# --- INCLUDE PATHS ---
env.Append(CPPPATH=["src/"])

# --- SOURCE DISCOVERY ---
# Using recursive globbing to ensure nested folders are included in the rebuild
sources = Glob("src/*.cpp")
sources += Glob("src/**/*.cpp")

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