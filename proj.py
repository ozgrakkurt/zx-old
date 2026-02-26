from typing import Optional, Dict, Set, List

from definition import Executable, Library, Project, Profile


PROFILES: Dict[str, Profile] = {
    "debug": Profile(),
    "opt_debug": Profile(opt_level=2),
    "release": Profile(opt_level=2, enable_debug_assert=False, enable_debug_info=False),
    "fuzz": Profile(opt_level=2, enable_fuzzing=True),
}

ZX: Library = Library(
    source_files=set(["zx_sys.c", "zx_mem.c"]),
)

PROJECT: Project = Project(
    libraries={
        "zx": ZX,
    },
)
