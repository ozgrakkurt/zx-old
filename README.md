# zx2

Anyone intending to use or modify this project is highly recommended to go through this document at least once.

Reading some code is also highly recommended as the code in this project is
designed to be very minimal and understandable. Code for the `zx` library is a good place to start.

This document is intended to be read from start to finish. It is not intended to be used as a reference.

This project includes:
- A light build generator written in python. This system generates `ninja.build` files
so the project can be built, tested, benchmarked, fuzz tested etc. using `ninja`.
The entry point for the build generation is `configure.py`, this file depends on the
other python files that are in the root of the project.
- A base library named `zx` which replaces `libc` APIs so the project doesn't require linking `libc`.
It also includes type definitions, utilities and data structures that would be expected from a standard library.
- A compression library named `noz`. `noz` is a very simple compression library that uses
byte masking to compress out the zero bytes. It is intended to be at least as fast as `memcpy`.
- A library for implementing user-space context switching, this library is named `fiber`.
This is intended to be used for doing async-io.
- A library for doing async-io using `io_uring` and the `fiber` library, this library is named `fio`.

## Build System

We use a build system that is comprised of `python` scripts that generate `build.ninja` files.

We then use `ninja` to `build`, `test`, `benchmark` and `fuzz test` the project.

Development flow mostly works like this:
- Run `./configure.py` to generate the `./build` folder.
- Run `ninja -C build/debug test` to run all tests.

`./configure.py --help` can be used to see the CLI usage for the `configure.py` script.

We do not use any external dependencies. We don't even use C++ standard library or libc or standard headers.

### Project Structure 

The project structure is defined in `./proj.py`.

The types used for defining the project structure come from `./definition.py`.

We use `.cc` for c++ source files and `.h` for c++ header files. All code is compiled as c++.

Naming rules for test/bench/fuzz targets are:
- All test targets are suffixed with parent library name and postfixed with `_test`.
- All benchmark targets are suffixed with parent library name and postfixed with `_bench`.
- All benchmark targets are suffixed with parent library name and postfixed with `_fuzz`.

Header and source files are split like this:
- Headers that include interface and headers that include inlined code go to `./include/<library name>/` directory.
- Source files, test files, benchmark files, fuzz files
and header files that include implementation details go to `./src/<library name>/` directory.
- The `./include` folder is added to the include path when compiling all code.
So a library can include a header located at `./include/libx/libx_heady.h` like this `#include <libx/libx_heady.h>`

For example for a library named `mylib` can be structured like this:
- `./include/mylib/mylib.h`
- `./src/mylib/mylib_impl_details.h`
- `./src/mylib/mylib.cc`
- `./src/mylib/mylib_simple_test.cc`
- `./src/mylib/mylib_complex_test.cc`
- `./src/mylib/mylib_test.cc`
- `./src/mylib/mylib_bench.cc`
- `./src/mylib/mylib_roundtrip_fuzz.cc`
- `./src/mylib/mylib_mem_fuzz.cc`

These files would be included in the project structure by including them in the `proj.py` file like this:

```python
MYLIB: Library = Library(
    dependencies=set(["mydep0", "mydep1"]),
    source_files=set(["mylib.cc"]),
    test_targets={
        "mylib_simple_test": set(["mydep1", "mydep2"]),
        "mylib_complex_test": set([]),
        "mylib_test": set([]),
    },
    bench_targets={
        "mylib_bench": set([]),
    },
    fuzz_targets={
        "mylib_roundtrip_fuzz": set([]),
        "mylib_mem_fuzz": set([]),
    },
)
```

- Each target is suffixed by the name of the library and postfixed by their kind (`test`/`bench`/`fuzz`).
- The source files are written including the extension (`.cc`), but targets are not.
- Header files are not included in `proj.py`.
- Targets can have extra dependencies independent of the library they depend on.

`proj.py` has to define a project `PROJECT: Project` variable at the top level to combine all of the libraries and
executables that it defines.

### Running Tests

Run all tests with `ninja -C build/debug test`.

Tests should always run "instantly" and there shouldn't be any flaky tests, so there should 
never be a reason to run individual tests. Except if a part of the project isn't compiling and
we want to test some part of the project that is compiling. In this case we can run a specific test like this:

`ninja -C build/debug mylib_simple_test`

More information about tests can be found in the `Writing Tests` section.

### Running Benchmarks

Benchmarks have to be run one-by-one.

For example to run the benchmark target defined in `mylib_bench.cc` in the above example:
`ninja -C build/release mylib_bench`

Generally:
`ninja -C build/<profile> <library name>_[bench tgt name_]bench`

More information about benchmarks can be found in the `Writing Benchmarks` section.

