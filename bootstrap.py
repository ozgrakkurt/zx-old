import argparse
import os
import sys
import subprocess
import copy
import json
from dataclasses import dataclass, field
from collections.abc import Callable

ROOTPATH = os.path.dirname(os.path.abspath(__file__))

def read_file(path: str) -> str:
    with open(path, "r") as file:
        return file.read()

def make_path_absolute(path: str) -> str:
    if os.path.isabs(path):
        return path
    else:
        return os.path.join(ROOTPATH, path)


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

@dataclass
class DepsJson:
    deps: set[str]
    test_deps: set[str]
    bin_deps: set[str]

def read_deps_json(path: str) -> DepsJson:
    data = json.loads(read_file(path))

    deps = data["deps"]
    test_deps = data["test_deps"]
    bin_deps = data["bin_deps"]

    assert isinstance(deps, list)
    assert isinstance(test_deps, list)
    assert isinstance(bin_deps, list)

    out = DepsJson(
        deps=set([]),
        test_deps=set([]),
        bin_deps=set([]),
    )

    for d in deps:
        assert d not in out.deps
        out.deps.insert(d)

    for d in test_deps:
        assert d not in out.test_deps
        out.test_deps.insert(d)

    for d in bin_deps:
        assert d not in out.bin_deps
        out.bin_deps.insert(d)

    return out
    

def build(cfg: Config) -> None:
    pass

def parse_file_name(filename: str, expected_extension: str) -> str:
    parts = filename.split(".")

    assert len(parts) == 2

    name = parts[0]
    ext = parts[1]

    assert ":" not in name
    assert "-" not in name
    assert len(name) > 0

    assert ext == expected_extension

    return name


def scan_entry(cfg: Config, path: str) -> list[Target]:
    deps = read_deps_json(os.path.join(path, "deps.json"))

    lib_deps = deps.deps
    test_deps = deps.deps.union(deps.test_deps)
    bin_deps = deps.deps.union(deps.bin_deps)

    lib_path = os.path.join(path, "lib")
    bin_path = os.path.join(path, "bin")
    test_path = os.path.join(path, "test")

    lib_dir = os.listdir(lib_path)
    for filename in lib_dir:
        file_path = os.path.join(lib_path, filename)


    targets = []

    return targets

def scan(cfg: Config) -> BuildState:
    state = BuildState(
        is_built={},
        deps={},
        rev_deps={},
        targets={},
        ready_queue=[],
    )

    src_dir = os.listdir(cfg.src_path)
    for entry in src_dir:
        targets = scan_entry(cfg, state, os.path.join(cfg.src_path, entry))


    return state


def build(state: BuildState) -> None:
    pass

@dataclass
class FlagConfig:
    dev_mode: bool
    fuzz: bool
    debug_info: bool

