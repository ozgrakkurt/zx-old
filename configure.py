#!/usr/bin/env python3

"""This script generates the Ninja build files/structure for the project.
- Put this script and ninja_syntax.py and definition.py at the project root together.
- Define the project structure in proj.py in the project root like it is done in this project.
- Run ./configure.py with desired flags to generate ninja build directories per profile.
- Can use ./configure.py --help to see the available flags.
- As an example, run 'ninja -C build/<profile_name> test' to run all tests with desired profile.
"""

import argparse
import os
import sys
import copy
from dataclasses import dataclass, field
from typing import Optional, Dict, Set, List

import ninja_syntax
import proj
import definition
import platform
from definition import Executable, Library, Project, Profile


ROOTPATH = os.path.dirname(os.path.abspath(__file__))

BASE_LIB_NAME = "zx"


@dataclass
class Config:
    original_cmd: str
    cxx: str
    ar: str
    objdump: str
    march: str
    thin_lto: bool


@dataclass
class Artifacts:
    test_stamps: List[str] = field(default_factory=list)
    lib_files: List[str] = field(default_factory=list)
    test_exes: List[str] = field(default_factory=list)
    bench_exes: List[str] = field(default_factory=list)
    fuzz_exes: List[str] = field(default_factory=list)
    executables: List[str] = field(default_factory=list)
    asm_outputs: List[str] = field(default_factory=list)


def validate_lib_sub_pattern(lib_name: str, sub_name: str, postfix: str):
    assert sub_name.endswith(postfix), f"expected {sub_name} to have postfix {postfix}"

    prefix = lib_name + "_"

    assert sub_name.startswith(prefix), f"expected {sub_name} to have prefix {prefix}"


def validate_lib_sub_implicit_deps(lib_name: str, sub_name: str, deps: Set[str]):
    assert lib_name not in deps, (
        f"sub target {sub_name} of library {lib_name} contains the library as a dependency. This is done by default and is not supposed to be defined in config."
    )

    assert BASE_LIB_NAME not in deps, (
        f"sub target {sub_name} of library {lib_name} can't explicitly depend on {BASE_LIB_NAME}. It is implicitly included."
    )


def validate_library(libs: Dict[str, Library], lib_name: str, lib: Library) -> None:
    assert len(lib.source_files) > 0, "a library has to have at least one source file"

    for dep_name in lib.dependencies:
        assert dep_name != lib_name, f"library {lib_name} depends on itself"
        assert dep_name in libs, (
            f"dep {dep_name} of library {lib_name} doesn't exist in library list"
        )
        assert dep_name != BASE_LIB_NAME, (
            f"library {lib_name} explicitly depends on {BASE_LIB_NAME}, this in not allowed as it is implicit."
        )

    for src_name in lib.source_files:
        assert src_name.startswith(lib_name), (
            f"expected source {src_name} of library {lib_name} to start with {lib_name}."
        )

    for fuzz_tgt_name, deps in lib.fuzz_targets.items():
        validate_lib_sub_pattern(lib_name, fuzz_tgt_name, "_fuzz")
        validate_lib_sub_implicit_deps(lib_name, fuzz_tgt_name, deps)

    for test_tgt_name, deps in lib.test_targets.items():
        validate_lib_sub_pattern(lib_name, test_tgt_name, "_test")
        validate_lib_sub_implicit_deps(lib_name, test_tgt_name, deps)

    for bench_tgt_name, deps in lib.bench_targets.items():
        validate_lib_sub_pattern(lib_name, bench_tgt_name, "_bench")
        validate_lib_sub_implicit_deps(lib_name, bench_tgt_name, deps)


def validate_exe(libs: Dict[str, Library], exe_name: str, exe: Executable) -> None:
    assert len(exe.source_files) > 0, (
        "an executable has to have at least one source file"
    )

    for dep_name in exe.dependencies:
        assert dep_name in libs, (
            f"dep {dep_name} of exe {exe_name} doesn't exist in library list"
        )
        assert dep_name != BASE_LIB_NAME, (
            f"exe {exe_name} explicitly depends on {BASE_LIB_NAME}, this in not allowed as it is implicit."
        )