### Running Fuzz Tests

Fuzz tests have to be run one-by-one.

For example to run the fuzz tests target defined in `mylib_roundtrip_fuzz.cc` in the above example:
`ninja -C build/fuzz mylib_roundtrip_fuzz`

Generally:
`ninja -C build/fuzz <library name>_[fuzz tgt name_]fuzz`

Fuzz tests run without stopping. User should decide when to stop the test.

More information about fuzz tests can be found in the `Writing Fuzz Tests` section.

### Compiler/Linker Flags

We do not use `libc` and we don't include system headers. We achieve this by passing these flags:
- -nostdlib
- -nostdinc

We disable the c++ standard library and unwanted features with these flags:
- -fno-exceptions
- -fno-rtti
- -nostdlib++
- -fno-threadsafe-statics

We also try enable as many compiler warnings as we can without making it too much of a hassle.

All compiler warnings are treated as errors.

### Profiles

- `release`, generates optimized and stripped build that doesn't include any debug information.
- `debug`, generates unoptimized build with full debug info.
- `opt_debug`, generates optimized build with full debug info.
- `fuzz`, generates optimized build with full debug info. This profile also builds the fuzz testing targets.
Read more about this in Fuzz Testing topic.

## Code Style

### Programming Language 

All code is written using `C++` with `c++20` standard, and compiled using `clang++`.

The specific compiler/linker flags used are defined in `configure.py` and are mostly constant.

We use `.h` for header files and `.cc` for source files. **ALL** code is **C** style **C++**.
**ALL** code is compiled by `clang++`.

### C++ Features

Code is mostly written in style of `C` despite being compiled as `C++`.

Only C++ features used are templates, template specialization and class enums.
No other C++ feature should be used except these. 

For example, we do **NOT** use features like:
- Namespaces.
- C++ template library.
- Constructors/Destructors.
- private/public method differentiation on structs.
- classes.

Usage of templates should be very carefully considered and templates should not be used unless it is really
essential. An example where templates are a good trade-off is container types like a hashtable or a linked list.

Usage of preprocessor macros should also be very carefully considered and be avoided if possible. Always prefer
constexpr to using macros.

### Documenting Code

Implementation and architecture of the code itself should always be documented inside the source files and never
outside of the source files.

Obvious code should never be documented. Comments should be kept as short as possible to avoid fatigue when
reading code.

Complicated code that is difficult to make sense of should have extensive documentation that explains how things
come together to make sense.

### Error Handling

Errors are returned using `ZXStatus` from functions. A function that can fail might look like this:
```c++
ZXStatus falliblefunc(...) {...}
```
`ZXStatus` is a class enum. `ZXStatus::Ok` means success,
so the function would return `ZXStatus::Ok` if an error didn't occur.

We also always pass in an `ZXErrStack*` into fallible functions. The error stack is used for recording the stack
trace of an error.
```c++
ZXStatus falliblefunc(..., ZXErrStack* err_stack) {...}
```
We can collect the stack trace into a string for arbitrary usage or just print it to stderr and abort.

Stack traces work exactly the same on any build type so they are reliable to use in optimized and stripped builds.

Error handling is implemented by a series of macros.

A full fallible function might look like this:
```c++
ZXStatus falliblefunc(ZXAlloc const* alloc, ZXErrStack* err_stack) {
    // return AssertFail error if alloc is a null pointer.
    // also push the source location where ZX_ERR_RETURN macro was used into
    // the error stack.
    if (alloc == nullptr) {
        ZX_ERR_RETURN((ZXStatus::AssertFail), err_stack);
    }

    // return AssertFail error if alloc is a null pointer.
    // same as the above but just takes a predicate and returns
    // AssertFail error if predicate evaluates to false.
    ZX_ERR_ASSERT((alloc != nullptr), err_stack);

    zx_u8* mem = nullptr;
    // allocate 1024 bytes and assign the pointer to mem, return error if that fails.
    // both code positions (this location and the original err location inside alloc code)
    // will be pushed to the error stack in case of an error.
    ZX_ERR_TRY((alloc->alloc<zx_u8>(1024, &mem, err_stack)), err_stack);

    // Same as ZX_ERR_TRY but will print a stack trace to stderr and 
    // abort the program instead of just early returning from the function.
    ZX_ERR_TRY_FATAL((alloc->alloc<zx_u8>(1024, &mem, err_stack)), err_stack);

    return ZXStatus::Ok;
}
```

All of these macros work exactly the same on all builds and they aren't disable on optimized and stripped builds.

All of these types, functions and macros are defined in `<zx/zx_err.h>`. It is recommended to at least skim over that code
to understand error handling.

### Assertions

