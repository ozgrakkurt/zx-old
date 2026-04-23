from typing import Optional, Dict, Set, List

from definition import Executable, Library, Project, Profile


PROFILES: Dict[str, Profile] = {
    "debug": Profile(opt_level=0, dev=True, debug_info=True, fuzz=False),
    "debug_opt": Profile(opt_level=2, dev=True, debug_info=True, fuzz=False),
    "release": Profile(
        opt_level=2,
        debug_info=False,
        dev=False,
        fuzz=False,
    ),
    "release_dbg": Profile(
        opt_level=2,
        debug_info=True,
        dev=False,
        fuzz=False,
    ),
    "fuzz": Profile(opt_level=2, debug_info=True, dev=True, fuzz=True),
    "fuzz_noopt": Profile(opt_level=0, debug_info=True, dev=True, fuzz=True),
}

ZX: Library = Library(
    source_files=set(
        [
            "zx_libc",
            # "zx_assert.cc",
            # "zx_err.cc",
            # "zx_fuzz.cc",
            # "zx_mem.cc",
            # "zx_page_alloc.cc",
            # "zx_start.cc",
            # "zx_sys.cc",
        ]
    ),
    test_targets=set(
        [
            "zx_int_test",
            "zx_slice_test",
            "zx_libc_test",
        ]
    ),
    # test_targets={
    #     "zx_hashtable_test": set([]),
    #     "zx_alloc_test": set([]),
    #     "zx_err_test": set([]),
    #     "zx_queue_test": set([]),
    #     "zx_slab_test": set([]),
    #     "zx_bitmap_test": set([]),
    #     "zx_int_test": set([]),
    # },
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
#     source_files=set(["noz.cc"]),
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
#
# FIBER: Library = Library(
#     source_files=set(["fiber.cc"]),
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
#     source_files=set(["fio_uring.cc"]),
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
