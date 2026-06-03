from typing import Optional, Dict, Set, List

from definition import Executable, Library, Project

ZX: Library = Library(
    source_files=set(
        [
            "zx_fuzz",
            "zx_panic",
        ]
    ),
    test_targets=set(
        [
            "zx_int_test",
            "zx_slice_test",
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
    bench_targets=set(["zx_rapidhash_v3_bench", "zx_xoshiro256_bench"]),
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
            "zx_alloc_fuzz",
        ]
    ),
)

NOZ: Library = Library(
    source_files=set(["noz"]),
    test_targets=set(["noz_test"]),
    fuzz_targets=set(["noz_fuzz", "noz_decompress_fuzz"]),
    bench_targets=set(["noz_bench"]),
)

ZIO: Library = Library(
    source_files=set(["zio"]),
)

PROJECT: Project = Project(
    libraries={
        "zx": ZX,
        "noz": NOZ,
    },
)
