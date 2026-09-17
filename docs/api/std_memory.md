# Module: std_memory

*Source: `std/memory.lx`*

Memory manipulation and allocation utilities

This module provides low-level memory operations for copying, comparing,
and manipulating raw memory. It includes implementations of standard C
library memory functions, plus additional utilities for common operations.

# Safety
These are low-level operations that work with raw pointers. Ensure that:
- Source and destination regions don't overlap (except for memmove)
- Pointers are valid and properly aligned
- Size parameters don't exceed allocated memory bounds

## Table of Contents

- [Functions](#functions)


## Functions

### `memcpy`

Copies n bytes from src to dest

This is a byte-by-byte copy that does NOT handle overlapping regions.
Use memmove() for overlapping memory regions.


```luma
pub memcpy -> fn(
    dest: *void,
    src: *void,
    n: i64
) *void
```

**Parameters:**
* `dest` - Destination memory address
* `src` - Source memory address
* `n` - Number of bytes to copy


**Returns:**
The destination pointer


**Example:**
```luma
let src: [i64; 3] = [1, 2, 3];
let dest: [i64; 3];
memory::memcpy(cast<*void>(&dest), cast<*void>(&src), 3 * sizeof<i64>);
```

### `memcmp`

Compares two memory regions byte by byte

Performs lexicographic comparison of memory contents.


```luma
pub memcmp -> fn(
    a: *void,
    b: *void,
    n: i64
) i64
```

**Parameters:**
* `a` - First memory region
* `b` - Second memory region
* `n` - Number of bytes to compare


**Returns:**
* `0` if regions are equal
* Positive value if first differing byte in a is greater
* Negative value if first differing byte in a is less


**Example:**
```luma
let a: [i64; 3] = [1, 2, 3];
let b: [i64; 3] = [1, 2, 3];
if (memory::memcmp(cast<*void>(&a), cast<*void>(&b), 3 * sizeof<i64>) == 0) {
    output("Arrays are equal\n");
}
```

### `memset`

Sets n bytes of memory to a specific value

Optimized implementation that processes 4 bytes at a time when possible,
then handles remaining bytes individually.


```luma
pub memset -> fn(
    dest: *void,
    value: i64,
    n: i64
) *void
```

**Parameters:**
* `dest` - Memory address to fill
* `value` - Byte value to set (only low 8 bits used)
* `n` - Number of bytes to set


**Returns:**
The destination pointer


**Example:**
```luma
let buffer: [byte; 100];
memory::memset(cast<*void>(&buffer), 0, 100); // Zero the buffer
```

### `memmove`

Copies memory regions handling overlaps correctly

Unlike memcpy, this function correctly handles overlapping memory regions
by choosing the appropriate copy direction (forward or backward).


```luma
pub memmove -> fn(
    dest: *void,
    src: *void,
    n: i64
) *void
```

**Parameters:**
* `dest` - Destination memory address
* `src` - Source memory address
* `n` - Number of bytes to copy


**Returns:**
The destination pointer


**Example:**
```luma
let buffer: [byte; 10];
// ... fill buffer ...
// Shift contents 2 bytes to the right (overlapping)
memory::memmove(cast<*void>(&buffer[2]), cast<*void>(&buffer[0]), 8);
```

### `memchr`

Finds first occurrence of a byte in memory

Scans memory for the first occurrence of a specific byte value.


```luma
pub memchr -> fn(
    ptr: *void,
    value: i64,
    n: i64
) *void
```

**Parameters:**
* `ptr` - Memory region to search
* `value` - Byte value to find
* `n` - Number of bytes to search


**Returns:**
Pointer to first occurrence, or NULL (0) if not found


**Example:**
```luma
let data: *byte = "Hello World";
let found: *byte = cast<*byte>(memory::memchr(cast<*void>(data), cast<i64>('W'), 11));
if (found != cast<*byte>(0)) {
    output("Found 'W'\n");
}
```

### `memzero`

Zeroes out memory region

Convenience function equivalent to memset(dest, 0, n).
Useful for clearing sensitive data or initializing structures.


```luma
pub memzero -> fn(
    dest: *void,
    n: i64
) *void
```

**Parameters:**
* `dest` - Memory address to zero
* `n` - Number of bytes to zero


**Returns:**
The destination pointer


**Example:**
```luma
let password: [byte; 64];
// ... use password ...
memory::memzero(cast<*void>(&password), 64); // Clear sensitive data
```

### `calloc`

Allocates and zeroes memory

Equivalent to C's calloc() - allocates memory for an array of elements
and initializes all bytes to zero.


```luma
pub #returns_ownership
calloc -> fn(
    count: i64,
    size: i64
) *void
```

**Parameters:**
* `count` - Number of elements
* `size` - Size of each element in bytes


**Returns:**
Pointer to allocated zeroed memory, or NULL on failure


**Example:**
```luma
let array: *i64 = cast<*i64>(memory::calloc(100, sizeof<i64>));
defer { free(array); }
// All 100 integers are initialized to 0
```

### `realloc`

Reallocates memory to a new size

Allocates a new block, copies old data, and frees the old block.
If ptr is NULL, behaves like alloc(). The old pointer becomes invalid.


```luma
pub #returns_ownership
realloc -> fn(
    ptr: *void,
    old_size: i64,
    new_size: i64
) *void
```

**Parameters:**
* `ptr` - Pointer to existing allocation (or NULL)
* `new_size` - New size in bytes


**Returns:**
Pointer to new allocation, or NULL on failure


**Example:**
```luma
let data: *i64 = cast<*i64>(alloc(10 * sizeof<i64>));
// ... need more space ...
data = cast<*i64>(memory::realloc(cast<*void>(data), 20 * sizeof<i64>));
```

### `memswap`

Swaps contents of two memory regions

Exchanges the bytes between two memory regions using a temporary variable.
The regions must be the same size.


```luma
pub memswap -> fn(
    a: *void,
    b: *void,
    n: i64
) void
```

**Parameters:**
* `a` - First memory region
* `b` - Second memory region
* `n` - Number of bytes to swap


**Example:**
```luma
let x: i64 = 10;
let y: i64 = 20;
memory::memswap(cast<*void>(&x), cast<*void>(&y), sizeof<i64>);
// Now x=20, y=10
```

### `memswapn`

Swaps multiple fixed-size elements between two arrays

Efficiently swaps count elements of size bytes between two arrays.


```luma
pub memswapn -> fn(
    a: *void,
    b: *void,
    count: i64,
    size: i64
) void
```

**Parameters:**
* `a` - First array
* `b` - Second array
* `count` - Number of elements to swap
* `size` - Size of each element in bytes


**Example:**
```luma
let arr1: [i64; 5] = [1, 2, 3, 4, 5];
let arr2: [i64; 5] = [6, 7, 8, 9, 10];
memory::memswapn(cast<*void>(&arr1), cast<*void>(&arr2), 5, sizeof<i64>);
```

### `memfill`

Fills memory with repeated values

Optimized for common sizes (1 byte, 8 bytes). For other sizes,
fills with the low byte of value.


```luma
pub memfill -> fn(
    dest: *void,
    value: i64,
    size: i64,
    count: i64
) *void
```

**Parameters:**
* `dest` - Destination memory
* `value` - Value to fill with
* `size` - Size of each element in bytes
* `count` - Number of elements to fill


**Returns:**
The destination pointer


**Example:**
```luma
let array: [i64; 100];
memory::memfill(cast<*void>(&array), 42, sizeof<i64>, 100);
// All 100 elements are now 42
```

### `memrev`

Reverses bytes in memory region

Reverses the order of bytes in place.


```luma
pub memrev -> fn(
    ptr: *void,
    n: i64
) *void
```

**Parameters:**
* `ptr` - Memory region to reverse
* `n` - Number of bytes


**Returns:**
The pointer


**Example:**
```luma
let data: [byte; 5] = ['H', 'e', 'l', 'l', 'o'];
memory::memrev(cast<*void>(&data), 5);
// Now: ['o', 'l', 'l', 'e', 'H']
```

### `memcount`

Counts occurrences of a byte in memory

Scans memory and counts how many times a specific byte value appears.


```luma
pub memcount -> fn(
    ptr: *void,
    value: i64,
    n: i64
) i64
```

**Parameters:**
* `ptr` - Memory region to search
* `value` - Byte value to count
* `n` - Number of bytes to search


**Returns:**
Number of occurrences found


**Example:**
```luma
let text: *byte = "Hello World";
let count: i64 = memory::memcount(cast<*void>(text), cast<i64>('l'), 11);
// count will be 3
```

### `memdup`

Duplicates a memory region

Allocates new memory and copies the source data. The caller is
responsible for freeing the returned pointer.


```luma
pub #returns_ownership
memdup -> fn(
    src: *void,
    n: i64
) *void
```

**Parameters:**
* `src` - Source memory to duplicate
* `n` - Number of bytes to duplicate


**Returns:**
Pointer to new allocation containing copy, or NULL on failure


**Example:**
```luma
let original: [i64; 5] = [1, 2, 3, 4, 5];
let copy: *i64 = cast<*i64>(memory::memdup(cast<*void>(&original), 5 * sizeof<i64>));
defer { free(copy); }
```

### `memeq`

Checks if two memory regions are equal

Convenience wrapper around memcmp that returns a boolean.


```luma
pub memeq -> fn(
    a: *void,
    b: *void,
    n: i64
) bool
```

**Parameters:**
* `a` - First memory region
* `b` - Second memory region
* `n` - Number of bytes to compare


**Returns:**
true if regions are identical, false otherwise


**Example:**
```luma
let a: [i64; 3] = [1, 2, 3];
let b: [i64; 3] = [1, 2, 3];
if (memory::memeq(cast<*void>(&a), cast<*void>(&b), 3 * sizeof<i64>)) {
    output("Equal!\n");
}
```

### `memmem`

Finds a substring in memory

Searches for the first occurrence of a byte sequence (needle)
within a larger memory region (haystack).


```luma
pub memmem -> fn(
    haystack: *void,
    haystack_len: i64,
    needle: *void,
    needle_len: i64
) *void
```

**Parameters:**
* `haystack` - Memory to search in
* `haystack_len` - Length of haystack in bytes
* `needle` - Pattern to search for
* `needle_len` - Length of needle in bytes


**Returns:**
Pointer to first occurrence, or NULL if not found


**Example:**
```luma
let text: *byte = "Hello World";
let pattern: *byte = "World";
let found: *void = memory::memmem(
    cast<*void>(text), 11,
    cast<*void>(pattern), 5
);
```

### `align`

Calculates alignment for struct members

Returns the maximum alignment requirement from a list of member alignments.
Used for determining struct alignment.


```luma
pub align -> fn(
    member_alignments: *i64,
    count: i64
) i64
```

**Parameters:**
* `member_alignments` - Array of alignment values
* `count` - Number of alignments in array


**Returns:**
Maximum alignment value


**Example:**
```luma
let alignments: [i64; 3] = [4, 8, 4];
let struct_align: i64 = memory::align(&alignments[0], 3);
// Returns 8 (largest alignment)
```

