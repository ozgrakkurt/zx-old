from dataclasses import dataclass, field
from typing import Optional, Dict, Set, List
import os

ROOTPATH = os.path.dirname(os.path.abspath(__file__))


@dataclass
class Library:
    source_files: Set[str]
    dependencies: Set[str] = field(default_factory=set)
    test_targets: Set[str] = field(default_factory=set)
    fuzz_targets: Set[str] = field(default_factory=set)
    bench_targets: Set[str] = field(default_factory=set)


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
    opt_level: int
    debug: bool
    fuzz: bool
