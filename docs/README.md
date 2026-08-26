# Luma

*A low-level compiled language for people who want C's control without giving up their afternoon to memory bugs.*

<p align="center">
  <img src="../assets/luma.png" alt="Luma Logo" width="160"/>
</p>

[Why?](#why) • [Language Tour](#language-tour) • [Self-Hosted](#self-hosted) • [Getting Started](#getting-started) • [Join Us](#join-us)

---

## Introduction

Luma is a systems programming language built around one bet: you can catch most of the memory bugs that matter at compile time without a borrow checker, without lifetimes, and without a garbage collector.

Memory management stays manual. You call `alloc()` and `free()` yourself, same as C. What's different is that the compiler watches you do it. As part of type checking, Luma's static analyzer tracks ownership through your code and flags use-after-free, double-frees, and leaked allocations before you ever get to run the program.

Luma doesn't pretend to be memory-safe. Out-of-bounds access, uninitialized reads, and raw pointer misuse are all still on you. What it does promise is that the specific, common mistake of losing track of an allocation gets caught early, for free, with syntax you can read at a glance.

---

## Why?

Most languages ask you to pick a side: manual memory management with C, or safety with something that takes memory management away from you. Luma's trying to split the difference a little differently:

- Ownership hints instead of lifetimes. Annotate a function with `#returns_ownership` or `#takes_ownership` and the analyzer understands who's responsible for freeing what, no lifetime syntax anywhere in your code.
- No runtime cost. All of this happens at compile time. There's no garbage collector, no reference counting, nothing running behind your back.
- Small, direct syntax. Control flow and memory operations are always visible in the source, nothing is implicit.

It's not trying to out-safety Rust. It's trying to give you most of the "wait, did I free that?" coverage without asking you to learn an entirely new mental model to get it.

---

## Language Tour

A few of the things Luma actually looks like, pulled straight from the test suite so they're guaranteed to compile.

**Structs and methods:**

```lx
const Point -> struct {
pub:
    x: float,
    y: float,

    distance_to -> fn (other: Point) float {
        let dx: float = other.x - self.x;
        let dy: float = other.y - self.y;
        return sqrt(dx * dx + dy * dy);
    },
};
```

**Struct embedding**, for when composition beats inheritance-shaped code:

```lx
const Entity -> struct {
pub:
    x: float,
    y: float,
    move -> fn (dx: float, dy: float) void {
        self.x = self.x + dx;
        self.y = self.y + dy;
    },
};

const Player -> struct {
pub:
    ...Entity,
    health: int,
};

let p: Player = Player { x: 0.0, y: 0.0, health: 100 };
p.move(1.0, 1.0); // promoted straight from Entity
```

**Scoped `switch`**, so you're not writing `Color::Red` on every arm:

```lx
switch using Color (c) {
    Red   -> { output("stop\n"); }
    Green -> { output("go\n"); }
}
```

**Ownership annotations**, so the analyzer knows what a function does with what you hand it:

```lx
#returns_ownership
const make_counter -> fn () *int {
    let p: *int = cast<*int>(alloc(sizeof<int>));
    *p = 0;
    return p;
}

#takes_ownership
const print_and_free -> fn (counter: *int) void {
    output(*counter, "\n");
    free(counter);
}
```

**FFI**, straight to a C library, no bindings generator involved:

```lx
@link("libc.so.6")

pub const malloc -> fn (size: int) *void;
```

That's a small slice. The full language reference is in [`docs/docs.md`](https://luma-website-mu.vercel.app/html/docs.html).

---

## Self-Hosted

The compiler is written in Luma. It compiles itself, and every commit proves it can: an existing `luma` binary builds the current compiler source, that output builds the same source again, and the two results are diffed byte-for-byte. If a compiler can't reproduce itself exactly from its own source, something's wrong, so that check runs before anything else does, on every push and every PR.

Releases are cross-compiled from that same self-hosted compiler: a single Linux CI run produces Linux, Windows, and macOS binaries, and the Windows/macOS ones actually get downloaded and executed on real runners before a release goes out. See [`docs/releases/`](releases/) for what's shipped and when.

---

## Project Status

Latest release: **[v0.3.4](releases/v0.3.4.md)**

**What's working:**

- Full lexer, parser, type checker, and C-transpiling codegen, self-hosted
- Static ownership analysis: use-after-free, double-free, and leak detection
- Structs, enums, struct embedding, static methods, scoped `switch`
- FFI via `@link` (any C/POSIX shared library) and `#dll_import` (Windows DLLs)
- A language server (`luma --lsp`) with diagnostics, hover, and completion
- Cross-platform builds for Linux, Windows, and macOS, verified in CI

**What's not there yet:** generics, and a few rough edges in the static analyzer around conditional allocation paths. See the Known Limitations section of the [latest release notes](releases/v0.3.4.md) for the current honest list.

---

## Getting Started

Building from source just needs a C compiler no LLVM, no Meson, nothing else to install first:

```bash
git clone https://github.com/Luma-Programming-Language/Luma.git
cd Luma

./scripts/bootstrap-build.sh
sudo ./scripts/install.sh
```

`bootstrap-build.sh` handles the chicken-and-egg problem of a self-hosted compiler for you: it ships with a prebuilt seed binary, uses it to build the current source, then rebuilds itself with its own output and checks the two match before calling it done.

Prefer a prebuilt binary? Grab one from the [latest release](releases/v0.3.4.md) instead.

### Hello, World

```lx
@module "main"

pub const main -> fn () int {
    output("Hello, World!\n");
    return 0;
}
```

```bash
$ luma hello.lx -name hello
$ ./hello
Hello, World!
```

### Cross-compiling

`luma` can target Windows and macOS from Linux directly, as long as [`zig`](https://ziglang.org) is on your `PATH` (it's the C toolchain that actually builds the foreign binary):

```bash
luma main.lx -t windows64 -name main.exe
luma main.lx -t macos     -name main
```

---

## Join Us

- [GitHub repository](https://github.com/Luma-Programming-Language/Luma)
- [Luma Website](https://luma-website-mu.vercel.app/index.html)
- [Discord community](https://discord.gg/gqnwasvqd9)
- [Language reference](docs.md)
- [Contributing guidelines](CONTRIBUTING.md)

---

<p align="center">
  <strong>Built with ❤️ by the Luma community</strong>
</p>

