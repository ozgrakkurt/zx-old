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
            "zx_assert.cc",
            "zx_err.cc",
            "zx_fuzz.cc",
            "zx_mem.cc",
            "zx_page_alloc.cc",
            "zx_start.cc",
            "zx_sys.cc",
        ]
    ),
    test_targets={
        "zx_hashtable_test": set([]),
        "zx_alloc_test": set([]),
        "zx_err_test": set([]),
        "zx_queue_test": set([]),
        "zx_slab_test": set([]),
        "zx_bitmap_test": set([]),
    },
    bench_targets={
        "zx_hashtable_bench": set([]),
        "zx_err_bench": set([]),
    },
    fuzz_targets={
        "zx_hashtable_fuzz": set([]),
        "zx_slab_fuzz": set([]),
        "zx_queue_fuzz": set([]),
    },
)

NOZ: Library = Library(
    source_files=set(["noz.cc"]),
    test_targets={
        "noz_test": set([]),
    },
    bench_targets={
        "noz_bench": set([]),
    },
    fuzz_targets={
        "noz_roundtrip_fuzz": set([]),
        "noz_decompress_safe_fuzz": set([]),
    },
)

FIBER: Library = Library(
    source_files=set(["fiber.cc"]),
    test_targets={
        "fiber_test": set([]),
    },
    bench_targets={
        "fiber_bench": set([]),
    },
)
#
# FIO: Library = Library(
#     dependencies=set(["fiber"]),
#     source_files=set(["fio_uring.cc", "fio_exe.cc"]),
#     test_targets={
#         "fio_uring_test": set([]),
#     },
# )

PROJECT: Project = Project(
    libraries={
        "zx": ZX,
        "noz": NOZ,
        "fiber": FIBER,
        # "fio": FIO,
    },
)
