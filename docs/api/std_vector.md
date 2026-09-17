# Module: std_vector

*Source: `std/vector.lx`*

Dynamic array (vector) implementation for Luma.

Provides a generic, resizable array container that can hold elements of any type.
The vector automatically grows when capacity is exceeded and provides efficient
random access, insertion, and removal operations.

# Example
```luma
let v: Vector = create_vector(sizeof<i64>);
defer free_vector(&v);

let x: i64 = 42;
v.push_back(cast<*void>(&x));
```

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)

---

## Structures

### `VectorIter`

| Field | Type | Description |
|-------|------|-------------|
| `data` | *void |  |
| `index` | i64 |  |
| `size` | i64 |  |
| `element_size` | i64 |  |

**Methods:**

#### `VectorIter::next()`

```luma
VectorIter::next -> fn(
) *void
```

### `Vector`

A dynamic array that can grow as needed.

Vector stores elements of a fixed size in contiguous memory.
All elements must be of the same type (specified by element_size).
The vector automatically reallocates when capacity is exceeded.


| Field | Type | Description |
|-------|------|-------------|
| `data` | *void |  |
| `capacity` | i64 | Pointer to contiguous data buffer |
| `size` | i64 | Maximum elements before resize |
| `element_size` | i64 | Current number of elements |

**Methods:**

#### `Vector::insert()`

```luma
Vector::insert -> fn(
    elem: *void,
    index: i64
) i64
```

#### `Vector::push_back()`

```luma
Vector::push_back -> fn(
    elem: *void
) void
```

#### `Vector::pop_back()`

```luma
Vector::pop_back -> fn(
    out: *void
) i64
```

#### `Vector::remove_at()`

```luma
Vector::remove_at -> fn(
    index: i64
) i64
```

#### `Vector::get()`

```luma
Vector::get -> fn(
    index: i64
) *void
```

#### `Vector::iter()`

```luma
Vector::iter -> fn(
) VectorIter
```


## Functions

### `create_vector_capacity`

Creates a vector with a specific initial capacity.

Allocates memory for init_capacity elements upfront.
Use this when you know approximately how many elements you'll need
to avoid multiple reallocations.

@param init_capacity Initial number of elements to allocate space for
@param element_size Size in bytes of each element (use sizeof<T>)
@return Newly created vector (caller must call free_vector when done)


```luma
pub #returns_ownership
create_vector_capacity -> fn(
    init_capacity: i64,
    element_size: i64
) Vector
```

**Example:**
```luma
let v: Vector = create_vector_capacity(100, sizeof<f64>);
defer free_vector(&v);
// Vector can hold 100 doubles before first reallocation
```

### `create_vector`

Creates a vector with default initial capacity.

Uses DEFAULT_VECTOR_CAPACITY (1024) as the initial capacity.
This is the most common way to create a vector.

@param element_size Size in bytes of each element (use sizeof<T>)
@return Newly created vector (caller must call free_vector when done)


```luma
pub #returns_ownership
create_vector -> fn(
    element_size: i64
) Vector
```

**Example:**
```luma
let v: Vector = create_vector(sizeof<i64>);
defer free_vector(&v);

loop [i: i64 = 0](i < 10) : (++i) {
    v.push_back(cast<*void>(&i));
}
```

### `free_vector`

Frees all memory associated with a vector.

After calling this function, the vector should not be used.
This function takes ownership of the vector.

@param v Pointer to the vector to free


```luma
pub #takes_ownership
free_vector -> fn(
    v: *Vector
) void
```

**Example:**
```luma
let v: Vector = create_vector(sizeof<i64>);
// ... use vector ...
free_vector(&v);
// v is now invalid
```

