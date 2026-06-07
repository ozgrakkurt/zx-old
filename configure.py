import argparse
import os
import sys
import subprocess
import copy
from dataclasses import dataclass, field
from typing import Optional, Dict, Set, List

import ninja_syntax
import proj
import definition
import platform
from definition import Executable, Library, Project


ROOTPATH = os.path.dirname(os.path.abspath(__file__))

BASE_LIB_NAME = "zx"


@dataclass
class Config:
    original_cmd: str
    cxx: str
    ar: str
    objdump: str
    llvm_config: str
    link_llvm: bool
    link_libc: bool
    static_link: bool
    march: str
    thin_lto: bool
    opt_level: int
    dev_mode: bool
    debug_info: bool
    fuzz: bool


@dataclass
class Artifacts:
    test_stamps: List[str] = field(default_factory=list)
    fuzz_stamps: List[str] = field(default_factory=list)
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

    for fuzz_tgt_name in lib.fuzz_targets:
        validate_lib_sub_pattern(lib_name, fuzz_tgt_name, "_fuzz")

    for test_tgt_name in lib.test_targets:
        validate_lib_sub_pattern(lib_name, test_tgt_name, "_test")

    for bench_tgt_name in lib.bench_targets:
        validate_lib_sub_pattern(lib_name, bench_tgt_name, "_bench")


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


def validate_config(config: Config) -> None:
    pass


def run_llvm_config(args: List[str]) -> str:
    result = subprocess.run(
        ["llvm-config"] + args, capture_output=True, text=True, check=True
    )
    return result.stdout.strip()


def make_cflags(config: Config) -> List[str]:
    cflags = [
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
        f"-march={config.march}",
        # Use cpp standard.
        "--std=c++20",
        # we don't use libc but this might effect some compiler builtins?
        "-fno-math-errno",
        # Use pipes instead of temporary files in compilation.
        # ninja docs or ninja repo configure.py say this is better.
        "-pipe",
        # optimization level
        f"-O{config.opt_level}",
        # include dir
        "-I$include_path",
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
        # Don't delete `ptr == nullptr` kind of checks. And don't interpret passing nullptr on some places as
        # undefined behavior.
        "-fno-delete-null-pointer-checks",
        # disable some c++ features
        "-fno-exceptions",
        "-fno-rtti",
        "-nostdlib++",
        "-fno-threadsafe-statics",
        "-fomit-frame-pointer",
        "-momit-leaf-frame-pointer",
    ]

    if config.link_llvm:
        cflags.append(run_llvm_config(["-cxxflags"]))
        cflags.append("-DZX_LINK_LLVM")

    if not config.link_libc:
        cflags.append("-nostdlib")
        cflags.append("-nostdinc")
        cflags.append("--target=x86_64-linux-none")
    else:
        cflags.append("-DZX_LINK_LIBC")

    if config.debug_info:
        cflags.append("-g1")
        cflags.append("-gdwarf64")
        cflags.append("-gdwarf-5")
        cflags.append("-funwind-tables")
        cflags.append("-fasynchronous-unwind-tables")
        cflags.append("-DZX_DEBUG_INFO")
    else:
        # Disable unwind tables
        cflags.append("-fno-unwind-tables")
        cflags.append("-fno-asynchronous-unwind-tables")

    if config.dev_mode:
        cflags.append("-DZX_DEV")

    if config.fuzz:
        cflags.append("-DZX_FUZZ=1")
        cflags.append("-fsanitize-coverage=trace-pc-guard")

    if config.thin_lto:
        cflags.append("-flto=thin")

    return cflags


def make_ldflags(build_path: str, config: Config) -> List[str]:
    ldflags = [
        # Target micro architecture.
        f"-march={config.march}",
        # use llvm linker
        "-fuse-ld=lld",
        # don't allow undefined references
        "-Wl,-z,defs",
        # remove unused sections
        "-Wl,--gc-sections",
        # disable some c++ features
        "-fno-exceptions",
        "-fno-rtti",
        "-nostdlib++",
        "-fno-threadsafe-statics",
        # optimization level
        f"-O{config.opt_level}",
        "-fomit-frame-pointer",
        "-momit-leaf-frame-pointer",
    ]

    if config.link_llvm:
        ldflags.append(run_llvm_config(["-ldflags"]))

    if config.static_link:
        ldflags.append("-static")

    if not config.link_libc:
        ldflags.append("-nostdlib")
        ldflags.append("-nostdinc")
        ldflags.append("--target=x86_64-linux-none")

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

    if config.thin_lto:
        ldflags.append("-flto=thin")

        # Enable incremental ThinLTO by giving a cache path.
        ldflags.append(f"-Wl,--thinlto-cache-dir=$build_path/thinlto-cache")

    return ldflags


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
        obj = os.path.join("$build_path", "obj", exe_name, source_file + ".o")
        src = os.path.join("$src_path", exe_name, source_file + ".cc")
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
    config: Config,
    artifacts: Artifacts,
    nw: ninja_syntax.Writer,
    libraries: Dict[str, Library],
    lib_name: str,
    lib: Library,
) -> None:
    nw.comment(f"library #{lib_name}")

    obj_files = []

    for source_file in lib.source_files:
        obj = os.path.join("$build_path", "obj", lib_name, source_file + ".o")
        src = os.path.join("$src_path", lib_name, source_file + ".cc")
        nw.build(obj, "cxx", src)
        obj_files.append(obj)

    lib_file_path = os.path.join("$build_path", f"lib{lib_name}.a")

    nw.build(lib_file_path, "ar", obj_files)
    artifacts.lib_files.append(lib_file_path)
    nw.newline()

    if not config.fuzz:
        # process test targets for this library
        if len(lib.test_targets) > 0:
            nw.comment(f"test targets for library #{lib_name}")
        for test_tgt_name in lib.test_targets:
            deps = set([])
            # implicitly depend on parent library
            deps.add(lib_name)
            exe_path = process_sub_target(nw, libraries, lib_name, test_tgt_name, deps)

            # Add test execution step
            nw.comment("test execution")
            artifacts.test_exes.append(exe_path)
            nw.build(test_tgt_name, "runexe", exe_path)

            nw.newline()

            nw.comment("test stamp")
            stamp_path = os.path.join(
                "$build_path", "test_stamps", f"{test_tgt_name}.stamp"
            )

            nw.build(stamp_path, "runtest", exe_path)
            artifacts.test_stamps.append(stamp_path)

            nw.newline()

    if not config.fuzz:
        # process the benchmark targets for this library
        if len(lib.bench_targets) > 0:
            nw.comment(f"benchmark targets for library #{lib_name}")
        for bench_tgt_name in lib.bench_targets:
            deps = set([])
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
            nw.build(bench_tgt_name + "_asm", "phony", asm_out)

            nw.newline()

    if config.fuzz:
        # process fuzz targets for this library
        if len(lib.fuzz_targets) > 0:
            nw.comment(f"fuzz targets for library #{lib_name}")
        for fuzz_tgt_name in lib.fuzz_targets:
            deps = set([])
            # implicitly depend on parent library
            deps.add(lib_name)
            exe_path = process_sub_target(nw, libraries, lib_name, fuzz_tgt_name, deps)

            # add fuzz execution step
            nw.comment("fuzz execution")
            artifacts.fuzz_exes.append(exe_path)
            nw.build(fuzz_tgt_name, "runexe", exe_path)

            nw.newline()

            nw.comment("fuzz stamp")
            stamp_path = os.path.join(
                "$build_path", "fuzz_stamps", f"{fuzz_tgt_name}.stamp"
            )

            nw.build(
                stamp_path,
                "runtest",
                exe_path,
            )
            artifacts.fuzz_stamps.append(stamp_path)

            nw.newline()

    nw.newline()


