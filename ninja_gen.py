"""Python module for generating .ninja files.

Builds on top of ninja_syntax.py to export a declarative API for
generating .ninja files to build a C project.
"""

import os
import sys
from dataclasses import dataclass, field
from typing import Optional, Dict, Set, List

import ninja_syntax


@dataclass
class TestTarget:
    source_files: Set[str] = field(default_factory=set)
    private_header_files: Set[str] = field(default_factory=set)
    dependencies: Set[str] = field(default_factory=set)


@dataclass
class Library:
    source_files: Set[str] = field(default_factory=set)
    public_header_files: Set[str] = field(default_factory=set)
    private_header_files: Set[str] = field(default_factory=set)
    dependencies: Set[str] = field(default_factory=set)
    test_targets: Dict[str, TestTarget] = field(default_factory=dict)
    fuzz_targets: Dict[str, TestTarget] = field(default_factory=dict)


@dataclass
class Executable:
    source_files: Set[str] = field(default_factory=set)
    private_header_files: Set[str] = field(default_factory=set)
    dependencies: Set[str] = field(default_factory=set)


@dataclass
class Project:
    libraries: Dict[str, Library] = field(default_factory=dict)
    executables: Dict[str, Executable] = field(default_factory=dict)


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
        len(lib.source_files) > 0 or len(lib.public_header_files) > 0,
        "a library has to have at least one source file or at least one public header file",
    )
    for dep_name in lib.dependencies:
        assert (dep_name != lib_name, f"library {lib_name} depends on itself")
        assert (
            dep_name in libs,
            f"dep {dep_name} of library {lib_name} doesn't exist in library list",
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
        # Search for libraries in the build folder.
        "-L$build_path",
    ]

    if config.no_libc:
        ldflags.append("-nostdlib")
        ldflags.append("-fno-builtin")

    if config.thin_lto:
        ldflags.append("-flto=thin")

        # Enable incremental ThinLTO by giving a cache path.
        ldflags.append(f"-Wl,--thinlto-cache-dir=$build_path/thinlto-cache")

    return ldflags


def gen_ninja_build(
    build_path: str, profile: Profile, config: Config, project: Project
) -> None:
    assert (os.path.isabs(build_path), "build_path has to be an absolute path")
    validate_profile(profile)
    validate_config(config)
    validate_project(project)

    cflags = make_cflags(profile, config)
    ldflags = make_ldflags(build_path, config)

    # build_path is required to be absolute by validation
    ninjapath = os.path.join(build_path, "build.ninja")
    nw = ninja_syntax.Writer(open(ninjapath, "w"))

    nw.variable("build_path", build_path)

    nw.newline()

    nw.variable("cflags", " ".join(cflags))
    nw.variable("ldflags", " ".join(ldflags))

    nw.newline()

    nw.variable("cc", args.cc)
    nw.variable("ar", args.ar)

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
        description=f"AR $out",
    )
    nw.rule(
        "link",
        command="$cc $ldflags -o $out $in $libs",
        description=f"LINK $out",
    )

    nw.newline()

    nw.close()
