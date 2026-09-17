# Module: std_cstring

*Source: `std/cstring.lx`*

String manipulation and conversion utilities

This module provides functions for working with null-terminated C-style strings,
including concatenation, comparison, conversion, and character classification.
All strings are represented as pointers to byte arrays (`*byte`) and must be
null-terminated.

This module is completely self-contained with no external dependencies.

# Memory Management
Functions that create new strings (prefixed with `from_` or returning `#returns_ownership`)
allocate memory that the caller must free. Use `defer { free(str); }` to ensure cleanup.

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)

---

## Structures

### `Slice`

String slice with pointer and length

| Field | Type | Description |
|-------|------|-------------|
| `ptr` | *byte |  |
| `len` | i64 |  |

**Methods:**

#### `Slice::slice_end()`

```luma
Slice::slice_end -> fn(
) *byte
```

#### `Slice::null_terminated_slice()`

```luma
Slice::null_terminated_slice -> fn(
    s: *byte
) Slice
```

### `String`

String with slice and capacity

| Field | Type | Description |
|-------|------|-------------|
| `slice` | Slice |  |
| `cap` | i64 |  |

**Methods:**

#### `String::concat_strs()`

```luma
#returns_ownership
String::concat_strs -> fn(
    start: *Slice,
    len: i64
) String
```

#### `String::as_slice()`

```luma
String::as_slice -> fn(
) Slice
```


## Functions

### `cat`

```luma
pub cat -> fn(
    dest: *byte,
    s1: *byte,
    s2: *byte
) *byte
```

### `free_string`

```luma
pub #takes_ownership
free_string -> fn(
    s: *String
) void
```

### `strlen`

Returns the length of a null-terminated string

Counts characters until the null terminator is found.


```luma
pub strlen -> fn(
    s: *byte
) i64
```

**Parameters:**
* `s` - String to measure


**Returns:**
Number of characters before null terminator


**Example:**
```luma
let len: i64 = string::strlen("Hello");
// len will be 5
```

### `dup`

Allocates a fresh copy of a null-terminated string

Useful when two owners need independent copies of the same text
(e.g. duplicating an AST name before attaching it to more than one node).


```luma
pub #returns_ownership
dup -> fn(
    s: *byte
) *byte
```

**Parameters:**
* `s` - String to duplicate


**Returns:**
Newly allocated copy of `s`; caller must free it

### `from_byte`

Converts a single byte to a string

Creates a 2-byte string containing the character and a null terminator.


```luma
pub #returns_ownership
from_byte -> fn(
    c: byte
) *byte
```

**Parameters:**
* `c` - Character to convert


**Returns:**
Newly allocated string containing the character


**Example:**
```luma
let str: *byte = string::from_byte('A');
defer { free(str); }
output(str); // Prints "A"
```

### `from_int`

Converts an integer to a string

Handles both positive and negative integers. For zero, returns "0".


```luma
pub #returns_ownership
from_int -> fn(
    n: i64
) *byte
```

**Parameters:**
* `n` - Integer to convert


**Returns:**
Newly allocated string representation of the integer


**Example:**
```luma
let str: *byte = string::from_int(-42);
defer { free(str); }
output(str); // Prints "-42"
```

### `from_float`

Converts an f32 to a string with specified precision

Separates integer and fractional parts, then combines them with
a decimal point. Precision determines number of decimal places.


```luma
pub #returns_ownership
from_float -> fn(
    f: f32,
    precision: i64
) *byte
```

**Parameters:**
* `f` - Float value to convert
* `precision` - Number of decimal places to include


**Returns:**
Newly allocated string representation of an f32


**Example:**
```luma
let str: *byte = string::from_float(3.14159, 2);
defer { free(str); }
output(str); // Prints "3.14"
```

### `int_to_str`

Converts integer to string into a provided buffer

Writes the string representation directly into the provided buffer
instead of allocating new memory.


```luma
pub int_to_str -> fn(
    num: i64,
    buf: *byte,
    buf_size: i64
) void
```

**Parameters:**
* `num` - Integer to convert
* `buf` - Buffer to write string into
* `buf_size` - Size of buffer in bytes


**Example:**
```luma
let buffer: [byte; 32];
string::int_to_str(12345, &buffer[0], 32);
output(&buffer[0]); // Prints "12345"
```

### `strcmp`

Compares two strings for equality

First checks if lengths differ, then compares contents byte by byte.


```luma
pub strcmp -> fn(
    s1: *byte,
    s2: *byte
) i64
```

**Parameters:**
* `s1` - First string
* `s2` - Second string


**Returns:**
* `0` if strings are equal
* `-1` if lengths differ
* `1` if contents differ but lengths match


**Example:**
```luma
if (string::strcmp("hello", "hello") == 0) {
    output("Strings are equal\n");
}
```

### `strncmp`

```luma
pub strncmp -> fn(
    s1: *byte,
    s2: *byte,
    n: i64
) i64
```

### `s_byte`

Searches for a character in a string

Returns the first occurrence of the character, or null byte if not found.


```luma
pub s_byte -> fn(
    s: *byte,
    c: i64
) byte
```

