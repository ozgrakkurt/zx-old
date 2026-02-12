from typing import Optional, Dict, Set, List

from definition import Executable, Library, Project, Profile


PROFILES: Dict[str, Profile] = {
    "release": Profile(opt_level=2, enable_debug_assert=False, enable_debug_info=False),
    "debug": Profile(opt_level=0, enable_debug_assert=True, enable_debug_info=True),
    "opt_debug": Profile(opt_level=2, enable_debug_assert=True, enable_debug_info=True),
}


SHIN: Library = Library(
    source_files=set(["shin_page_alloc.c", "shin_err_trace.c"]),
    test_targets={
        "shin_err_trace_test": set([]),
        "shin_alloc_test": set([]),
        "shin_rapidhash_v3_test": set([]),
        "shin_hashtable_test": set([]),
    },
    fuzz_targets={
        "shin_hashtable_fuzz": set([]),
    },
    bench_targets={
        "shin_hashtable_bench": set([]),
    },
)

STREAMVBYTE: Library = Library(
    source_files=set(["streamvbyte_0124_encode.c", "streamvbyte_0124_decode.c"]),
    test_targets={
        "streamvbyte_test": set([]),
    },
    fuzz_targets={
        "streamvbyte_roundtrip_fuzz": set([]),
        "streamvbyte_decode_fuzz": set([]),
    },
    bench_targets={
        "streamvbyte_bench": set([]),
    },
)

XORF: Library = Library(
    source_files=set(["xorf.c"]),
)

PROJECT: Project = Project(
    libraries={
        "shin": SHIN,
        "streamvbyte": STREAMVBYTE,
        "xorf": XORF,
    },
)
