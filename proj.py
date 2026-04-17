from typing import Optional, Dict, Set, List

from definition import Executable, Library, Project, Profile


PROFILES: Dict[str, Profile] = {
    "debug": Profile(),
    "opt_debug": Profile(opt_level=2),
    "release": Profile(
        opt_level=2,
        debug=False,
    ),
    "fuzz": Profile(opt_level=2, fuzz=True),
    "fuzz_debug": Profile(fuzz=True),
}

ZX: Library = Library(
    source_files=set(
        [
            # "zx_assert",
            # "zx_err",
            # "zx_fuzz",
            # "zx_mem",
            # "zx_page_alloc",
            "zx_start",
            # "zx_sys",
        ]
    ),
    test_targets={
        # "zx_hashtable_test": set([]),
        # "zx_alloc_test": set([]),
        # "zx_err_test": set([]),
        # "zx_queue_test": set([]),
        # "zx_slab_test": set([]),
        # "zx_bitmap_test": set([]),
        "zx_int_test": set([]),
    },
    # bench_targets={
    #     "zx_hashtable_bench": set([]),
    #     "zx_err_bench": set([]),
    # },
    # fuzz_targets={
    #     "zx_hashtable_fuzz": set([]),
    #     "zx_slab_fuzz": set([]),
    #     "zx_queue_fuzz": set([]),
    # },
)

# NOZ: Library = Library(
#     source_files=set(["noz"]),
#     test_targets={
#         "noz_test": set([]),
#     },
#     bench_targets={
#         "noz_bench": set([]),
#     },
#     fuzz_targets={
#         "noz_roundtrip_fuzz": set([]),
#         "noz_decompress_safe_fuzz": set([]),
#     },
# )

# FIBER: Library = Library(
#     source_files=set(["fiber"]),
#     test_targets={
#         "fiber_test": set([]),
#     },
#     bench_targets={
#         "fiber_bench": set([]),
#     },
# )
#
# FIO: Library = Library(
#     dependencies=set(["fiber"]),
#     source_files=set(["fio_uring"]),
#     test_targets={
#         "fio_uring_test": set([]),
#     },
# )

PROJECT: Project = Project(
    libraries={
        "zx": ZX,
        # "noz": NOZ,
        # "fiber": FIBER,
        # "fio": FIO,
    },
)
