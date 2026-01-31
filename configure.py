#!/usr/bin/env python3

"""Script that generates the build.ninja file"""

from argparse import ArgumentParser
import ninja_gen

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