def validate_project(project: Project) -> None:
    assert os.path.isabs(project.src_path), (
        "project.src_path has to be an absolute path"
    )

    assert os.path.isabs(project.include_path), (
        "project.include_path has to be an absolute path"
    )

    for lib_name, lib in project.libraries.items():
        validate_library(project.libraries, lib_name, lib)
        assert lib_name not in project.executables, (
            f"{lib_name} used as library name and executable name simultaneously"
        )

    for exe_name, exe in project.executables.items():
        validate_exe(project.libraries, exe_name, exe)

    assert BASE_LIB_NAME in project.libraries, (
        f"{BASE_LIB_NAME} library has to be defined"
    )


def validate_profile(profile: Profile) -> None:
    assert profile.opt_level >= 0 and profile.opt_level <= 3, (
        "profile optimization level has to be 0, 1, 2 or 3"
    )


def validate_config(config: Config) -> None:
    pass


def make_cflags(profile: Profile, config: Config) -> List[str]:
    cflags = [
        # Warnings.
        "-Wall",
        "-Wextra",
        "-Wconversion",
        "-Wsign-conversion",
        "-Wmissing-field-initializers",
        "-Wshadow",
        # warn on variable length array usage
        "-Wvla",
        # these don't seem to catch much but enable anyways
        "-Wpointer-bool-conversion",
        "-Wbool-operation",
        # make sure switch on enum has a default
        "-Wswitch-default",
        # Treat warnings as errors.
        "-Werror",
        # Target arch, preferably 'native'.
        f"-march={config.march}",
        # Use cpp20 standard.
        "--std=c++20",
        # we don't use libc but this might effect some compiler builtins?
        "-fno-math-errno",
        # Use pipes instead of temporary files in compilation.
        # ninja docs or ninja repo configure.py say this is better.
        "-pipe",
        # optimization level
        f"-O{profile.opt_level}",
        # include dir
        "-I$include_path",
        # disable frame pointer usage by compiler
        "-fomit-frame-pointer",
        # this caused compilation errors so is disabled. could be an idea to re-enable it
        "-fno-stack-protector",
        # disable libc
        "-nostdlib",
        # disable default includes
        "-nostdinc",
        # specify nolibc target
        # this doesn't have any effect as far as I can tell on a x86_64-linux-gnu PC, but do it anyway.
        "--target=x86_64-linux-none",
        # enable linker garbage collection
        "-ffunction-sections",
        "-fdata-sections",
        # disable strict aliasing
        "-fno-strict-aliasing",
        # disable undefined behavior on signed integer overflow
        "-fno-strict-overflow",
        # disable some c++ features
        "-fno-exceptions",
        "-fno-rtti",
        "-nostdlib++",
        "-fno-threadsafe-statics",
    ]

    if profile.enable_debug_info:
        cflags.append("-g")

    if profile.enable_debug_assert:
        cflags.append("-DZX_DBG_ASSERT_ENABLED=1")

    if profile.enable_fuzzing:
        cflags.append("-DZX_FUZZING_ENABLED=1")

    if config.thin_lto:
        cflags.append("-flto=thin")

    return cflags


def make_ldflags(build_path: str, config: Config) -> List[str]:
    ldflags = [
        # use llvm linker
        "-fuse-ld=lld",
        # don't allow undefined references
        "-Wl,-z,defs",
        # remove unused sections
        "-Wl,--gc-sections",
        # static link
        "-static",
        # disable libc
        "-nostdlib",
        "-nostdinc",
        # nolibc target
        "--target=x86_64-linux-none",
        # disable some c++ features
        "-fno-exceptions",
        "-fno-rtti",
        "-nostdlib++",
        "-fno-threadsafe-statics",
    ]

    if not profile.enable_debug_info:
        # tell linker to strip debug info
        ldflags.append("-Wl,-s")

    if config.thin_lto:
        ldflags.append("-flto=thin")

        # Enable incremental ThinLTO by giving a cache path.
        ldflags.append(f"-Wl,--thinlto-cache-dir=$build_path/thinlto-cache")

    return ldflags


