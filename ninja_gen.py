"""Python module for generating .ninja files.

Builds on top of ninja_syntax.py to export a declarative API for
generating .ninja files to build a C project.

Creates seperate build folders and build.ninja files for each requested profile.

Profiles can have fuzzing enabled to generate fuzz targets that use hoggfuzz.
"""

from argparse import ArgumentParser
import os
import sys

import ninja_syntax

# define API as dataclasses and implement ninja_gen function that takes a Project instance

rootpath = os.path.dirname(os.path.abspath(__file__))

PROJECT_NAME = "shin"

parser = ArgumentParser(
    prog="configure.py",
    description=f"Generate build.ninja file for {PROJECT_NAME}",
)
parser.add_argument(
    "--honggfuzz_path",
    help="Absolute path to honggfuzz folder which can be created by cloning honggfuzz repo and running a build",
    metavar="PATH",
    required=True,
)
parser.add_argument(
    "--cc",
    default="clang",
    help="C compiler name",
    metavar="EXE",
)
parser.add_argument(
    "--build_dir",
    default="build",
    help="Relative path to build directory to store the build.ninja file. Relative to 'configure.py' location.",
    metavar="PATH",
)
parser.add_argument(
    "--march",
    default="native",
    help="target CPU architecture to be passed into the compiler",
)
parser.add_argument(
    "--ar",
    default="ar",
    help="AR executable to use for creating static libraries.",
    metavar="EXE",
)

args = parser.parse_args()

print(f"Starting configure.py for project {PROJECT_NAME}")

print(f"Running with arguments: {args}")

if os.path.isabs(args.build_dir):
    sys.exit(f"Fatal Error: --build_dir can't be an absolute path. It needs to be a relative to 'configure.py' location.")

hfuzz_path = args.honggfuzz_path

if not os.path.isabs(hfuzz_path):
    sys.exit(f"Fatal Error: --honggfuzz_path has to be an absolute path.")

buildpath = os.path.join(rootpath, args.build_dir)

print(f"buildpath={buildpath}")

if os.path.isdir(buildpath):
    sys.exit(f"Fatal Error: Specified --build_dir ({buildpath}) already exists.")

BUILD_FILENAME = "build.ninja"

os.mkdir(buildpath)

ninjapath = os.path.join(buildpath, BUILD_FILENAME)

cflags = [
    # Warnings.
    "-Wall",
    "-Wextra",
    "-Wconversion",
    "-Wsign-conversion",
    # Treat warnings as errors.
    "-Werror",
    # Target arch, preferably 'native'.
    f"-march={args.march}",
    # Use c23 standard.
    "-std=c23",
    # Required for correctness.
    # Use 'restrict'/'register' in performance-critical code.
    "-fno-strict-aliasing",
    # Disable libc.
    "-nostdlib",
    "-fno-builtin",
    # Enable thin LTO.
    "-flto=thin",
    # Use pipes instead of temporary files in compilation.
    "-pipe",
]

def profile_lto_cache_path(profile_name):
    return os.path.join(buildpath, profile_name, "ltocache")

def profile_build_path

ldflags = [
    # Disable libc.
    "-nostdlib",
    # Static linking.
    "-static",
    # Enable thin LTO.
    "-flto=thin",
    # Enable incremental ThinLTO by giving a cache path.
    f"-Wl,--thinlto-cache-dir={lto_cache_path}",
    # Search for libraries in the build folder.
    "-L$builddir",
]

# Disable debugging and apply optimization.
# Anything above level 2 optimization is considered not-worth.
release_flags =  ["-O2", "-DNDEBUG"]
# Disable optimization and keep debug symbols.
debug_flags = ["-O0", "-g"]
# Keep debug symbols but apply optimization.
fuzz_flags = ["-O2", "-g"]

nw = ninja_syntax.Writer(open(ninjapath, 'w'))

nw.variable("buildpath", buildpath)

nw.newline()

nw.variable("cc", args.cc)
nw.variable("ar", args.ar)

nw.newline()

hfuzz_cc = os.path.join(hfuzz_path, "hfuzz_cc", "hfuzz-cc")
hfuzz = os.path.join(hfuzz_path, "honggfuzz")
nw.variable("hfuzz_cc", hfuzz_cc)
nw.variable("hfuzz", hfuzz)

nw.newline()

nw.variable("ldflags", " ".join(ldflags))

nw.newline()

nw.variable("release_flags", " ".join(release_flags + cflags))
nw.variable("debug_flags", " ".join(debug_flags + cflags))
nw.variable("fuzz_flags", " ".join(fuzz_flags + cflags))

nw.newline()

nw.rule(
    f"ar",
    command="rm -f $out && $ar crs $out $in",
    description=f"AR $out",
)

nw.newline()

nw.rule(
    f"link",
    command="$cc $ldflags -o $out $in $libs",
    description=f"LINK $out",
)
nw.rule(
    f"fuzz_link",
    command="$hfuzz_cc $ldflags -o $out $in $libs",
    description=f"FUZZ_LINK $out",
)

nw.newline()


