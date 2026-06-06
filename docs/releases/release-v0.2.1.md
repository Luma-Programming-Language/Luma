# Luma v0.2.1

Bugfix release with AST printer improvements and LLVM bundling for standalone distribution.

---

# What's Changed

## AST Printer Rewrite

The AST printer has been rewritten with proper tree connectors (`├──`, `└──`, `│`) using a flags array that tracks sibling state at each depth level. Output is now visually structured like a real tree.

Added:
- **ANSI color** — cyan for node types, blue for type nodes, green for param/field names, yellow for operators, magenta for annotations, dim for line:col/counts
- **Line:col annotations** — every node now prints its source position (e.g. `(5:18)`)
- **Node counts** — e.g. `Block (7 stmts)`, `Call (2 args)`

## LLVM Bundling

The release tarball now bundles `libLLVM.so` alongside the `luma` binary using `$ORIGIN` rpath. Users no longer need LLVM installed on their system — just download, untar, and run.

Details:
- `meson.build`: Added `-Wl,-rpath,$ORIGIN` link flag (Linux) / `@executable_path` (macOS)
- `release.yml`: Packaging step copies the LLVM shared library via `ldd`/`otool -L` into the release tarball
- Release archive now contains: `luma`, `libLLVM-20.so.1`, `std/`, README, INSTALL

## Fix

- `tests/threads.lx`: Fixed `pthread_join` argument type from `*void` to `**void`

---

# Installation

```bash
# Download luma-v0.2.1-linux-x86_64.tar.gz from GitHub Releases
tar xzf luma-v0.2.1-linux-x86_64.tar.gz
cd luma-v0.2.1-linux-x86_64
./luma --help
```

Build from source:
```bash
git clone https://github.com/Luma-Programming-Language/Luma.git
cd Luma
git checkout v0.2.1

meson setup build
ninja -C build

sudo ./install.sh
```

Requirements:
- LLVM 15+ (for source builds only — pre-built binaries bundle LLVM)
- Meson
- Ninja
- GCC or Clang
