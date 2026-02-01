from typing import Optional, Dict, Set, List

from definition import Executable, Library, Project, Profile

PROFILES: Dict[str, Profile] = {
    "release": Profile(opt_level=2, enable_debug_assert=False, enable_debug_info=False),
    "debug": Profile(opt_level=0, enable_debug_assert=True, enable_debug_info=True),
    "opt_debug": Profile(opt_level=2, enable_debug_assert=True, enable_debug_info=True),
}

SHIN: Library = Library(source_files=set(["anan.c"]))

PROJECT: Project = Project(
    libraries={
        "shin": SHIN,
    },
)
