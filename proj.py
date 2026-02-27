from typing import Optional, Dict, Set, List

from definition import Executable, Library, Project, Profile


PROFILES: Dict[str, Profile] = {
    "debug": Profile(),
    "opt_debug": Profile(opt_level=2),
    "release": Profile(opt_level=2, enable_debug_assert=False, enable_debug_info=False),
    "fuzz": Profile(opt_level=2, enable_fuzzing=True),
}

ZX: Library = Library(
    source_files=set(
        [
            "zx_sys.c",
            "zx_mem.c",
            "zx_page_alloc.c",
            "zx_err_trace.c",
            "zx_start.c",
            "zx_fuzz.c",
        ]
    ),
    test_targets={
        "zx_hashtable_test": set([]),
        "zx_alloc_test": set([]),
        "zx_err_trace_test": set([]),
    },
    bench_targets={
        "zx_hashtable_bench": set([]),
        "zx_err_trace_bench": set([]),
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