Assertions are implemented using `ZX_ASSERT((my_predicate))` and `ZX_DBG_ASSERT((my_predicate))` macros.

Both macros print source location and abort the program if predicate evaluates to false.

`ZX_ASSERT` is always enabled regardless of the build profile.

`ZX_DBG_ASSERT` is disabled in builds that use the `release` profile.

It is recommended to use `ZX_ASSERT` even in performance critical contexts, unless there is a visible
performance difference.

All functionality related to these macros are defined in `<zx/zx_assert.h>`

### Memory Allocation

We allocate memory from the operating system using **mmap** syscall and give memory memory back to os using the
**munmap** syscall. This is named as **page allocation** and the interface for it is in `<zx/zx_page_alloc.h>`
and the implementation is in `./src/zx/zx_page_alloc.cc`.

We then utilize the memory we acquired from the OS using the `ZXAlloc` interface. This interface implements
and allocator that gives out memory chunks.

`ZXAlloc` is essentially an `arena allocator` that lets the user make allocations and then do a `reset` to invalidate
all allocations at once and bring the allocator back to it's max capacity.

`ZXAlloc` is implemented by the `ZXArenaAlloc` and the `ZXDebugAlloc`.

`ZXArenaAlloc` just takes a pointer and a capacity and uses that memory as the it's pool.

`ZXDebugAlloc` takes in a capacity and directly allocates memory from the os itself. It also inserts
protected regions between the allocations to catch buffer overflows and out of bounds reads. This has a
significant runtime performance hit so we only use this allocator in tests and fuzz tests.

All tests and fuzz tests should use `ZXDebugAlloc`.

`ZXAlloc` interface is defined in `<zx/zx_alloc.h>`.
`ZXArenaAlloc` interface is defined in `<zx/zx_arena_alloc.h>`.
`ZXDebugAlloc` interface is defined in `<zx/zx_debug_alloc.h>`.

Example `ZXDebugAlloc` usage can be found in most `test`/`fuzz test`/`bench` target implementations.

### String Formatting and Printing

String formatting is done through the `ZXFmt` type and `ZX_FMT` macro defined in `<zx/zx_fmt.h>`.

`ZXFmt` is a container that holds the string that is being formatted.

`ZX_FMT(fmt, arg0, arg1, ...)` is used for pushing into the `ZXFmt` container.

`ZX_FMT` takes any type of argument that is supported in any position. It can take up-to 16 parameters at once.
Multiple calls must be used if more parameters are neeeded.

Example of formatter usage can be found in benchmarking code.

All functions relating to formatting **DO NOT** do error handling. They silently ignore if underlying
buffer is out of capacity. The result will be that the buffer will be partially written.

Printing is implemented by `zx_io_print` function defined in `<zx/zx_io.h>`. This function is a thin wrapper
around the `write` syscall. It can be used with `ZX_STDERR` or `ZX_STDOUT` for printing to the "console".

A `ZXFmt` instance can be reset after printing to start creating a new message using the same memory allocation.
This is done by calling the `reset` method on it.

### System Calls

We don't depend on `libc` so we have to implement the syscalls inside the project.

We use inline assembly for implementing the syscalls manually.

All syscalls interfaces are defined in `<zx/zx_sys.h>` and implemented in `./src/zx/zx_sys.cc`.

### Writing Tests 

Tests are written into files that have a regular `int main(void)` function and they are defined as regular programs
that don't print anything.

Tests are expected to abort or just return a non-zero return code from the main function to indicate failure. They
are expected to also write failure information into stderr in this case.

Tests are supposed to build and run instantly on any profile.

For example doing `./configure.py && ninja -C build/debug test` on a fresh build should feel instant.
Same should be the case when using `opt_debug` profile or `release` profile too.

Tests should not be flaky. If doing a flaky test is useful, it should be implemented as an executable target.

Tests should always be deterministic.

Test executables are always run in parallel with each other.

Multiple logical tests can be grouped into a single file by just calling each function in the main function of that file.

To understand more about this, check out `./proj.py` to locate some test targets
and check their sources in the `./src` folder.

### Measuring Time

Time is only measured in terms of cycles and never in terms of time units like nanoseconds.

`<zx/zx_time.h>` implements `ZXTimer` which is used for measuring the cycles.

### Writing Benchmarks 

Benchmarks are just regular programs that print arbitrary information. They have a regular `int main(void)` that 
is expected to call all relevant functions.

An example might be a benchmark that benchmarks a compression function. It will do some setup, call some functions
while measuring their running time (in terms of cycles, see Measuring Time section). And then it will print this
information.

Writing benchmarks is hard so it is often the better option to not even try. Just getting some low hanging benefit
by doing a crude program is often the right trade-off.