**Parameters:**
* `s` - String to search
* `c` - Character to find (as i64)


**Returns:**
The matching character if found, null byte otherwise


**Example:**
```luma
let ch: byte = string::s_byte("Hello", cast<i64>('l'));
if (ch != cast<byte>(0)) {
    output("Found 'l'\n");
}
```

### `contains_char`

Checks whether a character occurs anywhere in a string


```luma
pub contains_char -> fn(
    s: *byte,
    c: byte
) bool
```

**Parameters:**
* `s` - String to search
* `c` - Character to look for


**Returns:**
true if `c` occurs in `s`, false otherwise


**Example:**
```luma
if (string::contains_char("a/b", '/')) {
    output("has a slash\n");
}
```

### `copy`

Copies a string from source to destination

Copies characters including the null terminator.


```luma
pub copy -> fn(
    dest: *byte,
    src: *byte
) *byte
```

**Parameters:**
* `dest` - Destination buffer (must be large enough)
* `src` - Source string


**Returns:**
Destination pointer


**Example:**
```luma
let buffer: [byte; 100];
string::copy(&buffer[0], "Hello");
```

### `n_copy`

Copies at most n bytes of a string

Stops at null terminator or after n characters, whichever comes first.


```luma
pub n_copy -> fn(
    dest: *byte,
    src: *byte,
    n: i64
) *byte
```

**Parameters:**
* `dest` - Destination buffer
* `src` - Source string
* `n` - Maximum bytes to copy


**Returns:**
Destination pointer


**Example:**
```luma
let buffer: [byte; 10];
string::n_copy(&buffer[0], "Hello World", 5); // Copies "Hello"
```

### `cat`

Concatenates two strings into a destination buffer

Writes s1 followed by s2 into dest, with null terminator.


```luma
pub cat -> fn(
    dest: *byte,
    s1: *byte,
    s2: *byte
) *byte
```

**Parameters:**
* `dest` - Destination buffer (must be large enough)
* `s1` - First string
* `s2` - Second string


**Returns:**
Destination pointer


**Example:**
```luma
let buffer: [byte; 100];
string::cat(&buffer[0], "Hello ", "World");
output(&buffer[0]); // Prints "Hello World"
```

### `putbyte`

Outputs a single character to the terminal

Handles special characters with proper shell escaping using system commands.
Special handling for newline, tab, quotes, backslash, etc.


```luma
pub putbyte -> fn(
    c: byte
) i64
```

**Parameters:**
* `c` - Character to output


**Returns:**
The character as an i64


**Example:**
```luma
string::putbyte('A');
string::putbyte('\n');
```

### `is_digit`

Checks if a character is a digit (0-9)


```luma
pub is_digit -> fn(
    ch: byte
) bool
```

**Parameters:**
* `ch` - Character to test


**Returns:**
true if character is '0' through '9', false otherwise


**Example:**
```luma
if (string::is_digit('5')) {
    output("It's a digit!\n");
}
```

### `is_alpha`

Checks if a character is alphabetic or underscore

Accepts uppercase letters (A-Z), lowercase letters (a-z),
underscore (_), and at sign (@).


```luma
pub is_alpha -> fn(
    ch: byte
) bool
```

**Parameters:**
* `ch` - Character to test


**Returns:**
true if alphabetic or underscore/at, false otherwise


**Example:**
```luma
if (string::is_alpha('_')) {
    output("Valid identifier character\n");
}
```

### `is_alnum`

Checks if a character is alphanumeric

Returns true if character is a letter, digit, underscore, or at sign.


```luma
pub is_alnum -> fn(
    ch: byte
) bool
```

**Parameters:**
* `ch` - Character to test


**Returns:**
true if alphanumeric, false otherwise


**Example:**
```luma
if (string::is_alnum('A')) {
    output("Valid identifier character\n");
}
```

### `atio`

Converts ASCII string to integer

Supports decimal (no prefix), hex (0x), binary (0b) and octal (0o) literals
and skips underscore (`_`) digit separators. Stops at the first character
that is not a valid digit for the detected base (so a trailing type suffix
like `u8` is harmlessly ignored). Does not handle negative numbers.


```luma
pub atio -> fn(
    value: *byte
) i64
```

**Parameters:**
* `value` - String to parse


**Returns:**
Parsed integer value


**Example:**
```luma
let a: i64 = string::atio("12345");   // 12345
let b: i64 = string::atio("0xFF");    // 255
let c: i64 = string::atio("0b1010");  // 10
let d: i64 = string::atio("1_000");   // 1000
```

### `string_add`

Adds two large integers represented as strings

Performs arbitrary-precision addition digit by digit with carry propagation.
Both input strings must contain only digits.


```luma
pub #returns_ownership
string_add -> fn(
    num1: *byte,
    num2: *byte
) *byte
```

**Parameters:**
* `num1` - First number as string
* `num2` - Second number as string


**Returns:**
Sum as newly allocated string


**Example:**
```luma
let result: *byte = string::string_add("999", "1");
defer { free(result); }
output(result); // Prints "1000"
```

