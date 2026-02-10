#!/usr/bin/env python3

"""This script generates the Ninja build files/structure for the project.
- Put this script and ninja_syntax.py and definition.py at the project root together.
- Define the project structure in proj_cfg.py in the project root like it is done in this project.
- Run ./configure.py with desired flags to generate ninja build directories per profile.
    Can use ./configure.py --help to see the available flags.
- As an example, run 'ninja -C build/<profile_name> test' to run all tests with desired profile.
"""

import argparse
import os
import sys
import copy
from dataclasses import dataclass, field
from typing import Optional, Dict, Set, List

import ninja_syntax
import proj_cfg
import definition
import platform
from definition import Executable, Library, Project, Profile


ROOTPATH = os.path.dirname(os.path.abspath(__file__))


@dataclass
class Config:
    cc: str
    ar: str
    objdump: str
    march: str
    target: Optional[str]
    thin_lto: bool
    no_libc: bool
    static_link: bool
    # Enable fuzz targets and disable tests/executables
    fuzzing: bool
    # Have to be supplied path to hfuzz binary if fuzzing is true
    hfuzz: Optional[str]


def validate_lib_sub_pattern(lib_name: str, sub_name: str, postfix: str):
    assert sub_name.endswith(postfix), f"expected {sub_name} to have postfix {postfix}"

    prefix = lib_name + "_"

    assert sub_name.startswith(prefix), f"expected {sub_name} to have prefix {prefix}"


def validate_lib_sub_implicit_deps(lib_name: str, sub_name: str, deps: Set[str]):
    assert lib_name not in deps, (
        f"sub target {sub_name} of library {lib_name} contains the library as a dependency. This is done by default and is not supposed to be defined in config."
    )

    assert "shin" not in deps, (
        f"sub target {sub_name} of library {lib_name} can't explicitly depend on shin. It is implicitly included."
    )