def c_file_to_obj_file(filename: str) -> str:
    assert filename.endswith(".cc")
    return filename.removesuffix(".cc") + ".o"


def collect_deps_impl(
    libraries: Dict[str, Library], deps: Set[str], collect: Set[str]
) -> None:
    for dep_name in deps:
        collect_deps_impl(libraries, libraries[dep_name].dependencies, collect)
        collect.add(dep_name)


def collect_deps(libraries: Dict[str, Library], deps: Set[str]) -> Set[str]:
    # always implicitly depend on base library
    collect: Set[str] = set([BASE_LIB_NAME])

    collect_deps_impl(libraries, deps, collect)

    return collect


def process_sub_target(
    nw: ninja_syntax.Writer,
    libraries: Dict[str, Library],
    lib_name: str,
    tgt_name: str,
    deps: Set[str],
) -> str:
    link_inputs = []

    obj = os.path.join("$build_path", "obj", tgt_name, tgt_name + ".o")
    src = os.path.join("$src_path", lib_name, tgt_name + ".cc")
    nw.build(obj, "cxx", src)
    link_inputs.append(obj)

    exe_path = os.path.join("$build_path", lib_name, tgt_name)

    deps = collect_deps(libraries, deps)

    for lib_name in deps:
        lib_file_path = os.path.join("$build_path", f"lib{lib_name}.a")
        link_inputs.append(lib_file_path)

    nw.build(
        exe_path,
        "link",
        link_inputs,
    )

    nw.newline()

    return exe_path


def process_exe(
    artifacts: Artifacts,
    nw: ninja_syntax.Writer,
    libraries: Dict[str, Library],
    exe_name: str,
    exe: Executable,
) -> None:
    nw.comment(f"executable #{exe_name}")

    link_inputs = []

    for source_file in exe.source_files:
        obj = os.path.join(
            "$build_path", "obj", exe_name, c_file_to_obj_file(source_file)
        )
        src = os.path.join("$src_path", exe_name, source_file)
        nw.build(obj, "cxx", src)
        link_inputs.append(obj)

    exe_path = os.path.join("$build_path", exe_name)

    deps = collect_deps(libraries, exe.dependencies)

    for lib_name in deps:
        lib_file_path = os.path.join("$build_path", f"lib{lib_name}.a")
        link_inputs.append(lib_file_path)

    nw.build(
        exe_path,
        "link",
        link_inputs,
    )

    artifacts.executables.append(exe_path)
    nw.comment("execution")
    nw.build(exe_name, "runexe", exe_path)

    nw.newline()


