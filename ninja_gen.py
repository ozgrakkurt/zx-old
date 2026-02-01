"""Python module for generating .ninja files.

Builds on top of ninja_syntax.py to export a declarative API for
generating .ninja files to build a C project.
"""

import os
import sys
import copy
from dataclasses import dataclass, field
from typing import Optional, Dict, Set, List

import ninja_syntax


@dataclass
class Executable:
    source_files: Set[str] = field(default_factory=set)
    dependencies: Set[str] = field(default_factory=set)


@dataclass
class Library:
    source_files: Set[str] = field(default_factory=set)
    dependencies: Set[str] = field(default_factory=set)
    test_targets: Dict[str, Executable] = field(default_factory=dict)
    fuzz_targets: Dict[str, Executable] = field(default_factory=dict)


@dataclass
class Project:
    libraries: Dict[str, Library] = field(default_factory=dict)
    executables: Dict[str, Executable] = field(default_factory=dict)
    src_path: str
    include_path: str


@dataclass
class Config:
    cc: str = "clang"
    ar: str = "llvm-ar"
    march: str = "native"
    target: Optional[str] = None
    thin_lto: bool = True
    no_libc: bool = True
    # Enable fuzz targets and disable regular executables
    fuzzing: bool = False


@dataclass
class Profile:
    opt_level: int = 0
    debug_info_enabled: bool = True
    debug_assert_enabled: bool = True


def validate_library(libs: Dict[str, Library], lib_name: str, lib: Library) -> None:
    assert (
        len(lib.source_files) > 0,
        "a library has to have at least one source file",
    )
    for dep_name in lib.dependencies:
        assert (dep_name != lib_name, f"library {lib_name} depends on itself")
        assert (
            dep_name in libs,
            f"dep {dep_name} of library {lib_name} doesn't exist in library list",
        )
    for fuzz_tgt_name, fuzz_tgt in lib.fuzz_targets.items():
        assert (
            lib_name not in fuzz_tgt.dependencies,
            f"fuzz target {fuzz_tgt_name} of library {lib_name} contains the library as a dependency. This is done by default and is not supposed to be defined in config.",
        )

    for test_tgt_name, test_tgt in lib.test_targets.items():
        assert (
            lib_name not in test_tgt.dependencies,
            f"test target {test_tgt_name} of library {lib_name} contains the library as a dependency. This is done by default and is not supposed to be defined in config.",
        )


def validate_exe(libs: Dict[str, Library], exe_name: str, exe: Executable) -> None:
    assert (
        len(exe.source_files) > 0,
        "an executable has to have at least one source file",
    )
    for dep_name in exe.dependencies:
        assert (
            dep_name in libs,
            f"dep {dep_name} of exe {exe_name} doesn't exist in library list",
        )


def validate_project(project: Project) -> None:
    assert (
        os.path.isabs(project.src_path),
        "project.src_path has to be an absolute path",
    )
    assert (
        os.path.isabs(project.include_path),
        "project.include_path has to be an absolute path",
    )
    for lib_name, lib in project.libraries.items():
        validate_library(project.libraries, lib_name, lib)
        assert (
            lib_name not in project.executables,
            f"{lib_name} used as library name and executable name simultaneously",
        )
    for exe_name, exe in project.executables.items():
        validate_exe(project.libraries, exe_name, exe)


def validate_profile(profile: Profile) -> None:
    assert (
        profile.opt_level >= 0 and profile.opt_level <= 2,
        "profile optimization level has to be 0, 1 or 2",
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
        "-std=c23",
        # Required for correctness.
        # Use 'restrict'/'register' in performance-critical code.
        "-fno-strict-aliasing",
        # Use pipes instead of temporary files in compilation.
        "-pipe",
        f"-O{profile.opt_level}",
        "-I$include_path",
    ]

    if profile.debug_info_enabled:
        cflags.append("-g")
    else:
        cflags.append("-s")

    if profile.debug_assert_enabled:
        cflags.append("-DSHIN_DBG_ASSERT=1")

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
        # Static linking.
        "-static",
    ]

    if config.no_libc:
        ldflags.append("-nostdlib")
        ldflags.append("-fno-builtin")

    if config.thin_lto:
        ldflags.append("-flto=thin")

        # Enable incremental ThinLTO by giving a cache path.
        ldflags.append(f"-Wl,--thinlto-cache-dir=$build_path/thinlto-cache")

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


def collect_deps(libraries: Dict[str, Library], exe: Executable) -> Set[str]:
    collect: Set[str] = set()

    collect_deps_impl(libraries, exe.dependencies, collect)

    return collect


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

    deps = collect_deps(libraries, exe)

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
    assert (os.path.isabs(build_path), "build_path has to be an absolute path")
    validate_profile(profile)
    validate_config(config)
    validate_project(project)

    cflags = make_cflags(profile, config)
    ldflags = make_ldflags(build_path, config)

    # build_path is required to be absolute (checked in validation)
    ninjapath = os.path.join(build_path, "build.ninja")
    nw = ninja_syntax.Writer(open(ninjapath, "w"))

    # all of these path variables are absolute (checked in validation)
    nw.variable("src_path", project.src_path)
    nw.variable("include_path", project.include_path)
    nw.variable("build_path", build_path)

    nw.newline()

    nw.variable("cflags", " ".join(cflags))
    nw.variable("ldflags", " ".join(ldflags))

    nw.newline()

    nw.variable("cc", config.cc)
    nw.variable("ar", config.ar)

    nw.newline()

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

    nw.newline()

    test_exes = []

    for lib_name, lib in project.libraries.items():
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

        nw.newline()

        if config.fuzzing:
            for fuzz_tgt_name, fuzz_tgt in lib.fuzz_targets.items():
                exe_name = f"{lib_name}_fuzz_{fuzz_tgt_name}"
                exe = copy.deepcopy(fuzz_tgt)
                exe.dependencies.add(lib_name)
                process_exe(nw, project.libraries, exe_name, exe)
        else:
            for test_tgt_name, test_tgt in lib.test_targets.items():
                exe_name = f"{lib_name}_test_{test_tgt_name}"
                exe = copy.deepcopy(test_tgt)
                exe.dependencies.add(lib_name)
                process_exe(nw, project.libraries, exe_name, exe)
                test_exes.append(exe_name)

    if not config.fuzzing:
        for exe_name, exe in project.executables.items():
            process_exe(nw, project.libraries, exe_name, exe)

    nw.close()
