# Module: std_thread

*Source: `std/thread.lx`*

POSIX threading bindings for Linux

PLATFORM: Linux x86_64

Wraps libpthread — pthread_t, pthread_mutex_t, and pthread_cond_t
are all treated as i64 (opaque handles).

## Table of Contents

- [Functions](#functions)
- [Variables](#variables)
- [Linked Libraries](#linked-libraries)


## Functions

### `pthread_create`

```luma
pub pthread_create -> fn(
    tid: *i64,
    attr: *void,
    _fn: fn(*void) *void,
    arg: *void
) i64
```

### `pthread_join`

```luma
pub pthread_join -> fn(
    tid: i64,
    retval: **void
) i64
```

### `pthread_detach`

```luma
pub pthread_detach -> fn(
    tid: i64
) i64
```

### `pthread_self`

```luma
pub pthread_self -> fn(
) i64
```

### `pthread_exit`

```luma
pub pthread_exit -> fn(
    retval: *void
) void
```

### `pthread_equal`

```luma
pub pthread_equal -> fn(
    t1: i64,
    t2: i64
) i64
```

### `pthread_cancel`

```luma
pub pthread_cancel -> fn(
    tid: i64
) i64
```

### `pthread_mutex_init`

```luma
pub pthread_mutex_init -> fn(
    mutex: *i64,
    attr: *void
) i64
```

### `pthread_mutex_destroy`

```luma
pub pthread_mutex_destroy -> fn(
    mutex: *i64
) i64
```

### `pthread_mutex_lock`

```luma
pub pthread_mutex_lock -> fn(
    mutex: *i64
) i64
```

### `pthread_mutex_trylock`

```luma
pub pthread_mutex_trylock -> fn(
    mutex: *i64
) i64
```

### `pthread_mutex_unlock`

```luma
pub pthread_mutex_unlock -> fn(
    mutex: *i64
) i64
```

### `pthread_cond_init`

```luma
pub pthread_cond_init -> fn(
    cond: *i64,
    attr: *void
) i64
```

### `pthread_cond_destroy`

```luma
pub pthread_cond_destroy -> fn(
    cond: *i64
) i64
```

### `pthread_cond_wait`

```luma
pub pthread_cond_wait -> fn(
    cond: *i64,
    mutex: *i64
) i64
```

### `pthread_cond_signal`

```luma
pub pthread_cond_signal -> fn(
    cond: *i64
) i64
```

### `pthread_cond_broadcast`

```luma
pub pthread_cond_broadcast -> fn(
    cond: *i64
) i64
```

### `pthread_rwlock_init`

```luma
pub pthread_rwlock_init -> fn(
    rwlock: *i64,
    attr: *void
) i64
```

### `pthread_rwlock_destroy`

```luma
pub pthread_rwlock_destroy -> fn(
    rwlock: *i64
) i64
```

### `pthread_rwlock_rdlock`

```luma
pub pthread_rwlock_rdlock -> fn(
    rwlock: *i64
) i64
```

### `pthread_rwlock_wrlock`

```luma
pub pthread_rwlock_wrlock -> fn(
    rwlock: *i64
) i64
```

### `pthread_rwlock_unlock`

```luma
pub pthread_rwlock_unlock -> fn(
    rwlock: *i64
) i64
```

### `pthread_once`

```luma
pub pthread_once -> fn(
    once: *i64,
    _fn: fn() void
) i64
```

### `pthread_barrier_init`

```luma
pub pthread_barrier_init -> fn(
    barrier: *i64,
    attr: *void,
    count: i64
) i64
```

### `pthread_barrier_destroy`

```luma
pub pthread_barrier_destroy -> fn(
    barrier: *i64
) i64
```

### `pthread_barrier_wait`

```luma
pub pthread_barrier_wait -> fn(
    barrier: *i64
) i64
```


## Variables

- **`PTHREAD_T_SIZE`** : i64 *(const)*
- **`PTHREAD_MUTEX_SIZE`** : i64 *(const)*
- **`PTHREAD_COND_SIZE`** : i64 *(const)*
- **`PTHREAD_RWLOCK_SIZE`** : i64 *(const)*
- **`PTHREAD_BARRIER_SIZE`** : i64 *(const)*
- **`PTHREAD_ONCE_SIZE`** : i64 *(const)*

## Linked Libraries

External native libraries linked by this module.

> **FFI library:** `libpthread.so.0`
>

