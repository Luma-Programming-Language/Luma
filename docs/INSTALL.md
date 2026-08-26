# Installing Luma

Luma is a self-hosted compiler — it transpiles to C and just needs a C compiler to build. No LLVM, no Meson, nothing else to install first.

## Quick Start (prebuilt binary)

Grab the archive for your platform from the [latest release](releases/v0.3.4.md).

### Linux / macOS

```bash
tar -xzf luma-v0.3.4-linux-x86_64.tar.gz    # or luma-v0.3.4-macos-x86_64.tar.gz
cd luma-v0.3.4-linux-x86_64

sudo ./install.sh   # system-wide, requires sudo
# or
./install.sh        # user-local install, no sudo needed
```

### Windows

1. Extract `luma-v0.3.4-windows-x86_64.zip`.
2. Run `install.bat` — as Administrator for a system-wide install, or without for a user-local one.

## Building from Source

```bash
git clone https://github.com/Luma-Programming-Language/Luma.git
cd Luma

./scripts/bootstrap-build.sh
sudo ./scripts/install.sh
```

`bootstrap-build.sh` solves the chicken-and-egg problem of a self-hosted compiler for you: the repo ships a working seed binary (`bootstrap/luma-seed`), which builds `bin/luma` from the current source, which then rebuilds itself from that same source and checks the two outputs match byte-for-byte before calling it done. All it needs on your machine is `cc`.

### Cross-compiling

`luma` can target another OS directly, as long as [`zig`](https://ziglang.org) is on your `PATH` — it's the actual C toolchain used to build the foreign binary:

```bash
./scripts/cross-build.sh windows64 dist/luma
./scripts/cross-build.sh macos     dist/luma
```

---

## Standard Library Paths

When you pass a std lib file — via `-l std/whatever.lx`, or a linked file resolved the same way — `luma` looks for it in this order, first match wins:

1. **Exactly as given**, relative to your current directory (or as an absolute path). A local `./std/` next to your source always wins over anything installed.
2. **User-local install**: `~/.luma/std/` (Linux/macOS) or `%USERPROFILE%\.luma\std\` (Windows) — where `install.sh`/`install.bat` put things when run without admin/sudo.
3. **System-wide install**: `/usr/local/lib/luma/std/` (Linux/macOS) or `C:\Program Files\luma\std\` (Windows).

If none of the three has the file, you'll get a "module not found — was its file passed with -l?" error.

---

## Manual Installation

If you'd rather not run the installer script:

### Linux / macOS

**System-wide:**

```bash
sudo mkdir -p /usr/local/bin /usr/local/lib/luma/std
sudo cp luma /usr/local/bin/
sudo cp -r std/* /usr/local/lib/luma/std/
sudo chmod +x /usr/local/bin/luma
```

**User-local:**

```bash
mkdir -p ~/.local/bin ~/.luma/std
cp luma ~/.local/bin/
cp -r std/* ~/.luma/std/

# add to ~/.bashrc or ~/.zshrc if it's not already on your PATH
export PATH="$PATH:$HOME/.local/bin"
```

### Windows

1. Create `C:\Program Files\luma\bin` and `\std` (system-wide) or `%USERPROFILE%\.luma\bin` and `\std` (user-local).
2. Copy `luma.exe` into the `bin` directory.
3. Copy the contents of `std/` into the `std` directory.
4. Add the `bin` directory to your `PATH` environment variable.

---

## Verifying Installation

```bash
luma --version
```

```
Luma Compiler v0.3.4
```

---

## Troubleshooting

**"module not found — was its file passed with -l?"**

Either the file wasn't passed with `-l` at all, or it's not sitting in any of the three [Standard Library Paths](#standard-library-paths) tiers above. Double-check the install actually landed where you expect (`ls ~/.luma/std/` or `ls /usr/local/lib/luma/std/`), and that the path you're passing to `-l` matches what's actually on disk relative to your current directory.

**`luma: command not found` (Linux/macOS)**

The install directory isn't on your `PATH`. Add whichever one you used:

```bash
export PATH="$PATH:$HOME/.local/bin"        # user-local install
export PATH="$PATH:/usr/local/bin"          # system-wide install (usually already on PATH)
```

**PATH issues (Windows)**

Search "Environment Variables" in the Start menu, edit your PATH, and add `%USERPROFILE%\.luma\bin` or `C:\Program Files\luma\bin`, then restart your terminal.

---

## Uninstalling

### Linux / macOS

```bash
# system-wide
sudo rm /usr/local/bin/luma
sudo rm -rf /usr/local/lib/luma

# user-local
rm ~/.local/bin/luma
rm -rf ~/.luma
```

### Windows

Delete the install directory (`C:\Program Files\luma` or `%USERPROFILE%\.luma`) and remove it from `PATH`.

---

## Support

- [GitHub Issues](https://github.com/Luma-Programming-Language/Luma/issues)
- [Language reference](docs.md)
- [Discord](https://discord.gg/gqnwasvqd9)

---

Luma is licensed under the MIT License.