def gen_ninja_build(build_path: str, config: Config, project: Project) -> None:
    assert os.path.isabs(build_path), "build_path has to be an absolute path"
    validate_config(config)
    validate_project(project)

    cflags = make_cflags(config)
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
        command="$objdump --visualize-jumps --no-show-raw-insn -d -S -C -M intel $in > $out",
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
        # This is fine since we don't have a "run all executables/bench_targets at once"
        pool="console",
    )

    nw.newline()

    artifacts = Artifacts()

    # process libraries
    for lib_name, lib in project.libraries.items():
        process_library(config, artifacts, nw, project.libraries, lib_name, lib)

    if not config.fuzz:
        # process executables
        for exe_name, exe in project.executables.items():
            process_exe(artifacts, nw, project.libraries, exe_name, exe)

        nw.newline()

    if not config.fuzz:
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
        nw.comment("Build all libraries")
        nw.build("build_libs", "phony", artifacts.lib_files)

        nw.comment("Build all fuzzers")
        nw.build("build_fuzzers", "phony", artifacts.fuzz_exes)

        nw.comment("Run all fuzzers")
        nw.build("fuzz", "phony", artifacts.fuzz_stamps)

        nw.default(
            [
                "build_libs",
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
        "llvm-config": args.llvm_config,
        "opt-level": args.opt_level,
        "dev-mode": args.dev_mode,
        "debug-info": args.debug_info,
        "fuzz": args.fuzz,
        "link-llvm": args.link_llvm,
        "link-libc": args.link_libc,
        "static-link": args.static_link,
    }

    out_flags = []
    for fname, flag in flags.items():
        if isinstance(flag, bool):
            if flag:
                out_flags.append(f"--{fname}")
            else:
                out_flags.append(f"--no-{fname}")
        elif isinstance(flag, int):
            out_flags.append(f"--{fname}={flag}")
        elif flag is not None:
            assert isinstance(flag, str), "flag has to be string at this point"
            out_flags.append(f'--{fname}="{flag}"')

    return "python " + os.path.abspath(__file__) + " " + " ".join(out_flags)


if __name__ == "__main__":
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
        "--llvm-config",
        default="llvm-config",
        help="llvm-config executable to use for linking with LLVM.",
        metavar="EXE",
    )
    parser.add_argument(
        "--objdump",
        default="objdump",
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
        default=True,
        help="enable LLVM thin-lto",
    )
    parser.add_argument(
        "--opt-level",
        default="g",
        help="Optimization level, can be 0, 1, 2, 3, g, fast etc.",
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
        "--link-libc",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="Link with libc",
    )
    parser.add_argument(
        "--link-llvm",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="Link with llvm",
    )
    parser.add_argument(
        "--static-link",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Use static linking",
    )

    args = parser.parse_args()

    assert not os.path.isabs(args.build_dir), "build-dir can't be an absolute path"

    cfg = Config(
        original_cmd=reconstruct_command(args),
        cxx=args.cxx,
        ar=args.ar,
        objdump=args.objdump,
        llvm_config=args.llvm_config,
        march=args.march,
        thin_lto=args.thin_lto,
        opt_level=args.opt_level,
        dev_mode=args.dev_mode,
        debug_info=args.debug_info,
        fuzz=args.fuzz,
        link_libc=args.link_libc,
        link_llvm=args.link_llvm,
        static_link=args.static_link,
    )

    build_path = os.path.join(ROOTPATH, args.build_dir)

    if not os.path.exists(build_path):
        os.makedirs(build_path)

    gen_ninja_build(build_path, cfg, proj.PROJECT)
