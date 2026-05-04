from typing import Optional, Dict, Set, List

from definition import Executable, Library, Project, Profile


PROFILES: Dict[str, Profile] = {
    "test0": Profile(opt_level=0, dev=True, debug_info=True, fuzz=False),
    "test1": Profile(opt_level=0, dev=False, debug_info=True, fuzz=False),
    "test2": Profile(opt_level=2, dev=True, debug_info=True, fuzz=False),
    "test3": Profile(opt_level=2, dev=False, debug_info=True, fuzz=False),
    "test4": Profile(opt_level=3, dev=True, debug_info=True, fuzz=False),
    "test5": Profile(opt_level=3, dev=False, debug_info=True, fuzz=False),
    "bench0": Profile(opt_level=2, dev=False, debug_info=False, fuzz=False),
    "bench1": Profile(opt_level=3, dev=False, debug_info=False, fuzz=False),
    "fuzz0": Profile(opt_level=2, debug_info=True, dev=True, fuzz=True),
    "fuzz1": Profile(opt_level=3, debug_info=True, dev=True, fuzz=True),
    "fuzz2": Profile(opt_level=0, debug_info=True, dev=True, fuzz=True),
}

ZX: Library = Library(
    source_files=set(
        [
            "zx_libc",
            "zx_fuzz",
        ]
    ),
    test_targets=set(
        [
            "zx_int_test",
            "zx_slice_test",
            "zx_libc_test",
            "zx_time_test",
            "zx_xoshiro256_test",
            "zx_rapidhash_v3_test",
            "zx_page_alloc_test",
            "zx_alloc_test",
            "zx_fmt_test",
            "zx_err_trace_test",
            "zx_hashtable_test",
            "zx_bitmap_test",
            "zx_queue_test",
            "zx_slab_test",
        ]
    ),
    bench_targets=set(["zx_rapidhash_v3_bench"]),
    fuzz_targets=set(
        [
            "zx_strlen_fuzz",
            "zx_memset_fuzz",
            "zx_memcmp_fuzz",
            "zx_memcpy_fuzz",
            "zx_hashtable_fuzz",
            "zx_bitmap_fuzz",
            "zx_queue_fuzz",
            "zx_slab_fuzz",
        ]
    ),
)

NOZ: Library = Library(
    source_files=set(["noz"]),
    test_targets=set(["noz_test"]),
    fuzz_targets=set(["noz_fuzz", "noz_decompress_fuzz"]),
)

PROJECT: Project = Project(
    libraries={
        "zx": ZX,
        "noz": NOZ,
    },
)
