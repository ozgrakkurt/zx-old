from typing import Optional, Dict, Set, List

from definition import Executable, Library, Project, Profile


PROFILES: Dict[str, Profile] = {
    "debug": Profile(),
    "opt_debug": Profile(opt_level=2),
    "release": Profile(opt_level=2, enable_debug_assert=False, enable_debug_info=False),
    "fuzz": Profile(opt_level=2, enable_fuzzing=True),
    "fuzz_debug": Profile(enable_fuzzing=True),
}

ZX: Library = Library(
    source_files=set(
        [
            "zx_alloc.c",
            "zx_assert.c",
            "zx_err.c",
            "zx_fuzz.c",
            "zx_mem.c",
            "zx_page_alloc.c",
            "zx_start.c",
            "zx_sys.c",
        ]
    ),
    test_targets={
        "zx_hashtable_test": set([]),
        "zx_alloc_test": set([]),
        "zx_err_test": set([]),
    },
    bench_targets={
        "zx_hashtable_bench": set([]),
        "zx_err_bench": set([]),
    },
    fuzz_targets={
        "zx_hashtable_fuzz": set([]),
    },
)

PROJECT: Project = Project(
    libraries={
        "zx": ZX,
    },
)
