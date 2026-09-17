# Module: std_sys

*Source: `std/sys.lx`*

sys.lx - Unix System Call Interface

PLATFORM: Linux x86_64 and macOS x86_64/ARM64 (via @os blocks)

WARNING: This code will NOT work on:
  - Windows (completely different system call mechanism)
  - ARM/ARM64 Linux (different syscall numbers)
  - 32-bit x86 Linux (different syscall numbers and calling convention)

macOS note: macOS wraps syscalls through libSystem. Direct syscall usage
via the syscall instruction is officially unsupported by Apple and may
break across OS updates. The numbers here reflect the BSD-derived kernel
interface (class 2, i.e. number | 0x2000000) as used internally.
For production macOS code, prefer libc wrappers.

## Table of Contents

- [Variables](#variables)
- [OS-Specific](#os-specific)


## Variables

- **`O_RDONLY`** : i64 *(const)* — FILE FLAGS
- **`O_WRONLY`** : i64 *(const)*
- **`O_RDWR`** : i64 *(const)*
- **`S_IRWXU`** : i64 *(const)* — FILE PERMISSIONS  (POSIX — identical on both platforms)
- **`S_IRUSR`** : i64 *(const)*
- **`S_IWUSR`** : i64 *(const)*
- **`S_IXUSR`** : i64 *(const)*
- **`S_IRWXG`** : i64 *(const)*
- **`S_IRGRP`** : i64 *(const)*
- **`S_IWGRP`** : i64 *(const)*
- **`S_IXGRP`** : i64 *(const)*
- **`S_IRWXO`** : i64 *(const)*
- **`S_IROTH`** : i64 *(const)*
- **`S_IWOTH`** : i64 *(const)*
- **`S_IXOTH`** : i64 *(const)*
- **`MODE_0644`** : i64 *(const)*
- **`MODE_0755`** : i64 *(const)*
- **`MODE_0777`** : i64 *(const)*
- **`SEEK_SET`** : i64 *(const)* — LSEEK WHENCE  (POSIX — identical on both platforms)
- **`SEEK_CUR`** : i64 *(const)*
- **`SEEK_END`** : i64 *(const)*
- **`STDIN`** : i64 *(const)* — STANDARD FILE DESCRIPTORS  (identical everywhere)
- **`STDOUT`** : i64 *(const)*
- **`STDERR`** : i64 *(const)*
- **`PROT_NONE`** : i64 *(const)* — MMAP PROTECTION FLAGS  (POSIX — identical on both platforms)
- **`PROT_READ`** : i64 *(const)*
- **`PROT_WRITE`** : i64 *(const)*
- **`PROT_EXEC`** : i64 *(const)*
- **`MAP_SHARED`** : i64 *(const)* — MMAP FLAGS
- **`MAP_PRIVATE`** : i64 *(const)*
- **`MAP_FIXED`** : i64 *(const)*
- **`SIGHUP`** : i64 *(const)* — SIGNALS  (mostly POSIX, but SIGBUS/SIGCHLD positions differ slightly)
- **`SIGINT`** : i64 *(const)*
- **`SIGQUIT`** : i64 *(const)*
- **`SIGILL`** : i64 *(const)*
- **`SIGTRAP`** : i64 *(const)*
- **`SIGABRT`** : i64 *(const)*
- **`SIGFPE`** : i64 *(const)*
- **`SIGKILL`** : i64 *(const)*
- **`SIGSEGV`** : i64 *(const)*
- **`SIGPIPE`** : i64 *(const)*
- **`SIGALRM`** : i64 *(const)*
- **`SIGTERM`** : i64 *(const)*
- **`WNOHANG`** : i64 *(const)* — WAIT FLAGS  (identical on both platforms)
- **`WUNTRACED`** : i64 *(const)*
- **`EPERM`** : i64 *(const)* — ERRNO VALUES  (POSIX — same numbers on both platforms for these common ones)
- **`ENOENT`** : i64 *(const)*
- **`ESRCH`** : i64 *(const)*
- **`EINTR`** : i64 *(const)*
- **`EIO`** : i64 *(const)*
- **`ENXIO`** : i64 *(const)*
- **`E2BIG`** : i64 *(const)*
- **`EBADF`** : i64 *(const)*
- **`ECHILD`** : i64 *(const)*
- **`EAGAIN`** : i64 *(const)*
- **`ENOMEM`** : i64 *(const)*
- **`EACCES`** : i64 *(const)*
- **`EFAULT`** : i64 *(const)*
- **`EBUSY`** : i64 *(const)*
- **`EEXIST`** : i64 *(const)*
- **`ENODEV`** : i64 *(const)*
- **`ENOTDIR`** : i64 *(const)*
- **`EISDIR`** : i64 *(const)*
- **`EINVAL`** : i64 *(const)*
- **`ENFILE`** : i64 *(const)*
- **`EMFILE`** : i64 *(const)*
- **`ENOSPC`** : i64 *(const)*
- **`EPIPE`** : i64 *(const)*

## OS-Specific

### `"linux"`

- **`SYS_READ`** : i64 *(const)*
- **`SYS_WRITE`** : i64 *(const)*
- **`SYS_OPEN`** : i64 *(const)*
- **`SYS_CLOSE`** : i64 *(const)*
- **`SYS_STAT`** : i64 *(const)*
- **`SYS_FSTAT`** : i64 *(const)*
- **`SYS_LSTAT`** : i64 *(const)*
- **`SYS_LSEEK`** : i64 *(const)*
- **`SYS_MMAP`** : i64 *(const)*
- **`SYS_MUNMAP`** : i64 *(const)*
- **`SYS_BRK`** : i64 *(const)*
- **`SYS_IOCTL`** : i64 *(const)*
- **`SYS_PREAD`** : i64 *(const)*
- **`SYS_PWRITE`** : i64 *(const)*
- **`SYS_PIPE`** : i64 *(const)*
- **`SYS_SELECT`** : i64 *(const)*
- **`SYS_DUP`** : i64 *(const)*
- **`SYS_DUP2`** : i64 *(const)*
- **`SYS_GETPID`** : i64 *(const)*
- **`SYS_FORK`** : i64 *(const)*
- **`SYS_EXECVE`** : i64 *(const)*
- **`SYS_EXIT`** : i64 *(const)*
- **`SYS_WAIT4`** : i64 *(const)*
- **`SYS_KILL`** : i64 *(const)*
- **`SYS_FCNTL`** : i64 *(const)*
- **`SYS_GETCWD`** : i64 *(const)*
- **`SYS_CHDIR`** : i64 *(const)*
- **`SYS_MKDIR`** : i64 *(const)*
- **`SYS_RMDIR`** : i64 *(const)*
- **`SYS_UNLINK`** : i64 *(const)*
- **`SYS_GETUID`** : i64 *(const)*
- **`SYS_GETGID`** : i64 *(const)*
- **`SYS_GETTIMEOFDAY`** : i64 *(const)*
- **`SYS_CLOCK_GETTIME`** : i64 *(const)*

### `"macos"`

- **`SYS_READ`** : i64 *(const)* — macOS BSD syscall table (x86_64)
- **`SYS_WRITE`** : i64 *(const)*
- **`SYS_OPEN`** : i64 *(const)*
- **`SYS_CLOSE`** : i64 *(const)*
- **`SYS_STAT`** : i64 *(const)*
- **`SYS_FSTAT`** : i64 *(const)*
- **`SYS_LSTAT`** : i64 *(const)*
- **`SYS_LSEEK`** : i64 *(const)*
- **`SYS_MMAP`** : i64 *(const)*
- **`SYS_MUNMAP`** : i64 *(const)*
- **`SYS_BRK`** : i64 *(const)*
- **`SYS_IOCTL`** : i64 *(const)*
- **`SYS_PREAD`** : i64 *(const)*
- **`SYS_PWRITE`** : i64 *(const)*
- **`SYS_PIPE`** : i64 *(const)*
- **`SYS_SELECT`** : i64 *(const)*
- **`SYS_DUP`** : i64 *(const)*
- **`SYS_DUP2`** : i64 *(const)*
- **`SYS_GETPID`** : i64 *(const)*
- **`SYS_FORK`** : i64 *(const)*
- **`SYS_EXECVE`** : i64 *(const)*
- **`SYS_EXIT`** : i64 *(const)*
- **`SYS_WAIT4`** : i64 *(const)*
- **`SYS_KILL`** : i64 *(const)*
- **`SYS_FCNTL`** : i64 *(const)*
- **`SYS_GETCWD`** : i64 *(const)*
- **`SYS_CHDIR`** : i64 *(const)*
- **`SYS_MKDIR`** : i64 *(const)*
- **`SYS_RMDIR`** : i64 *(const)*
- **`SYS_UNLINK`** : i64 *(const)*
- **`SYS_GETUID`** : i64 *(const)*
- **`SYS_GETGID`** : i64 *(const)*
- **`SYS_GETTIMEOFDAY`** : i64 *(const)*
- **`SYS_CLOCK_GETTIME`** : i64 *(const)*

### `"windows64"`

- **`SYS_READ`** : i64 *(const)*
- **`SYS_WRITE`** : i64 *(const)*
- **`SYS_OPEN`** : i64 *(const)*
- **`SYS_CLOSE`** : i64 *(const)*
- **`SYS_STAT`** : i64 *(const)*
- **`SYS_FSTAT`** : i64 *(const)*
- **`SYS_LSTAT`** : i64 *(const)*
- **`SYS_LSEEK`** : i64 *(const)*
- **`SYS_MMAP`** : i64 *(const)*
- **`SYS_MUNMAP`** : i64 *(const)*
- **`SYS_BRK`** : i64 *(const)*
- **`SYS_IOCTL`** : i64 *(const)*
- **`SYS_PREAD`** : i64 *(const)*
- **`SYS_PWRITE`** : i64 *(const)*
- **`SYS_PIPE`** : i64 *(const)*
- **`SYS_SELECT`** : i64 *(const)*
- **`SYS_DUP`** : i64 *(const)*
- **`SYS_DUP2`** : i64 *(const)*
- **`SYS_GETPID`** : i64 *(const)*
- **`SYS_FORK`** : i64 *(const)*
- **`SYS_EXECVE`** : i64 *(const)*
- **`SYS_EXIT`** : i64 *(const)*
- **`SYS_WAIT4`** : i64 *(const)*
- **`SYS_KILL`** : i64 *(const)*
- **`SYS_FCNTL`** : i64 *(const)*
- **`SYS_GETCWD`** : i64 *(const)*
- **`SYS_CHDIR`** : i64 *(const)*
- **`SYS_MKDIR`** : i64 *(const)*
- **`SYS_RMDIR`** : i64 *(const)*
- **`SYS_UNLINK`** : i64 *(const)*
- **`SYS_GETUID`** : i64 *(const)*
- **`SYS_GETGID`** : i64 *(const)*
- **`SYS_GETTIMEOFDAY`** : i64 *(const)*
- **`SYS_CLOCK_GETTIME`** : i64 *(const)*

### `"linux"`

- **`O_CREAT`** : i64 *(const)*
- **`O_EXCL`** : i64 *(const)*
- **`O_NOCTTY`** : i64 *(const)*
- **`O_TRUNC`** : i64 *(const)*
- **`O_APPEND`** : i64 *(const)*
- **`O_NONBLOCK`** : i64 *(const)*
- **`O_DIRECTORY`** : i64 *(const)*
- **`O_CLOEXEC`** : i64 *(const)*

### `"macos"`

- **`O_CREAT`** : i64 *(const)*
- **`O_EXCL`** : i64 *(const)*
- **`O_NOCTTY`** : i64 *(const)*
- **`O_TRUNC`** : i64 *(const)*
- **`O_APPEND`** : i64 *(const)*
- **`O_NONBLOCK`** : i64 *(const)*
- **`O_DIRECTORY`** : i64 *(const)*
- **`O_CLOEXEC`** : i64 *(const)*

### `"windows64"`

- **`O_CREAT`** : i64 *(const)*
- **`O_EXCL`** : i64 *(const)*
- **`O_NOCTTY`** : i64 *(const)*
- **`O_TRUNC`** : i64 *(const)*
- **`O_APPEND`** : i64 *(const)*
- **`O_NONBLOCK`** : i64 *(const)*
- **`O_DIRECTORY`** : i64 *(const)*
- **`O_CLOEXEC`** : i64 *(const)*

### `"linux"`

- **`MAP_ANONYMOUS`** : i64 *(const)*
- **`MAP_ANON`** : i64 *(const)*

### `"macos"`

- **`MAP_ANONYMOUS`** : i64 *(const)*
- **`MAP_ANON`** : i64 *(const)*

### `"windows64"`

- **`MAP_ANONYMOUS`** : i64 *(const)*
- **`MAP_ANON`** : i64 *(const)*

### `"linux"`

- **`SIGBUS`** : i64 *(const)*
- **`SIGCHLD`** : i64 *(const)*

### `"macos"`

- **`SIGBUS`** : i64 *(const)*
- **`SIGCHLD`** : i64 *(const)*

### `"windows64"`

- **`SIGBUS`** : i64 *(const)*
- **`SIGCHLD`** : i64 *(const)*

### `"linux"`

### `exit`

PROCESS MANAGEMENT
Terminate the current process with the given exit code.

```luma
pub exit -> fn(
    code: i64
) void
```

### `fork`

Fork the current process. Returns the child PID in the parent, 0 in the child.

```luma
pub fork -> fn(
) i64
```

### `getpid`

Return the PID of the calling process.

```luma
pub getpid -> fn(
) i64
```

### `getuid`

Return the real user ID of the calling process.

```luma
pub getuid -> fn(
) i64
```

### `getgid`

Return the real group ID of the calling process.

```luma
pub getgid -> fn(
) i64
```

### `kill`

Send signal `sig` to process `pid`.

```luma
pub kill -> fn(
    pid: i64,
    sig: i64
) i64
```

### `wait4`

Wait for a child process. Stores exit status in `status` if non-null.

```luma
pub wait4 -> fn(
    pid: i64,
    status: *i64,
    options: i64,
    rusage: *void
) i64
```

### `execve`

Replace the current process image with a new one.

```luma
pub execve -> fn(
    path: *byte,
    argv: **byte,
    envp: **byte
) i64
```

### `read`

FILE OPERATIONS
Read up to `count` bytes from `fd` into `buf`. Returns bytes read or a negative errno.

```luma
pub read -> fn(
    fd: i64,
    buf: *void,
    count: i64
) i64
```

### `write`

Write up to `count` bytes from `buf` to `fd`. Returns bytes written or a negative errno.

```luma
pub write -> fn(
    fd: i64,
    buf: *void,
    count: i64
) i64
```

### `open`

Open a file at `path` with the given `flags` and `mode`. Returns an fd or a negative errno.

```luma
pub open -> fn(
    path: *byte,
    flags: i64,
    mode: i64
) i64
```

### `close`

Close the file descriptor `fd`.

```luma
pub close -> fn(
    fd: i64
) i64
```

### `lseek`

Reposition the file offset of `fd`. Returns the new offset or a negative errno.

```luma
pub lseek -> fn(
    fd: i64,
    offset: i64,
    whence: i64
) i64
```

### `pread`

Read up to `count` bytes from `fd` at `offset` without changing the file position.

```luma
pub pread -> fn(
    fd: i64,
    buf: *void,
    count: i64,
    offset: i64
) i64
```

### `pwrite`

Write up to `count` bytes to `fd` at `offset` without changing the file position.

```luma
pub pwrite -> fn(
    fd: i64,
    buf: *void,
    count: i64,
    offset: i64
) i64
```

### `dup`

Duplicate file descriptor `oldfd`. Returns the new fd or a negative errno.

```luma
pub dup -> fn(
    oldfd: i64
) i64
```

### `dup2`

Duplicate `oldfd` to `newfd`, closing `newfd` first if it is open.

```luma
pub dup2 -> fn(
    oldfd: i64,
    newfd: i64
) i64
```

### `pipe`

Create a pipe. Writes the read and write fds into `pipefd[0]` and `pipefd[1]`.

```luma
pub pipe -> fn(
    pipefd: *i64
) i64
```

### `unlink`

Delete the file at `path`.

```luma
pub unlink -> fn(
    path: *byte
) i64
```

### `mkdir`

DIRECTORY OPERATIONS
Create a directory at `path` with the given `mode`.

```luma
pub mkdir -> fn(
    path: *byte,
    mode: i64
) i64
```

### `rmdir`

Remove the empty directory at `path`.

```luma
pub rmdir -> fn(
    path: *byte
) i64
```

### `chdir`

Change the working directory to `path`.

```luma
pub chdir -> fn(
    path: *byte
) i64
```

### `getcwd`

Get the current working directory into `buf`. Returns `buf` on success, null on failure.

```luma
pub getcwd -> fn(
    buf: *byte,
    size: i64
) *byte
```

### `brk`

MEMORY MANAGEMENT
Adjust the program break to `addr`. Returns the new break or a negative errno.

```luma
pub brk -> fn(
    addr: *void
) i64
```

### `mmap`

Map memory. Returns a pointer to the mapped region, or a negative errno cast to *void.

```luma
pub mmap -> fn(
    addr: *void,
    length: i64,
    prot: i64,
    flags: i64,
    fd: i64,
    offset: i64
) *void
```

### `munmap`

Unmap a previously mapped region.

```luma
pub munmap -> fn(
    addr: *void,
    length: i64
) i64
```

### `is_error`

HELPER FUNCTIONS
Returns true if `result` represents a syscall error (i.e. in the range [-4095, -1]).

```luma
pub is_error -> fn(
    result: i64
) bool
```

### `get_errno`

Extract the errno value from a failed syscall result. Returns 0 if not an error.

```luma
pub get_errno -> fn(
    result: i64
) i64
```

### `write_str`

Write a null-terminated string to `fd`. Returns bytes written or a negative errno.

```luma
pub write_str -> fn(
    fd: i64,
    s: *byte
) i64
```

### `eprint`

Write a null-terminated string to stderr.

```luma
pub eprint -> fn(
    s: *byte
) i64
```


### `"macos"`

### `exit`

PROCESS MANAGEMENT
Terminate the current process with the given exit code.

```luma
pub exit -> fn(
    code: i64
) void
```

### `fork`

Fork the current process. Returns the child PID in the parent, 0 in the child.

```luma
pub fork -> fn(
) i64
```

### `getpid`

Return the PID of the calling process.

```luma
pub getpid -> fn(
) i64
```

### `getuid`

Return the real user ID of the calling process.

```luma
pub getuid -> fn(
) i64
```

### `getgid`

Return the real group ID of the calling process.

```luma
pub getgid -> fn(
) i64
```

### `kill`

Send signal `sig` to process `pid`.

```luma
pub kill -> fn(
    pid: i64,
    sig: i64
) i64
```

### `wait4`

Wait for a child process. Stores exit status in `status` if non-null.

```luma
pub wait4 -> fn(
    pid: i64,
    status: *i64,
    options: i64,
    rusage: *void
) i64
```

### `execve`

Replace the current process image with a new one.

```luma
pub execve -> fn(
    path: *byte,
    argv: **byte,
    envp: **byte
) i64
```

### `read`

FILE OPERATIONS
Read up to `count` bytes from `fd` into `buf`. Returns bytes read or a negative errno.

```luma
pub read -> fn(
    fd: i64,
    buf: *void,
    count: i64
) i64
```

### `write`

Write up to `count` bytes from `buf` to `fd`. Returns bytes written or a negative errno.

```luma
pub write -> fn(
    fd: i64,
    buf: *void,
    count: i64
) i64
```

### `open`

Open a file at `path` with the given `flags` and `mode`. Returns an fd or a negative errno.

```luma
pub open -> fn(
    path: *byte,
    flags: i64,
    mode: i64
) i64
```

### `close`

Close the file descriptor `fd`.

```luma
pub close -> fn(
    fd: i64
) i64
```

### `lseek`

Reposition the file offset of `fd`. Returns the new offset or a negative errno.

```luma
pub lseek -> fn(
    fd: i64,
    offset: i64,
    whence: i64
) i64
```

### `pread`

Read up to `count` bytes from `fd` at `offset` without changing the file position.

```luma
pub pread -> fn(
    fd: i64,
    buf: *void,
    count: i64,
    offset: i64
) i64
```

### `pwrite`

Write up to `count` bytes to `fd` at `offset` without changing the file position.

```luma
pub pwrite -> fn(
    fd: i64,
    buf: *void,
    count: i64,
    offset: i64
) i64
```

### `dup`

Duplicate file descriptor `oldfd`. Returns the new fd or a negative errno.

```luma
pub dup -> fn(
    oldfd: i64
) i64
```

### `dup2`

Duplicate `oldfd` to `newfd`, closing `newfd` first if it is open.

```luma
pub dup2 -> fn(
    oldfd: i64,
    newfd: i64
) i64
```

### `pipe`

Create a pipe. Writes the read and write fds into `pipefd[0]` and `pipefd[1]`.

```luma
pub pipe -> fn(
    pipefd: *i64
) i64
```

### `unlink`

Delete the file at `path`.

```luma
pub unlink -> fn(
    path: *byte
) i64
```

### `mkdir`

DIRECTORY OPERATIONS
Create a directory at `path` with the given `mode`.

```luma
pub mkdir -> fn(
    path: *byte,
    mode: i64
) i64
```

### `rmdir`

Remove the empty directory at `path`.

```luma
pub rmdir -> fn(
    path: *byte
) i64
```

### `chdir`

Change the working directory to `path`.

```luma
pub chdir -> fn(
    path: *byte
) i64
```

### `getcwd`

Get the current working directory into `buf`. Returns `buf` on success, null on failure.

```luma
pub getcwd -> fn(
    buf: *byte,
    size: i64
) *byte
```

### `brk`

MEMORY MANAGEMENT
Adjust the program break to `addr`. Returns the new break or a negative errno.

```luma
pub brk -> fn(
    addr: *void
) i64
```

### `mmap`

Map memory. Returns a pointer to the mapped region, or a negative errno cast to *void.

```luma
pub mmap -> fn(
    addr: *void,
    length: i64,
    prot: i64,
    flags: i64,
    fd: i64,
    offset: i64
) *void
```

### `munmap`

Unmap a previously mapped region.

```luma
pub munmap -> fn(
    addr: *void,
    length: i64
) i64
```

### `is_error`

HELPER FUNCTIONS
Returns true if `result` represents a syscall error (i.e. in the range [-4095, -1]).

```luma
pub is_error -> fn(
    result: i64
) bool
```

### `get_errno`

Extract the errno value from a failed syscall result. Returns 0 if not an error.

```luma
pub get_errno -> fn(
    result: i64
) i64
```

### `write_str`

Write a null-terminated string to `fd`. Returns bytes written or a negative errno.

```luma
pub write_str -> fn(
    fd: i64,
    s: *byte
) i64
```

### `eprint`

Write a null-terminated string to stderr.

```luma
pub eprint -> fn(
    s: *byte
) i64
```


