#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=[".", "-I/usr/include/libusb-1.0"])
if env["platform"] == "windows":
	env.Append(LIBPATH=["lib/win/"])

env.Append(LIBS=["usb-1.0"])
sources = Glob("*.cpp")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "bin/libmj9k.{}.{}.framework/libmj9k.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "bin/libmj9k{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