def make_cxxflags(config: FlagConfig) -> List[str]:
    cxxflags = [
        # Warnings.
        "-Wall",
        "-Wextra",
        "-Wconversion",
        "-Wsign-conversion",
        "-Wmissing-field-initializers",
        "-Wshadow",
        "-Wimplicit-fallthrough",
        # We only use C style structs so this is not an issue. This can cause bugs
        # if we ever use types with constructors?
        "-Wno-reorder-init-list",
        # This causes annoying warnings with templated code.
        # Could try to enable it and then check if there are any interesting catches later.
        "-Wno-tautological-compare",
        # warn on variable length array usage
        "-Wvla",
        # these don't seem to catch much but enable anyways
        "-Wpointer-bool-conversion",
        "-Wbool-operation",
        # make sure switch on enum has a default
        "-Wswitch-default",
        # Treat warnings as errors.
        "-Werror",
        # Target micro architecture.
        "-march=native",
        # Use cpp standard.
        "--std=c++23",
        "-fno-math-errno",
        # this caused compilation errors so is disabled. could be an idea to re-enable it
        "-fno-stack-protector",
        # enable linker garbage collection
        "-ffunction-sections",
        "-fdata-sections",
        # disable strict aliasing
        "-fno-strict-aliasing",
        # disable undefined behavior on signed integer overflow
        "-fno-strict-overflow",
        # signed integer overflow is defined to wrap.
        "-fwrapv",
        # Don't delete `ptr != nullptr` kind of checks because "dereferencing nullptr is undefined behavior BULLSHI"
        "-fno-delete-null-pointer-checks",
        # disable some c++ features
        "-fno-exceptions",
        "-fno-rtti",
        "-nostdlib++",
        "-fno-threadsafe-statics",
        "-fomit-frame-pointer",
        "-momit-leaf-frame-pointer",
        # disable libc
        "-nostdlib",
        "-nostdinc",
        "--target=x86_64-linux-none",
        "-fno-builtin",
    ]

    cxxflags.append(f"-O{config.opt_level}")

    if config.debug_info:
        cxxflags.append("-g1")
        cxxflags.append("-gdwarf64")
        cxxflags.append("-gdwarf-5")
        cxxflags.append("-funwind-tables")
        cxxflags.append("-fasynchronous-unwind-tables")
        cxxflags.append("-DZX_DEBUG_INFO")
    else:
        # Disable unwind tables
        cxxflags.append("-fno-unwind-tables")
        cxxflags.append("-fno-asynchronous-unwind-tables")

    if config.dev_mode:
        cxxflags.append("-DZX_DEV")

    if config.fuzz:
        cxxflags.append("-DZX_FUZZ=1")
        cxxflags.append("-fsanitize-coverage=trace-pc-guard")

    return cxxflags


def make_ldflags(config: FlagConfig) -> List[str]:
    ldflags = [
        # use llvm linker
        "-fuse-ld=lld",
        # don't allow undefined references
        "-Wl,-z,defs",
        # remove unused sections
        "-Wl,--gc-sections",
        "-nostdlib++",
        "-fomit-frame-pointer",
        "-momit-leaf-frame-pointer",
        "-static",
        # disable libc
        "-nostdlib",
        "-nostdinc",
        "--target=x86_64-linux-none",
    ]

    ldflags.append(f"-O{config.opt_level}")

    if config.debug_info:
        ldflags.append("-g1")
        ldflags.append("-gdwarf64")
        ldflags.append("-gdwarf-5")
        ldflags.append("-funwind-tables")
        ldflags.append("-fasynchronous-unwind-tables")
    else:
        # Disable unwind tables
        ldflags.append("-fno-unwind-tables")
        ldflags.append("-fno-asynchronous-unwind-tables")
        # tell linker to strip debug info
        ldflags.append("-Wl,-s")

    return ldflags

def parse_args() -> Config:
    parser = argparse.ArgumentParser(
        prog="bootstrap.py",
        description="bootstrap the codebase by building and testing everything",
    )

    parser.add_argument(
        "--build-dir",
        default="bootstrap"
        help="Relative path to build directory to store build output. Relative to 'configure.py' location.",
    )
    parser.add_argument(
        "--src-dir",
        default="bootstrap"
        help="Relative path to source code directory. Relative to 'configure.py' location.",
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
        "--opt-level",
        default="0",
        help="Optimization level, can be anything that the c++ compiler supports.  0, 1, 2, 3, g, fast etc.",
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

    args = parser.parse_args()

    flags_cfg = FlagConfig(
        fuzz=args.fuzz,
        dev_mode=args.dev_mode,
        debug_info=args.debug_info,
    )

    cxxflags = make_cxxflags(flags_cfg)
    ldflags = make_ldflags(flags_cfg)

    return Config(
        cxx=args.cxx,
        ar=args.ar,
        clang_scan_deps=args.clang_scan_deps,
        build_path=make_path_absolute(build_path),
        src_path=make_path_absolute(src_path),
        cxxflags=cxxflags,
        ldflags=ldflags,
    )

def main():
    cfg = parse_args()
    state = scan(cfg)
    build(state)

if __name__ == "__main__":
    main()
