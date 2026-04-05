# AGENTS.md for zx2

## Build System

**Entry point:** `./configure.py` generates `build.ninja` files for each profile.

**Command order:**
- First: `./configure.py` (must run before any ninja commands)
- Then: `ninja -C build/<profile> <target>`

**Profiles:**
- `debug` - unoptimized, full debug info
- `opt_debug` - optimized, full debug info
- `release` - optimized, stripped
- `fuzz` - optimized, debug info, fuzz targets included
- `fuzz_debug` - debug info, fuzz targets included

**No external dependencies** - even libc is excluded.

## Running Tests

**Run all tests:**
```bash
ninja -C build/debug test
```

**Run single test:**
```bash
ninja -C build/debug <library>_<test_name>_test
```

**Test expectations:**
- Must run instantly
- Must be deterministic
- Must never be flaky
- Multiple logical tests grouped in one file via `main()`

## Running Benchmarks

**Run single benchmark (one-by-one):**
```bash
ninja -C build/release <library>_<bench_name>_bench
```

## Running Fuzz Tests

**Run single fuzz test (one-by-one, runs without stopping):**
```bash
ninja -C build/fuzz <library>_<fuzz_name>_fuzz
```

## Project Structure

**Libraries:** `zx`, `noz`, `fiber`, `fio`

**Directory layout:**
- Headers + public interfaces: `include/<library>/`
- Implementation + tests: `src/<library>/`
- Include path: `-Iinclude`

**Library naming rule:** Headers use `<libx/libx_header.h>` syntax

**Test/bench/fuzz naming:**
- Test: `<lib>_<name>_test`
- Bench: `<lib>_<name>_bench`
- Fuzz: `<lib>_<name>_fuzz`

## Code Style

**C++ as C-style C++:**
- Only C++ features: templates, template specialization, class enums
- **NO:** namespaces, C++ std lib, constructors/destructors, private/public distinction, classes
- **NO** preprocessor macros - prefer constexpr

**File extensions:**
- Headers: `.h` (public interfaces)
- Sources: `.cc` (implementation)

**Data structures:** Always use `struct`, never `class`

**Error handling:** Use `ZXStatus` return type + `ZXErrStack*` parameter
- Macros: `ZX_ERR_RETURN`, `ZX_ERR_ASSERT`, `ZX_ERR_TRY`, `ZX_ERR_TRY_FATAL`
- All macros work on all build types

**Assertions:** `ZX_ASSERT()` (always enabled), `ZX_DBG_ASSERT()` (disabled in release)

**Memory allocation:**
- `ZXAlloc` interface via `ZXDebugAlloc` (tests/fuzz) or `ZXArenaAlloc`
- Uses `mmap`/`munmap` for OS-level allocation
- Tests must use `ZXDebugAlloc` for overflow detection

**SIMD:** Use inline assembly with AT&T syntax (no `immintrin.h`)

**Integer types:** Use `_BitInt` types from `<zx/zx_def.h>` (e.g., `zx_u64`, `zx_i64`)
- Checked arithmetic functions from `<zx/zx_int.h>` for size calculations

## Platform

**Only x86_64 Linux with AVX512** (znver4+). Platform-specific code requires guards:
```cpp
#ifndef __AVX2__
#error "avx2 is required"
#endif
```

## Measuring Time

Use `ZXTimer` from `<zx/zx_time.h>` - measures cycles only, never time units.

## API Versioning

No API breakage ever. Add new functionality, don't refactor existing APIs.

## Code Organization

**Implementation docs inside source files only.** Never outside.

**Test files:** Regular `int main()` programs, no special frameworks.

**No `main()` in library source files** - only in test/fuzz/bench targets.