Benchmarks will be more accurate and useful as they are done on the higher levels of a program. This is because
performance depends on so many things that it is basically impossible to understand or evaluate benchmarks that
are just running a tiny portion of a program like a hashtable lookup. This kind of benchmark will only give a 
crude idea of if that function is running roughly as expected.

A good example of a simple and primitive benchmark is `./src/noz/noz_bench.cc`.

### Writing Fuzz Tests 

### Instrumenting Code for Fuzz Tests

The instrumentation macros are defined in `<zx/zx_fuzz_instr.h>` and they are used for marking code paths.

The fuzzer will try to engineer inputs based on how many code paths are hit and which code paths are hit.

The fuzzer also will display how many code paths it was able to reach so far and how many it couldn't reach yet.
It will also show the specific target code paths that it couldn't reach yet. The developer that is running or writing
the fuzz test is expected to look at this information to improve the fuzz test and also is expected to use this
information to decide when to stop the fuzz test.

The error handling and assertion macros implicitly put fuzz instrumentation into the code so the developer
does have to manually put fuzz markers around error handling or assertion code.

An example of instrumentation can be found in `./include/zx/zx_hashtable.h`.

### API Versioning

There should be no API breakage ever. The documented pieces of API will never change.

New functionality can be added by adding more APIs.

Introducing a better way of doing something doesn't require refactoring all code that did that thing. This creates
needles work and is marginally useful.

Only exception to this is when the improvement is very dramatic or there is bug in the old code.

### Platform

The code is only supposed to work on x86_64 linux machines that have most new `avx512` features.
It is developed and run on machines that have `znver4` or later architectures.

We don't need to handle cross-platform use cases but we also don't want to make it impossible to add that later on.

So we try to limit platform specific code somewhat and try to put
guards like this in files that use platform specific functionality:
```c++
#ifndef __AVX2__
#error "avx2 is required"
#endif
```

### Integers and Integer Arithmetic

The code exclusively uses `_BitInt` integer types except where it is impossible, like when doing inline assembly.
The `_BitInt` types aren't in C++ standard and `g++` doesn't implement them,
but we use them since we are using `clang++` and they don't suffer
from auto promotion rules that effect normal integer types in `C` and `C++`.

`<zx/zx_def.h>` defines integers like `zx_u64`, `zx_i16` etc. These are `typedef`s of `_BitInt`.

All code should use `<zx/zx_def.h>` for integer and floating point types.

There are some utility functions defined in `<zx/zx_int.h>` header. These include:
- Checked arithmetic functions like `zx_chk_add_u64` or `zx_chk_shl_u64`.
- Bit utilities like `zx_ctz_u64` or `zx_bit_width_u64`.
- Functions that are about power of two calculation like `zx_next_pow_two_u8` and `zx_is_pow_two_u64`.

All code that is doing size calculations for memory allocation or
some data structure should use the checked aritmetic functions.

Most code that isn't in a hot loop should always use checked arithmetic,
unless it is extremely clear that an operation can't overflow or divide-by-zero or similar.

### Data Structures

We always use `struct` and never use `class` for defining data structures.
We use `class enum` for defining enums and never use regular `C` style `enum`.
We never use constructors/destructors.

The typical function that would be used to initialize a data structure is defined like this:
```c++
struct MyStructure {
    static inline ZXStatus init(ZXAlloc const* alloc, zx_u64 mysize, MyStructure* data, ZXErrStack *err_stack) {...}
};
```
This method would report errors using the returned `ZXStatus` and
the stack passed in as `ZXErrStack`. It will initialize the passed in `MyStructure* data`.

We generally don't define a deinit function because we allocate memory using arena allocation pattern.
But some structures do need it, for example a structure that
holds an io_uring instance will need to do system calls at init and
deinit functions to allocate/free system resources relating to the io_uring instance.

A deinit function might look like this:
```c++
struct MyStructure {
    static inline ZXStatus deinit(MyStructure* data, ZXErrStack *err_stack) {...}
};
```
This method will release resources relating to the structure and
it will set status of passed in `MyStruct* data` to some "zero" value.

A typical "zero" value for a pointer is `nullptr`.

### SIMD and Inline Assembly

The default way of doing SIMD in C++ is to use the SIMD intrinsics defined in `<immintrin.h>` header.

We don't include any system headers by passing `--nostdinc` to the compiler,
so we don't have access to the `<immintrin.h>` header.

We instead use inline assembly.

We use at&t syntax inline assembly as that is the default in `clang++`.

Some examples of this can be found in:
- `./include/zx/zx_hashtable.h`
- `./src/noz/noz.cc`
