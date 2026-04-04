# zx2

Anyone intending to use or modify this project is highly recommended to go through this document at least once.

Reading some example code for parts that are not clear is also highly recommended as the code in this project is
designed to be very minimal and understandable.

This project includes:
- A light build generator written in python. This system generates `ninja.build` files
so the project can be built, tested, benchmarked, fuzz tested etc. using `ninja`.
The entry point for the build generation is `configure.py`, this file depends on the
other python files that are in the root of the project.
- A base library named `zx` which replaces `libc` APIs so the project doesn't require linking `libc`.
It also includes type definitions, utilities and data structures that would be expected from a standard library.
- A compression library named `noz`. `noz` is a very simple compression library that uses
byte masking to compress out the zero bytes. It is intended to be at least as fast as `memcpy`.
- A library for implementing user-space context switching, this library is named `fiber`. This is intended to be used for doing async-io.
- A library for doing async-io using `io_uring` and the `fiber` library, this library is named `fio`.

# Build System

We do not use `libc` and we don't include system headers. We achieve this by passing:
- -nostdlib
- -nostdinc

We disable the c++ standard library and unwanted features with these flags:
- -fno-exceptions
- -fno-rtti
- -nostdlib++
- -fno-threadsafe-statics

## Building the code
All of these commands are expected to be run at the project root.

- First do configure step by running `./configure.py`.
- Then build everything by running something like `ninja -C build/<profile name>`. For example `ninja -C build/debug`.

## Profiles

- `release`, generates optimized and stripped build that doesn't include any debug information.
- `debug`, generates unoptimized build with full debug info.
- `opt_debug`, generates optimized build with full debug info.
- `fuzz`, generates optimized build with full debug info. This profile also builds the fuzz testing targets. Read more about this in Fuzz Testing topic.

## Fuzz Testing

Fuzz tests

# Coding Standard

All code is written using `C++` with `c++20` standard, and compiled using `clang++`.

The specific compiler/linker flags used are defined in `configure.py` and are mostly constant.

`./configure.py --help` can be used to see the CLI usage for the `configure.py` script.

The standard way of using `configure.py` is to run it without any cli arguments like this: `./configure.py`.

We use `.h` for header files and `.cc` for source files. ALL code is C++ and is processed by `clang++`.

## API Versioning

There should be no API breakage ever. The documented pieces of API will never change.

New functionality can be added by adding more APIs.

Introducing a better way of doing something doesn't require refactoring all code that did that thing. This creates
needles work and is marginally useful.

Only exception to this is when the improvement is very dramatic or there is bug in the old code.

## C++ Features

Code is mostly written like `C` code that follows the `C23` standard.

Only C++ features used are templates, template specialization and class enums. No other C++ feature should be used except these. 

For example, we do NOT use features like:
- Namespaces.
- C++ template library.
- Constructors/Destructors.
- private/public method differentiation on structs.
- classes.

## Platform

The code is only supposed to work on x86_64 linux machines that have most new `avx512` features.
It is developed and run on machines that have `znver4` or later architectures.

We don't need to handle cross-platform use cases but we also don't want to make it impossible to add that later on.

So we try to limit platform specific code somewhat and try to put guards like this in files that use platform specific functionality:
```c++
#ifndef __AVX2__
#error "avx2 is required"
#endif
```

## SIMD

The default way of doing SIMD in C++ is to use the SIMD intrinsics defined in `<immintrin.h>` header.

We don't include any system headers by passing `--nostdinc` to the compiler, so we don't have access to the `<immintrin.h>` header.

We instead use inline assembly. `clang++` has good support for inline assembly so it works out well.

Some examples of this can be found in:
- `include/zx/zx_hashtable.h`
- `src/noz/noz.cc`

## Integers

The code exclusively uses `_BitInt` integer types except where it is impossible, like when doing inline assembly.
The `_BitInt` types aren't in C++ standard and `g++` doesn't implement them, but we use them since we are using `clang++` and they don't suffer
from auto promotion rules that effect normal integer types in `C` and `C++`.

There are some utility functions defined in `<zx/zx_int.h>` header. These include:
- Checked arithmetic functions like `zx_chk_add_u64` or `zx_chk_shl_u64`.
- Bit utilities like `zx_ctz_u64` or `zx_bit_width_u64`.
- Functions that are about power of two calculation like `zx_next_pow_two_u8` and `zx_is_pow_two_u64`.

All code that is doing size calculations for memory allocation or some data structure should use the checked aritmetic functions.

Most code that isn't in a hot loop should always use checked arithmetic, unless it is extremely clear that an operation can't overflow or divide-by-zero or similar.

## Data structures

We always use `struct` and never use `class`. We use `class enum` for defining enums and never use regular `C` style `enum`.
We never use constructors/destructors.

The typical function that would be used to initialize a data structure is defined like this:
```c++
struct MyStructure {
    static inline ZXStatus init(zx_u64 mysize, MyStructure* data, ZXErrStack *err_stack) {...}
};
```
This method would report errors using the returned `ZXStatus` and the stack passed in as `ZXErrStack`. It will initialize the passed in `MyStructure* data`.

We generally don't define a deinit function because we allocate memory using arena allocation pattern. BNut some structures do need it, for example a structure that
holds an io_uring instance will need to do system calls at init and deinit functions to allocate/free system resources relating to the io_uring instance.

A deinit function might look like this:
```c++
struct MyStructure {
    static inline ZXStatus deinit(MyStructure* data, ZXErrStack *err_stack) {...}
};
```
This method will release resources relating to the structure and it will set status of passed in `MyStruct* data` to some "zero" value.
A typical "zero" value for a pointer is `nullptr`.

# Project Structure

- Source files are in `include` and `src` directories.
- `include` is for putting functionality that is `public` or for functionality that is intended to be inlined into
user's code. Only header `.h` files are 

# Documentation

- Documentation 