def process_library(
    artifacts: Artifacts,
    nw: ninja_syntax.Writer,
    libraries: Dict[str, Library],
    lib_name: str,
    lib: Library,
) -> None:
    nw.comment(f"library #{lib_name}")

    obj_files = []

    for source_file in lib.source_files:
        obj = os.path.join(
            "$build_path", "obj", lib_name, c_file_to_obj_file(source_file)
        )
        src = os.path.join("$src_path", lib_name, source_file)
        nw.build(obj, "cxx", src)
        obj_files.append(obj)

    lib_file_path = os.path.join("$build_path", f"lib{lib_name}.a")

    nw.build(lib_file_path, "ar", obj_files)
    artifacts.lib_files.append(lib_file_path)
    nw.newline()

    if not profile.enable_fuzzing:
        # process test targets for this library
        if len(lib.test_targets) > 0:
            nw.comment(f"test targets for library #{lib_name}")
        for test_tgt_name, test_tgt_deps in lib.test_targets.items():
            deps = copy.deepcopy(test_tgt_deps)
            # implicitly depend on parent library
            deps.add(lib_name)
            exe_path = process_sub_target(nw, libraries, lib_name, test_tgt_name, deps)

            # Add test execution step
            nw.comment("test execution")
            artifacts.test_exes.append(exe_path)
            nw.build(test_tgt_name, "runexe", exe_path)

            nw.newline()

            nw.comment("test stamp")
            stamp_path = os.path.join("$build_path", "stamps", f"{test_tgt_name}.stamp")

            nw.build(stamp_path, "runtest", exe_path)
            artifacts.test_stamps.append(stamp_path)

            nw.newline()

    if not profile.enable_fuzzing:
        # process the benchmark targets for this library
        if len(lib.bench_targets) > 0:
            nw.comment(f"benchmark targets for library #{lib_name}")
        for bench_tgt_name, bench_tgt_deps in lib.bench_targets.items():
            deps = copy.deepcopy(bench_tgt_deps)
            # implicitly depend on parent library
            deps.add(lib_name)
            exe_path = process_sub_target(nw, libraries, lib_name, bench_tgt_name, deps)

            # Add bench execution step
            nw.comment("benchmark execution")
            artifacts.bench_exes.append(exe_path)
            nw.build(bench_tgt_name, "runexe", exe_path)

            nw.newline()

            # generate assembly output
            nw.comment("assembly generation")
            asm_out = os.path.join(
                "$build_path",
                f"{bench_tgt_name}.asm",
            )
            nw.build(asm_out, "objdump", exe_path)
            artifacts.asm_outputs.append(asm_out)
            nw.newline()

    if profile.enable_fuzzing:
        # process fuzz targets for this library
        if len(lib.fuzz_targets) > 0:
            nw.comment(f"fuzz targets for library #{lib_name}")
        for fuzz_tgt_name, fuzz_tgt_deps in lib.fuzz_targets.items():
            deps = copy.deepcopy(fuzz_tgt_deps)
            # implicitly depend on parent library
            deps.add(lib_name)
            exe_path = process_sub_target(nw, libraries, lib_name, fuzz_tgt_name, deps)

            artifacts.fuzz_exes.append(exe_path)

            # fuzz_in_path = os.path.join("$build_path", "fuzz_input", fuzz_tgt_name)
            # fuzz_out_path = os.path.join(
            #     "$build_path", "fuzz_output", fuzz_tgt_name
            # )
            nw.build(
                fuzz_tgt_name,
                "runexe",
                exe_path,
                # variables={
                #     "fuzz_input_path": fuzz_in_path,
                #     "fuzz_output_path": fuzz_out_path,
                # },
            )

            nw.newline()

    nw.newline()


