# Luma v0.2.0 – Cross-Platform, FFI & First-Class Functions

Luma is a systems programming language focused on explicit memory management, fast compilation, and compile-time ownership verification without a borrow checker or garbage collector.

Version 0.2.0 is the largest release since Luma's initial launch, introducing foreign function support, cross-platform development capabilities, first-class function types, significant compiler improvements, and expanded tooling.

---

# What's New

## Foreign Function Interface (FFI)

Luma can now call native libraries directly, opening access to existing C libraries and operating system APIs.

### `#lib_import`

Link and call functions from shared libraries:

```lx
@link("libc.so.6")

#lib_import("libc")
pub const printf -> fn (fmt: *byte, val: *byte) int;

#lib_import("libc")
pub const malloc -> fn (size: int) *void;
```

Features:

* Direct access to native libraries
* No wrapper code required
* No generated bindings required
* Simple declaration syntax

The standard library now includes `std/libc.lx` with bindings for common libc functionality including stdio, stdlib, string, math, and time APIs.

### `#dll_import`

Import functions directly from Windows DLLs:

```lx
#dll_import("kernel32.dll", callconv: "stdcall")
pub const CreateFileA -> fn (...) *void;
```

Features:

* Direct Windows API access
* Support for multiple calling conventions
* Foundation for native Windows development

---

## First-Class Function Types

Functions are now real types within the language.

```lx
let op: fn (int, int) int = add;

let result: int = op(5, 3);
```

Function types can be used for:

* Callbacks
* Higher-order APIs
* Thread entry points
* Native library integration

Example:

```lx
pub const apply -> fn (
    callback: fn (int) int,
    value: int
) int {
    return callback(value);
}
```

Features:

* Function type declarations using `fn (...) return_type`
* Functions may be assigned to variables
* Functions may be passed as arguments
* Compatible with native callback APIs
* Implicit compatibility between `fn(...)` and `*fn(...)`

---

## Cross-Platform Support

Luma now supports the following operating systems and architectures, detected automatically at compile time:

* Linux
* Windows (32-bit and 64-bit)
* macOS (Intel and ARM)
* iOS
* Android
* FreeBSD
* NetBSD
* OpenBSD
* DragonFly BSD
* Generic Unix

### `@os` Directive

Write platform-specific code directly in the language:

```lx
@os linux {
    "linux"   -> {}
    "macOS"   -> {}
    "windows" -> {}
}
```

Features:

* Platform-specific implementations
* Compile-time selection
* Cleaner standard library abstractions
* Reduced platform boilerplate

---

# Performance Improvements

### Parallel Module Compilation

Large projects now compile significantly faster through parallel object generation.

Highlights:

* Automatic CPU core detection
* Multi-threaded module compilation
* Reduced LLVM verification overhead

Observed improvements:

```text
18.33s → 2.78s
```

Up to a 6.6× speedup on larger projects.

### Compiler Optimizations

Additional improvements include:

* Faster module lookup
* LLVM code generation cleanup
* Reduced allocation overhead
* Improved internal hashing and lookup performance

---

# Standard Library

## New Modules

### `std/libc.lx`

Provides bindings for:

* stdio
* stdlib
* string
* math
* time
* socket APIs

### `std/thread.lx`

POSIX thread support including:

* pthread_create
* pthread_join
* synchronization primitives

### `std/args.lx`

Command-line argument handling utilities.

### `std/win32.lx`

Windows API bindings for:

* kernel32
* msvcrt

---

## Library Improvements

* Binary file read/write helpers added
* Vector library expanded with iterator support
* Improved allocator implementation
* Safe allocation wrappers added internally
* Printing support improved
* Exponent (`**`) and modulo (`%`) operators added

---

# Language Server Improvements

The Luma Language Server received major stability and usability improvements.

Highlights:

* Improved diagnostics
* Better semantic highlighting
* Hover type information
* Improved standard library path resolution
* Better code completion
* Cleaner error reporting

LSP support is now significantly more reliable than previous releases.

---

# Documentation

Documentation generation has been expanded and improved.

Features:

* Better formatting
* Module documentation support
* Struct and enum documentation
* Function documentation
* Attribute documentation
* Improved generated output

---

# Compiler & Analyzer Improvements

Numerous improvements were made across the parser, type checker, static analyzer, and LLVM backend.

Highlights include:

* Improved ownership tracking
* Better module resolution
* Improved struct method handling
* Correct handling of function pointer code generation
* More reliable Windows and macOS support
* Reduced analyzer false positives
* Improved expression and type handling

---

# Build System

Luma now uses Meson.

Previous:

```bash
make
sudo ./install.sh
```

New:

```bash
meson setup build
ninja -C build
sudo ./install.sh
```

Benefits:

* Improved cross-platform support
* Cleaner build configuration
* Better dependency management
* Easier maintenance

---

# Breaking Changes

⚠️ Build system migrated from Make to Meson.

Projects building Luma from source should update their build workflow accordingly.

---

# Installation

Pre-built binaries are available for:

* Linux x86_64
* Windows x86_64
* macOS x86_64
* macOS ARM64

Build from source:

```bash
git clone https://github.com/Luma-Programming-Language/Luma.git
cd luma
git checkout v0.2.0

meson setup build
ninja -C build

sudo ./install.sh
```

Requirements:

* LLVM 15+
* Meson
* Ninja
* GCC or Clang

---

# Known Limitations

### macOS

* Some terminal output behavior may vary depending on terminal configuration
* System-call abstraction layer remains Linux-focused

### Windows

* `std/win32.lx` currently covers only a subset of available APIs

### Function Types

* Closures and captures are not yet supported
* Additional callback-related features are planned

### Static Analyzer

* Conditional allocation paths may still produce false positives
* Array-of-pointer ownership tracking remains limited

### Language Features

* Generics are not yet implemented

---

# Community

Issues:
https://github.com/Luma-Programming-Language/Luma/issues

Discord:
https://discord.gg/gqnwasvqd9

---

Made with care by the Luma Team