def validate_library(libs: Dict[str, Library], lib_name: str, lib: Library) -> None:
    assert len(lib.source_files) > 0, "a library has to have at least one source file"

    for dep_name in lib.dependencies:
        assert dep_name != lib_name, f"library {lib_name} depends on itself"
        assert dep_name in libs, (
            f"dep {dep_name} of library {lib_name} doesn't exist in library list"
        )
        assert dep_name != "shin", (
            f"library {lib_name} explicitly depends on shin, this in not allowed as it is implicit."
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
        assert dep_name != "shin", (
            f"exe {exe_name} explicitly depends on shin, this in not allowed as it is implicit."
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

    assert "shin" in project.libraries, "shin library has to be defined"


def validate_profile(profile: Profile) -> None:
    assert profile.opt_level >= 0 and profile.opt_level <= 2, (
        "profile optimization level has to be 0, 1 or 2"
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
        # Treat warnings as errors.
        "-Werror",
        # Target arch, preferably 'native'.
        f"-march={config.march}",
        # Use c23 standard.
        "--std=c23",
        # enable feature_test_macro so linux/gnu functions don't get disabled because of std=c23 flag
        "-D_DEFAULT_SOURCE=1"
        # Required for correctness.
        # Use 'restrict'/'register' in performance-critical code.
        "-fno-strict-aliasing",
        # Use pipes instead of temporary files in compilation.
        "-pipe",
        f"-O{profile.opt_level}",
        "-I$include_path",
    ]

    if profile.enable_debug_info:
        cflags.append("-g")

    if profile.enable_debug_assert:
        cflags.append("-DSHIN_DBG_ASSERT_ENABLED=1")

    if config.no_libc:
        cflags.append("-nostdlib")
        cflags.append("-fno-builtin")

    if config.thin_lto:
        cflags.append("-flto=thin")

    if config.target is not None:
        cflags.append(f"--target={config.target}")

    return cflags


def make_ldflags(build_path: str, config: Config) -> List[str]:
    ldflags = [
        # use llvm linker
        "-fuse-ld=lld",
    ]

    if config.static_link:
        ldflags.append("-static")

    if not profile.enable_debug_info:
        # tell linker to strip debug info
        ldflags.append("-Wl,-s")

    if config.no_libc:
        ldflags.append("-nostdlib")
        ldflags.append("-fno-builtin")

    if config.thin_lto:
        ldflags.append("-flto=thin")

        # Enable incremental ThinLTO by giving a cache path.
        ldflags.append(f"-Wl,--thinlto-cache-dir=$build_path/thinlto-cache")

    if config.target is not None:
        ldflags.append(f"--target={config.target}")

    return ldflags


def c_file_to_obj_file(filename: str) -> str:
    assert filename.endswith(".c")
    return filename.removesuffix(".c") + ".o"


def collect_deps_impl(
    libraries: Dict[str, Library], deps: Set[str], collect: Set[str]
) -> None:
    for dep_name in deps:
        collect_deps_impl(libraries, libraries[dep_name].dependencies, collect)
        collect.add(dep_name)


def collect_deps(libraries: Dict[str, Library], deps: Set[str]) -> Set[str]:
    # always implicitly depend on shin
    collect: Set[str] = set(["shin"])

    collect_deps_impl(libraries, deps, collect)

    return collect


def process_sub_target(
    nw: ninja_syntax.Writer,
    libraries: Dict[str, Library],
    lib_name: str,
    tgt_name: str,
    deps: Set[str],
) -> None:
    link_inputs = []

    obj = os.path.join("$build_path", "obj", tgt_name, tgt_name + ".o")
    src = os.path.join("$src_path", lib_name, tgt_name + ".c")
    nw.build(obj, "cc", src)
    link_inputs.append(obj)

    exe_path = os.path.join("$build_path", tgt_name)

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


def process_exe(
    nw: ninja_syntax.Writer,
    libraries: Dict[str, Library],
    exe_name: str,
    exe: Executable,
) -> None:
    link_inputs = []

    for source_file in exe.source_files:
        obj = os.path.join(
            "$build_path", "obj", exe_name, c_file_to_obj_file(source_file)
        )
        src = os.path.join("$src_path", exe_name, source_file)
        nw.build(obj, "cc", src)
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
    nw.variable("cc", config.cc)
    nw.variable("ar", config.ar)
    nw.variable("objdump", config.objdump)

    if config.fuzzing:
        assert config.hfuzz is not None, (
            "have to supply config.hfuzz if config.fuzz is set to true"
        )
        nw.variable("hfuzz", config.hfuzz)

    nw.newline()

    nw.comment("rules")

    nw.rule(
        "cc",
        command="$cc -MMD -MT $out -MF $out.d $cflags -c $in -o $out",
        depfile="$out.d",
        deps="gcc",
        description="CC $out",
    )
    nw.rule(
        "ar",
        command="rm -f $out && $ar crs $out $in",
        description="AR $out",
    )
    nw.rule(
        "link",
        command="$cc $ldflags -o $out $in",
        description="LINK $out",
    )
    nw.rule(
        "objdump",
        command="$objdump -d -S -M intel $in > $out",
        description="OBJDUMP $in",
    )

    nw.newline()

    nw.comment("regeneration")
    nw.rule(
        "configure",
        command=f"{os.path.abspath(__file__)} {' '.join(sys.argv[1:])}",
        generator=True,
        description="Regenerate build.ninja via configure script",
    )
    nw.build(
        "build.ninja",
        "configure",
        implicit=[
            os.path.abspath(ninja_syntax.__file__),
            os.path.abspath(definition.__file__),
            os.path.abspath(proj_cfg.__file__),
            os.path.abspath(__file__),
        ],
    )
    nw.newline()

    if not config.fuzzing:
        nw.comment(
            "Run a test executable. On success, create a stamp file to not re-run unless needed."
        )
        nw.rule(
            "runtest",
            command="$in && touch $out",
            description="RUNTEST $in",
        )
        nw.newline()

        nw.comment(
            "Run a benchmark executable. Don't create a stamp file so it is run every time."
        )
        nw.rule(
            "runbench",
            command="$in",
            description="RUNBENCH $in",
            # print to console as the command is running since benchmarks can be very long running.
            # This is fine since we don't have a "run all benchmarks at once" kind of command like we have for tests.
            pool="console",
        )

        nw.newline()

    if config.fuzzing:
        nw.comment("Run a fuzz executable using honggfuzz in persistent mode.")
        nw.rule(
            "hfuzz",
            command=(
                "mkdir -p $fuzz_input_path $fuzz_output_path && $hfuzz -Q"
                " -W $fuzz_output_path --input $fuzz_input_path -- $in"
            ),
            description="HONGGFUZZ $in",
            pool="console",
        )
        nw.newline()

    test_stamps = []
    lib_files = []
    test_exes = []
    bench_exes = []
    fuzz_exes = []
    asm_outputs = []
    executables = []

    # process libraries
    for lib_name, lib in project.libraries.items():
        nw.comment(f"library #{lib_name}")

        obj_files = []

        for source_file in lib.source_files:
            obj = os.path.join(
                "$build_path", "obj", lib_name, c_file_to_obj_file(source_file)
            )
            src = os.path.join("$src_path", lib_name, source_file)
            nw.build(obj, "cc", src)
            obj_files.append(obj)

        lib_file_path = os.path.join("$build_path", f"lib{lib_name}.a")

        nw.build(lib_file_path, "ar", obj_files)
        lib_files.append(lib_file_path)
        nw.newline()

        if config.fuzzing:
            # process fuzz targets for this library
            if len(lib.fuzz_targets) > 0:
                nw.comment(f"fuzz targets for library #{lib_name}")
            for fuzz_tgt_name, fuzz_tgt_deps in lib.fuzz_targets.items():
                deps = copy.deepcopy(fuzz_tgt_deps)
                # implicitly depend on parent library
                deps.add(lib_name)
                process_sub_target(nw, project.libraries, lib_name, fuzz_tgt_name, deps)

                exe_path = os.path.join("$build_path", fuzz_tgt_name)
                fuzz_exes.append(exe_path)

                fuzz_in_path = os.path.join("$build_path", "fuzz_input", fuzz_tgt_name)
                fuzz_out_path = os.path.join(
                    "$build_path", "fuzz_output", fuzz_tgt_name
                )
                nw.build(
                    fuzz_tgt_name,
                    "hfuzz",
                    exe_path,
                    variables={
                        "fuzz_input_path": fuzz_in_path,
                        "fuzz_output_path": fuzz_out_path,
                    },
                )

                nw.newline()
        else:
            # process test targets for this library
            if len(lib.test_targets) > 0:
                nw.comment(f"test targets for library #{lib_name}")
            for test_tgt_name, test_tgt_deps in lib.test_targets.items():
                deps = copy.deepcopy(test_tgt_deps)
                # implicitly depend on parent library
                deps.add(lib_name)
                process_sub_target(nw, project.libraries, lib_name, test_tgt_name, deps)

                # Add test execution step
                exe_path = os.path.join("$build_path", test_tgt_name)
                test_exes.append(exe_path)

                stamp_path = os.path.join(
                    "$build_path", "stamps", f"{test_tgt_name}.stamp"
                )

                nw.build(stamp_path, "runtest", exe_path)
                test_stamps.append(stamp_path)

                nw.newline()

            # process the benchmark targets for this library
            if len(lib.bench_targets) > 0:
                nw.comment(f"benchmark targets for library #{lib_name}")
            for bench_tgt_name, bench_tgt_deps in lib.bench_targets.items():
                deps = copy.deepcopy(bench_tgt_deps)
                # implicitly depend on parent library
                deps.add(lib_name)
                process_sub_target(
                    nw, project.libraries, lib_name, bench_tgt_name, deps
                )

                # Add bench execution step
                nw.comment("bencmark execution")
                exe_path = os.path.join("$build_path", bench_tgt_name)
                bench_exes.append(exe_path)
                nw.build(bench_tgt_name, "runbench", exe_path)

                nw.newline()

                # generate assembly output
                nw.comment("assembly generation")
                asm_out = os.path.join(
                    "$build_path",
                    "asm",
                    f"{bench_tgt_name}.asm",
                )
                nw.build(asm_out, "objdump", exe_path)
                asm_outputs.append(asm_out)
                nw.newline()

    if not config.fuzzing:
        # process executables
        for exe_name, exe in project.executables.items():
            nw.comment(f"executable #{exe_name}")
            process_exe(nw, project.libraries, exe_name, exe)
            exe_path = os.path.join("$build_path", exe_name)
            executables.append(exe_path)

        nw.newline()

        nw.comment("Build all libraries")
        nw.build("build_libs", "phony", lib_files)

        nw.comment("Build all tests")
        nw.build("build_tests", "phony", test_exes)

        nw.comment("Build all benchmarks")
        nw.build("build_benches", "phony", bench_exes)

        nw.comment("Build all benchmark assembly output")
        nw.build("asm", "phony", asm_outputs)

        nw.comment("Build all exes")
        nw.build("build_exes", "phony", executables)

        nw.comment("Run all tests")
        nw.build("test", "phony", test_stamps)

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
        nw.build("build_fuzzers", "phony", fuzz_exes)

        nw.newline()

        nw.default(["build_fuzzers"])

    nw.newline()

    nw.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="configure.py",
        description="Generate build.ninja file for shin",
    )

    parser.add_argument(
        "--build_dir",
        default="build",
        help="Relative path to build directory to store the build.ninja file. Relative to 'configure.py' location.",
    )
    parser.add_argument(
        "--honggfuzz_path",
        help="Absolute path to honggfuzz folder, which can be created by cloning honggfuzz repo and running a build. Fuzz setup generated only when this argument is provided.",
        metavar="PATH",
    )
    parser.add_argument(
        "--cc",
        default="clang",
        help="C compiler name",
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
        help="target CPU architecture to be passed into the compiler",
    )
    parser.add_argument(
        "--target",
        help="LLVM target triple to use. We always use static linking so using glibc doesn't make sense in any case.",
    )
    parser.add_argument(
        "--thin-lto",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="enable LLVM thin-lto",
    )
    parser.add_argument(
        "--disable-libc",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="disable linking/using libc and compiler built-ins",
    )
    parser.add_argument(
        "--static-link",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="Toggle static linking",
    )

    args = parser.parse_args()

    assert not os.path.isabs(args.build_dir), "build_dir can't be an absolute path"

    if args.honggfuzz_path is not None:
        assert os.path.isabs(args.honggfuzz_path), (
            "honggfuzz_path has to be an absolute path"
        )

    cfg = Config(
        cc=args.cc,
        ar=args.ar,
        objdump=args.objdump,
        march=args.march,
        target=args.target,
        thin_lto=args.thin_lto,
        no_libc=args.disable_libc,
        static_link=args.static_link,
        fuzzing=False,
        hfuzz=None,
    )

    for profile_name, profile in proj_cfg.PROFILES.items():
        build_path = os.path.join(ROOTPATH, args.build_dir, profile_name)

        if not os.path.exists(build_path):
            os.makedirs(build_path)

        gen_ninja_build(build_path, profile, cfg, proj_cfg.PROJECT)

        if args.honggfuzz_path is not None:
            fuzz_build_path = os.path.join(
                ROOTPATH, args.build_dir, f"{profile_name}_fuzz"
            )

            fuzz_cfg = copy.deepcopy(cfg)
            fuzz_cfg.cc = os.path.join(args.honggfuzz_path, "hfuzz_cc", "hfuzz-cc")
            fuzz_cfg.fuzzing = True
            fuzz_cfg.hfuzz = os.path.join(args.honggfuzz_path, "honggfuzz")

            if not os.path.exists(fuzz_build_path):
                os.makedirs(fuzz_build_path)

            gen_ninja_build(fuzz_build_path, profile, fuzz_cfg, proj_cfg.PROJECT)