def gen_ninja_build(
    build_path: str, profile: Profile, config: Config, project: Project
) -> None:
    assert os.path.isabs(build_path), "build_path has to be an absolute path"
    validate_profile(profile)
    validate_config(config)
    validate_project(project)

    cflags = make_cflags(profile, config)
    ldflags = make_ldflags(build_path, config)

    # build_path is required to be absolute (checked in validation)
    ninjapath = os.path.join(build_path, "build.ninja")
    nw = ninja_syntax.Writer(open(ninjapath, "w"))

    # all of these path variables are absolute (checked in validation)
    nw.comment("path variables")
    nw.variable("src_path", project.src_path)
    nw.variable("include_path", project.include_path)
    nw.variable("build_path", build_path)

    nw.newline()

    nw.comment("build flags")
    nw.variable("cflags", " ".join(cflags))
    nw.variable("ldflags", " ".join(ldflags))

    nw.newline()

    nw.comment("tools")
    nw.variable("cxx", config.cxx)
    nw.variable("ar", config.ar)
    nw.variable("objdump", config.objdump)

    nw.newline()

    nw.comment("rules")

    nw.rule(
        "cxx",
        command="$cxx -MMD -MT $out -MF $out.d $cflags -c $in -o $out",
        depfile="$out.d",
        deps="gcc",
        description="CXX $out",
    )
    nw.rule(
        "ar",
        command="rm -f $out && $ar crs $out $in",
        description="AR $out",
    )
    nw.rule(
        "link",
        command="$cxx $ldflags -o $out $in",
        description="LINK $out",
    )
    nw.rule(
        "objdump",
        command="$objdump -d -S -M intel $in > $out",
        description="OBJDUMP $in",
    )

    nw.newline()

    nw.comment("re-run config script with same parameters as the initial invocation")
    nw.rule(
        "configure",
        command=config.original_cmd,
        generator=True,
        description="Regenerate build.ninja via configure script",
    )
    nw.build(
        "build.ninja",
        "configure",
        implicit=[
            os.path.abspath(ninja_syntax.__file__),
            os.path.abspath(definition.__file__),
            os.path.abspath(proj.__file__),
            os.path.abspath(__file__),
        ],
    )
    nw.newline()

    nw.comment(
        "Run a test executable. On success, create a stamp file to not re-run unless needed."
    )
    nw.rule(
        "runtest",
        command="$in && touch $out",
        description="RUNTEST $in",
    )
    nw.newline()

    nw.comment("Run an executable. Don't create a stamp file so it is run every time.")
    nw.rule(
        "runexe",
        command="$in",
        description="RUNEXE $in",
        # print to console as the command is running since some commands can be very long running.
        # This is fine since we don't have a "run all executables/fuzz_targets/bench_targets at once"
        #   kind of command like we have for tests.
        pool="console",
    )

    nw.newline()

    artifacts = Artifacts()

    # process libraries
    for lib_name, lib in project.libraries.items():
        process_library(artifacts, nw, project.libraries, lib_name, lib)

    if not profile.enable_fuzzing:
        # process executables
        for exe_name, exe in project.executables.items():
            process_exe(artifacts, nw, project.libraries, exe_name, exe)

        nw.newline()

    if not profile.enable_fuzzing:
        nw.comment("Build all libraries")
        nw.build("build_libs", "phony", artifacts.lib_files)

        nw.comment("Build all tests")
        nw.build("build_tests", "phony", artifacts.test_exes)

        nw.comment("Build all benchmarks")
        nw.build("build_benches", "phony", artifacts.bench_exes)

        nw.comment("Build all benchmark assembly output")
        nw.build("asm", "phony", artifacts.asm_outputs)

        nw.comment("Build all exes")
        nw.build("build_exes", "phony", artifacts.executables)

        nw.comment("Run all tests")
        nw.build("test", "phony", artifacts.test_stamps)

        nw.newline()

        nw.default(
            [
                "build_libs",
                "build_tests",
                "build_benches",
                "build_exes",
            ]
        )
    else:
        nw.comment("Build all fuzzers")
        nw.build("build_fuzzers", "phony", artifacts.fuzz_exes)

        nw.default(
            [
                "build_fuzzers",
            ]
        )

    nw.newline()

    nw.close()


def reconstruct_command(args: argparse.Namespace) -> str:
    flags = {
        "build-dir": args.build_dir,
        "cxx": args.cxx,
        "ar": args.ar,
        "objdump": args.objdump,
        "march": args.march,
        "thin-lto": args.thin_lto,
    }

    out_flags = []
    for fname, flag in flags.items():
        if isinstance(flag, bool):
            if flag:
                out_flags.append(f"--{fname}")
            else:
                out_flags.append(f"--no-{fname}")
        elif flag is not None:
            assert isinstance(flag, str), "flag has to be string at this point"
            out_flags.append(f'--{fname}="{flag}"')

    return os.path.abspath(__file__) + " " + " ".join(out_flags)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="configure.py",
        description="Generate build.ninja files",
    )

    parser.add_argument(
        "--build-dir",
        default="build",
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
        metavar="EXE",
    )
    parser.add_argument(
        "--march",
        default="native",
        help="target CPU micro-architecture to be passed into the compiler",
    )
    parser.add_argument(
        "--thin-lto",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="enable LLVM thin-lto",
    )

    args = parser.parse_args()

    assert not os.path.isabs(args.build_dir), "build-dir can't be an absolute path"

    cfg = Config(
        original_cmd=reconstruct_command(args),
        cxx=args.cxx,
        ar=args.ar,
        objdump=args.objdump,
        march=args.march,
        thin_lto=args.thin_lto,
    )

    for profile_name, profile in proj.PROFILES.items():
        build_path = os.path.join(ROOTPATH, args.build_dir, profile_name)

        if not os.path.exists(build_path):
            os.makedirs(build_path)

        gen_ninja_build(build_path, profile, cfg, proj.PROJECT)
