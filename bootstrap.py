import argparse
import os
import sys
import subprocess
import copy
from dataclasses import dataclass, field
from collections.abc import Callable

ROOTPATH = os.path.dirname(os.path.abspath(__file__))

@dataclass
class Config:
    cxx: str
    ar: str
    clang_scan_deps: str
    cxxflags: list[str]
    ldflags: list[str]
    build_path: str
    src_path: str


@dataclass
class Target:
    inputs: list[str]
    output: str
    build: Callable[[], None]


@dataclass
class BuildState:
    is_built: dict[str, bool]
    deps: dict[str, set[str]]
    rev_deps: dict[str, set[str]]
    targets: dict[str, Target]
    ready_queue: list[str]

def build(cfg: Config) -> None:
    pass

def parse_args() -> Config:
    parser = argparse.ArgumentParser(
        prog="configure.py",
        description="Generate build.ninja files",
    )

    parser.add_argument(
        "--build-dir",
        required=True,
        help="Relative path to build directory to store the build.ninja file. Relative to 'configure.py' location.",
    )
    parser.add_argument(
        "--cxx",
        default="clang++",
        help="C++ compiler name",
        metavar="EXE",
    )
    parser.add_argument(
        "--ar",
        default="llvm-ar",
        help="AR executable to use for creating static libraries.",
        metavar="EXE",
    )
    parser.add_argument(
        "--objdump",
        default="llvm-objdump",
        help="objdump executable to use for creating assembly from executable by disassembling.",
    )
    parser.add_argument(
        "--march",
        default="native",
        help="target CPU micro-architecture to be passed into the compiler",
    )
    parser.add_argument(
        "--thin-lto",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="enable clang thin-lto",
    )
    parser.add_argument(
        "--opt-level",
        default="0",
        help="Optimization level, can bea anything the c++ compiler supports.  0, 1, 2, 3, g, fast etc.",
    )
    parser.add_argument(
        "--dev-mode",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="enable development mode",
    )
    parser.add_argument(
        "--debug-info",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="emit debug information",
    )
    parser.add_argument(
        "--fuzz",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="enable fuzz instrumentation and fuzz targets. also disables test/bench/exe targets.",
    )
    parser.add_argument(
        "--cxx-time-trace",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="enable compilation profiling output from clang using -ftime-trace flag.",
    )

    args = parser.parse_args()

    assert not os.path.isabs(args.build_dir), "build-dir can't be an absolute path"

    cfg = Config(
        cxx=args.cxx,
        ar=args.ar,
        clang_scan_deps=args.clang_scan_deps,
        cxxflags=cxxflags,
        ldflags=ldflags,
        build_path=build_path,
        src_path=src_path,
    )

    return cfg

def main():
    cfg = parse_args()
    build(cfg)

if __name__ == "__main__":
    main()
