from dataclasses import dataclass, field
from typing import Optional, Dict, Set, List
import os

ROOTPATH = os.path.dirname(os.path.abspath(__file__))


@dataclass
class Library:
    source_files: Set[str] = field(default_factory=set)
    dependencies: Set[str] = field(default_factory=set)
    test_targets: Dict[str, Set[str]] = field(default_factory=dict)
    fuzz_targets: Dict[str, Set[str]] = field(default_factory=dict)
    bench_targets: Dict[str, Set[str]] = field(default_factory=dict)


@dataclass
class Executable:
    source_files: Set[str]
    dependencies: Set[str] = field(default_factory=set)


@dataclass
class Project:
    src_path: str = os.path.join(ROOTPATH, "src")
    include_path: str = os.path.join(ROOTPATH, "include")
    libraries: Dict[str, Library] = field(default_factory=dict)
    executables: Dict[str, Executable] = field(default_factory=dict)


@dataclass
class Profile:
    opt_level: int = 0
    enable_debug_info: bool = True
    enable_debug_assert: bool = True
    enable_fuzzing: bool = False
