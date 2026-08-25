# Luma Language Documentation

Luma is a statically typed, compiled programming language designed for systems programming. It combines the low-level control of C with a strong type system and modern safety features that eliminate many common runtime errors.

Luma is built on three core principles:

- **Simplicity**: Minimal syntax with consistent patterns
- **Safety**: Strong typing and memory safety features
- **Performance**: Zero-cost abstractions and predictable performance

---

## Quick Start

Here's a complete Luma program that demonstrates the core language features:

```luma
@module "main"

const Point -> struct {
pub:
    x: int,
    y: int,

    distance_to -> fn (other: Point) float {
        let dx: int = other.x - self.x;
        let dy: int = other.y - self.y;
        return cast<float>(sqrt(cast<double>(dx * dx + dy * dy)));
    },
};

const Status -> enum {
    Active,
    Inactive,
    Pending,
};

pub const main -> fn (argc: int, argv: **byte) int {
    let origin: Point = Point { x: 0, y: 0 };
    let destination: Point = Point { x: 3, y: 4 };
    let current_status: Status = Status::Active;
    
    outputln("Distance: ", origin.distance_to(destination));
    
    switch (current_status) {
        Status::Active -> outputln("System is running");
        Status::Inactive -> outputln("System is stopped");
        Status::Pending -> outputln("System is starting");
    }
    
    return 0;
}
```

This example shows:
- Module declaration with `@module`
- Struct definitions with methods
- Enum definitions
- Static access with `::` for enum variants
- Runtime access with `.` for struct members
- Function definitions and calls
- Switch statements with pattern matching

---

## Type System

Luma provides a straightforward type system with both primitive and compound types.

### Primitive Types
```
int      - Signed integer (64-bit)
uint     - Unsigned integer (64-bit)
float    - Floating point (32-bit)
double   - Floating point (64-bit)
bool     - Boolean (1 byte)
byte     - Single byte (1 byte)
*byte    - Character pointer / C-style string
void     - No value (used for function return types and generic pointers)
```

**Note on String Types:**
- String literals like `"hello"` are of type `*byte` (null-terminated character arrays)
- All string operations in the standard library use `*byte`
- There is no separate `str` type in Luma

### Type Modifiers & Operators
```
*T       - Pointer type (declares a pointer to type T)
[T; N]   - Array type (fixed-size array of N elements of type T)
```

**Pointer Operators:**
```
*expr    - Dereference operator (access value pointed to)
&expr    - Address-of operator (get pointer to value)
```

**Example:**
```luma
let x: int = 42;           // x is an int
let ptr: *int = &x;        // ptr is a pointer to int, holds address of x
let value: int = *ptr;     // value is 42 (dereferenced ptr)
```

### Enumerations

Enums provide type-safe constants with underlying integer values:
```luma
const Direction -> enum {
    North,    // = 0
    South,    // = 1
    East,     // = 2
    West      // = 3
};

const current_direction: Direction = Direction::North;

// Can cast to int if needed
let dir_value: int = cast<int>(Direction::North);  // 0
```

### Structures

Structures group related data with optional access control:
```luma
const Point -> struct {
    x: int,
    y: int
};

// With explicit access modifiers and methods
const Player -> struct {
pub:
    name: *byte,
    score: int,

    // Methods can be defined inside structs — fields are reached through
    // `self`, never as bare names. Currently, a method must be declared in
    // the `pub:` group to typecheck correctly — one declared after `priv:`
    // doesn't get `self` resolved (a known compiler bug).
    get_info -> fn () void {
        outputln("Player: ", self.name, " Score: ", self.score);
    },
priv:
    internal_id: int,
};
```

### Using Types
```luma
const origin: Point = Point { x: 0, y: 0 };

const player: Player = Player { 
    name: "Alice", 
    score: 100,
    internal_id: 12345 
};

// Access fields
outputln(origin.x);           // 0
outputln(player.name);        // Alice

// Call methods
player.get_info();            // Player: Alice Score: 100
```

### Pointers to Structs

Unlike C, there's no separate `->` operator — `.` works the same way whether you have a struct value or a pointer to one, auto-dereferencing either way:

```luma
const move_point -> fn (p: *Point, dx: int, dy: int) void {
    p.x = p.x + dx;   // not p->x
    p.y = p.y + dy;
}

pub const main -> fn (argc: int, argv: **byte) int {
    let p: Point = Point { x: 1, y: 2 };
    let ptr: *Point = &p;

    move_point(ptr, 10, 10);
    outputln(p.x, " ", p.y);     // 11 12
    outputln(ptr.x, " ", ptr.y); // same values, through the pointer

    return 0;
}
```

This is also why a method's `self` (always `*Self` under the hood) reads no differently from a plain struct field access — `self.x` inside a method and `p.x` on a local value use identical syntax.

### Struct Composition vs. Embedding

A struct-typed field declared normally (without `...`) is composition, not embedding — it's a nested value, and its fields are **not** promoted. You reach them through the field name, same as any other member:

```luma
const Line -> struct {
pub:
    start: Point,
    end: Point,
};

pub const main -> fn (argc: int, argv: **byte) int {
    let line: Line = Line {
        start: Point { x: 0, y: 0 },
        end: Point { x: 5, y: 5 },
    };

    outputln(line.start.x);  // 0 — through `.start`, not promoted
    outputln(line.end.y);    // 5
    return 0;
}
```

Compare this to [Struct Embedding](#struct-embedding) below, where `...Point,` would make `line.x` and `line.y` valid directly. Use composition when the nested struct is conceptually a distinct part (a `Line` *has a* `start` and an `end`); use embedding when the outer struct *is a kind of* the inner one and should expose its interface directly (a `Player` *is an* `Entity`).

**Structs don't support `==`** — comparing two struct values field-by-field isn't generated automatically; compare the fields you care about individually instead.

### Struct Embedding

A struct can embed another by value with `...Type,` as a member. The embedded struct's fields and methods are promoted onto the outer struct — accessible and initializable as if they were declared directly on it, with no runtime indirection (the embedded value is laid out inline, not behind a pointer):

```luma
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
    name: *byte,

    // Defining `move` here shadows Entity's — a direct member always wins
    // over a promoted one of the same name.
    move -> fn (dx: float, dy: float) void {
        self.x = self.x + (dx * 2.0);
        self.y = self.y + (dy * 2.0);
    },
};

pub const main -> fn (argc: int, argv: **byte) int {
    // `x`/`y` are promoted fields — the literal initializes them exactly
    // like `name`, even though they live on the embedded `Entity`.
    let player: Player = Player { x: 0.0, y: 0.0, name: "Connor" };
    player.move(10.0, 5.0);           // Player's own move — doubled deltas
    outputln(player.x, " ", player.y);
    return 0;
}
```

Promoted members resolve through as many embedding levels as needed, and work identically through a pointer (`p.move(...)` where `p: *Player`).

An override can still reach the shadowed base method explicitly, by naming the embedded type directly with `.` — `Entity.move(dx, dy)` inside `Player::move` calls `Entity`'s own implementation on `self`'s embedded `Entity`, bypassing the override it's written inside of:

```luma
const Player -> struct {
pub:
    ...Entity,
    name: *byte,

    move -> fn (dx: float, dy: float) void {
        Entity.move(dx, dy);  // delegates to Entity's own move, unmodified
    },
};
```

This only resolves inside a method whose owning struct actually embeds the named type somewhere; `Entity.move(...)` written elsewhere doesn't mean anything.

### Static Methods

A method declared `static` has no implicit `self` and is called on the type itself with `::`, not on an instance with `.` — useful for constructors and other functions that logically belong to a type but don't operate on an existing value of it:

```luma
const Point -> struct {
pub:
    x: float,
    y: float,

    static origin -> fn () Point {
        return Point { x: 0.0, y: 0.0 };
    },

    static at -> fn (x: float, y: float) Point {
        return Point { x: x, y: y };
    },

    move -> fn (dx: float, dy: float) void {
        self.x = self.x + dx;
        self.y = self.y + dy;
    },
};

pub const main -> fn (argc: int, argv: **byte) int {
    let a: Point = Point::origin();   // static — no instance needed
    let b: Point = Point::at(3.0, 4.0);
    a.move(1.0, 1.0);                 // instance method — needs `a`
    return 0;
}
```

Inside a `static` method's body, `self` isn't in scope — there's no instance to refer to. `static` is only valid on a method (`name -> fn (...) T { ... }`), not on a data field.

`#returns_ownership`/`#takes_ownership` can be combined with `static` in either order:

```luma
#returns_ownership
static create -> fn (...) *T { ... }
```

### Heap-Allocated Structs

A `static` "constructor" returning a pointer, paired with an instance "destructor" method, is the idiomatic way to give a struct manual, class-like lifetime management — the same ownership rules from [Memory Management](#memory-management) apply, just wrapped in methods instead of loose functions:

```luma
const Person -> struct {
pub:
    name: *byte,  // owned
    age: int,

    #returns_ownership
    static create -> fn (name: *byte, age: int) *Person {
        let p: *Person = cast<*Person>(alloc(sizeof<Person>));
        p.name = name;
        p.age = age;
        return p;
    },

    destroy -> fn () void {
        free(self.name);
    },
};

pub const main -> fn (argc: int, argv: **byte) int {
    let alice: *Person = Person::create(cast<*byte>(alloc(6)), 30);
    defer { alice.destroy(); free(alice); }

    outputln(alice.age);
    return 0;
}
```

`destroy` only frees what the struct itself owns (`name`) — the struct's own allocation (`alice` the pointer) is a separate responsibility, freed by whoever called `create`, same as any other `#returns_ownership` pointer. Nothing in Luma calls `destroy` automatically; there's no destructor-on-scope-exit — pair it with `defer` explicitly, as shown.

### Type Compatibility
```luma
// Same types
let x: int = 42;
let y: int = x;  // OK

// Different types require explicit cast
let f: float = cast<float>(x);  // OK
let z: int = f;  // ERROR: must use cast<int>(f)

// Pointer type safety
let int_ptr: *int = &x;
let void_ptr: *void = cast<*void>(int_ptr);  // Explicit cast required
```

See [Type Casting System](#type-casting-system) for full details on conversions.

---

## Generics (Not yet supported)

Luma supports generic programming through templates, enabling you to write code that works with multiple types while maintaining type safety and zero-cost abstractions.

### Generic Functions

Generic functions are declared with type parameters in angle brackets `<>` after the `fn` keyword:

```luma
const add = fn<T>(a: T, b: T) T { 
    return a + b; 
}

const swap = fn<T>(a: *T, b: *T) void {
    let temp: T = *a;
    *a = *b;
    *b = temp;
}

const max = fn<T>(a: T, b: T) T {
    if (a > b) {
        return a;
    }
    return b;
}
```

### Using Generic Functions

Generic functions require **explicit type arguments** at the call site:

```luma
const main = fn() int {
    // Integer arithmetic
    outputln("add(1, 2) = ", add<int>(1, 2));
    
    // Floating-point arithmetic
    outputln("add(1.5, 2.5) = ", add<float>(1.5, 2.5));

    // Swapping integers
    let x: int = 5; 
    let y: int = 10;
    swap<int>(&x, &y);
    outputln("After swap: x = ", x, ", y = ", y);
    
    // Finding maximum
    let largest: int = max<int>(42, 17);
    outputln("Max: ", largest);
    
    return 0;
}
```

### Generic Structs

Structs can also be generic, allowing you to create container types and data structures that work with any type:

```luma
const Box -> struct<T> {
    value: T,
    
    get = fn() T {
        return value;
    },
    
    set = fn(new_value: T) void {
        value = new_value;
    }
};

const Pair -> struct<T, U> {
    first: T,
    second: U
};

// Usage
const main = fn() int {
    // Box holding an integer
    let int_box: Box<int> = Box { value: 42 };
    outputln("Box contains: ", int_box.get());
    
    // Box holding a float
    let float_box: Box<float> = Box { value: 3.14 };
    
    // Pair with different types
    let pair: Pair<int, *byte> = Pair { 
        first: 1, 
        second: "hello" 
    };
    outputln("Pair: (", pair.first, ", ", pair.second, ")");
    
    return 0;
}
```

### Monomorphization

Luma uses **monomorphization** for generic code generation. This means:

- The compiler generates **separate machine code** for each concrete type used
- Generic code has **zero runtime overhead** compared to hand-written type-specific code
- Each instantiation (e.g., `add<int>`, `add<float>`) produces its own optimized assembly
- Similar to C++ templates and Rust generics, not Java's type erasure

**Example:**

```luma
const identity = fn<T>(x: T) T {
    return x;
}

// These calls generate separate functions in the compiled binary:
let a: int = identity<int>(42);        // Generates identity_int
let b: float = identity<float>(3.14);  // Generates identity_float
let c: *byte = identity<*byte>("hello"); // Generates identity_str
```

---

## Top-Level Bindings with `const`

Luma uses the `const` keyword as a **unified declaration mechanism** for all top-level bindings. Whether you're declaring variables, functions, types, or enums, `const` provides a consistent syntax that enforces immutability at the binding level.

### Basic Syntax

```luma
const NUM: int = 42;                                  // Immutable variable
const Direction -> enum { North, South, East, West };  // Enum definition
const Point -> struct { x: int, y: int };              // Struct definition
const add -> fn (a: int, b: int) int {                 // Function definition
    return a + b;
}
```

(Generic bindings like `struct<T>`/`fn<T>` are aspirational syntax — see [Generics](#generics-not-yet-supported), which isn't implemented yet.)

### Why This Design?

**Unified syntax**: One parsing rule handles all top-level declarations, simplifying both the compiler and developer experience.

**Semantic clarity**: The binding itself is immutable—you cannot reassign or shadow a top-level `const`. This prevents accidental redefinition bugs.

**Compiler optimization**: Immutable bindings enable better optimization opportunities.

**Future extensibility**: This approach naturally supports compile-time metaprogramming and uniform import behavior.

### Important Notes

```luma
const x: int = 5;
x = 10; // Error: `x` is immutable

const add -> fn (a: int, b: int) int { return a + b; }
add = something_else; // Error: cannot reassign function binding
```

---

## Variables and Mutability

Inside functions, use `let` to declare local variables:

```luma
const main -> fn (argc: int, argv: **byte) int {
    let x: int = 10;        // Mutable local variable
    x = 20;                 // Can be reassigned
    
    let y: int = 5;
    y = y + 1;              // Can be modified
    
    let counter: int = 0;
    loop (counter < 10) {
        counter = counter + 1;  // Mutating in loop
    }
    
    return 0;
}
```

**Key difference:**
- `const` at top-level = immutable binding (cannot reassign)
- `let` in functions = mutable variable (can reassign and modify)

---

## Functions

Functions are first-class values in Luma.

### Function Declaration

```luma
// Basic function
const add -> fn (a: int, b: int) int {
    return a + b;
}

// Function with no parameters
const greet -> fn () void {
    outputln("Hello!");
}

// Function with no return value
const print_number -> fn (n: int) void {
    outputln("Number: ", n);
}
```

A function declaration's closing `}` never takes a trailing `;` — that's only for `struct`/`enum` declarations and ordinary statements (see [Top-Level Bindings with `const`](#top-level-bindings-with-const)).

### main()

`main` can declare zero, one, or two parameters — `argc: int` and `argv: **byte`, in that order. Its C-level signature is always `int main(int, char**)` regardless; declaring fewer just leaves the rest unnamed on the Luma side.

```luma
pub const main -> fn () int {
    return 0;
}

pub const main -> fn (argc: int) int {
    return 0;
}

pub const main -> fn (argc: int, argv: **byte) int {
    return 0;
}
```

### Function Calls

```luma
const main -> fn (argc: int, argv: **byte) int {
    let result: int = add(5, 3);
    outputln("5 + 3 = ", result);
    
    greet();
    print_number(42);
    
    return 0;
}
```

### Function Parameters

Parameters are passed by value by default:

```luma
const modify -> fn (x: int) void {
    x = 100;  // Modifies local copy only
}

const main -> fn (argc: int, argv: **byte) int {
    let num: int = 10;
    modify(num);
    outputln(num);  // Still 10
    return 0;
}
```

To modify the caller's variable, use pointers:

```luma
const modify_ptr -> fn (x: *int) void {
    *x = 100;  // Modifies original value
}

const main -> fn (argc: int, argv: **byte) int {
    let num: int = 10;
    modify_ptr(&num);  // Pass address
    outputln(num);     // Now 100
    return 0;
}
```

### Return Values

```luma
// Single return value
const square -> fn (x: int) int {
    return x * x;
}

// Multiple return values via struct
const DivResult -> struct {
    quotient: int,
    remainder: int
};

const divide -> fn (a: int, b: int) DivResult {
    return DivResult {
        quotient: a / b,
        remainder: a % b
    };
}

const main -> fn (argc: int, argv: **byte) int {
    let result: DivResult = divide(17, 5);
    outputln("17 / 5 = ", result.quotient, " R ", result.remainder);
    return 0;
}
```

### Early Returns

```luma
const find_positive -> fn (numbers: *int, size: int) int {
    loop [i: int = 0](i < size) : (++i) {
        if (numbers[i] > 0) {
            return numbers[i];  // Early return
        }
    }
    return -1;  // Not found
}
```

---

## Name Resolution

Luma uses two distinct operators for name resolution to provide semantic clarity:

### Static Access with `::`

The `::` operator is used for **compile-time static access**:

```luma
// Enum variants
const day: WeekDay = WeekDay::Monday;

// Module/namespace access
math::sqrt(16.0)

// Static methods — no implicit `self`, called on the type itself
Point::at(10, 20)
```

(See [Static Methods](#static-methods) for how `Point::at` is declared.)

### Runtime Member Access with `.`

The `.` operator is used for **runtime member access**:

```luma
// Struct field access
let point: Point = Point { x: 10, y: 20 };
outputln(point.x);  // Access field at runtime

// Method calls on instances
let distance: float = origin.distance_to(destination);
```

### Benefits of This Distinction

- **Semantic clarity**: `::` means "resolved at compile time", `.` means "accessed at runtime"
- **Easier parsing**: The compiler immediately knows the access type
- **Consistent with systems languages**: Similar to C++ and Rust conventions
- **Future-proof**: Supports advanced features like associated functions

---

## Control Flow

Luma provides clean, flexible control flow constructs that handle most programming patterns without unnecessary complexity.

### Conditional Statements

Use `if`, `elif`, and `else` for branching logic:

```luma
const x: int = 7;

if (x > 10) {
    outputln("Large number");
} elif (x > 5) {
    outputln("Medium number");  // This will execute
} else {
    outputln("Small number");
}
```

### Loop Constructs

The `loop` keyword provides several iteration patterns:

#### For-Style Loops

```luma
// Basic for loop
loop [i: int = 0](i < 10) {
    outputln("Iteration: ", i);
    ++i;
}

// For loop with post-increment
loop [i: int = 0](i < 10) : (++i) {
    outputln("i = ", i);
}

```

**Not currently working**: multiple loop-init variables (`loop [i: int = 0, j: int = 0](...) ...`) parse but generate invalid C (`for (long long i = 0, long long j = 0; ...)`, which repeats the type where C expects a bare `j = 0`) — the build fails at the C compile step. For now, declare the second counter with `let` above the loop instead:

```luma
let j: int = 0;
loop [i: int = 0](i < 10) : (++i) {
    outputln("i = ", i, ", j = ", j);
    ++j;
}
```

#### While-Style Loops

```luma
// Condition-only loop
let counter: int = 0;
loop (counter < 5) {
    outputln("Count: ", counter);
    counter = counter + 1;
}

// While loop with post-action
let j: int = 0;
loop (j < 10) : (++j) {
    outputln("Processing: ", j);
}
```

#### Infinite Loops

```luma
loop {
    // Runs forever until `break` is encountered
    if (should_exit()) {
        break;
    }
    do_work();
}
```

#### Loop Control

```luma
// Break: exit loop early
loop [i: int = 0](i < 100) : (++i) {
    if (i == 50) {
        break;  // Exit loop
    }
    process(i);
}

// Continue: skip to next iteration
loop [i: int = 0](i < 10) : (++i) {
    if (i % 2 == 0) {
        continue;  // Skip even numbers
    }
    outputln("Odd: ", i);
}
```

---

## Switch Statements

Luma provides powerful pattern matching through `switch` statements that work with enums, integers, and other types. Switch statements must be exhaustive and all cases must be compile-time constants.

### Basic Switch Syntax

```luma
@module "main"

const WeekDay -> enum {
    Sunday,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
};

const classify_day -> fn (day: WeekDay) void {
    switch (day) {
        WeekDay::Monday, WeekDay::Tuesday, WeekDay::Wednesday, 
        WeekDay::Thursday, WeekDay::Friday ->
            outputln("Weekday => ", day);
        WeekDay::Saturday, WeekDay::Sunday ->
            outputln("Weekend => ", day);
    }
}

pub const main -> fn (argc: int, argv: **byte) int {
    classify_day(WeekDay::Monday);   // Output: Weekday => 1
    classify_day(WeekDay::Saturday); // Output: Weekend => 6
    return 0;
}
```

### Switch with Default Case

When you need to handle unexpected values or want a catch-all case, use the default wildcard pattern `_`:

```luma
const handle_status_code -> fn (code: int) void {
    switch (code) {
        200 -> outputln("OK");
        404 -> outputln("Not Found");
        500 -> outputln("Internal Server Error");
        _   -> outputln("Unknown status code");
    }
}
```

### Switch Features

- **Multiple values per case**: Combine multiple values using commas
- **Exhaustiveness**: All possible values must be covered (or use `_` for default)
- **Compile-time constants**: All case values must be compile-time constants
- **No fallthrough**: Each case is automatically contained (no `break` needed)

### Scoped Enum Access with `using`

Fully-qualified case labels (`WeekDay::Monday`) get repetitive once an enum has more than a couple of members. `switch using <Path::To::Enum> (expr)` lets every case label in that switch be written as a bare member name instead:

```luma
const classify_day -> fn (day: WeekDay) void {
    switch using WeekDay (day) {
        Monday, Tuesday, Wednesday, Thursday, Friday -> {
            outputln("Weekday => ", day);
        }
        Saturday, Sunday -> {
            outputln("Weekend => ", day);
        }
    }
}
```

This is pure sugar, resolved entirely while parsing: `Monday` inside this switch expands to exactly the same `WeekDay::Monday` node a fully-qualified label would produce, so exhaustiveness checking, duplicate-case detection, and codegen all behave identically to the fully-qualified form. A label can still be written fully qualified (or reference a different enum entirely) inside a `using` switch — `using` only applies to bare identifiers, so an explicit `Other::Value` label is left untouched. The shorthand is scoped to the one `switch` block it's declared on; it never leaks into surrounding code.

---

## Module System

Luma provides a simple module system for code organization and namespace management.

### Module Declaration

Every Luma source file must declare its module name:

```luma
@module "main"

// Your code here...
```

### Importing Modules

Use the `@use` directive to import other modules:

```luma
@module "main"

@use "std_libc" as c
@use "std_cstring" as string

pub const main -> fn (argc: int, argv: **byte) int {
    // Access imported functions with namespace
    let result: double = c::sqrt(16.0);
    let len: int = string::strlen("hello");

    outputln("sqrt(16): ", result);
    outputln("Length: ", len);
    return 0;
}
```

`@use` only declares the dependency — it doesn't locate the file. Every module you `@use` also has to be passed to the compiler explicitly with `-l`:

```
luma main.lx -l std/libc.lx std/cstring.lx -name main
```

### Standard Library Module Names

All standard library modules use the `std_` prefix:

```
std_math      - Mathematical functions and constants
std_memory    - Low-level memory operations
std_cstring   - Null-terminated (*byte) string utilities: strlen, strcmp, copy, dup, ...
std_string    - Growable String struct built on top of std_cstring
std_io        - High-level I/O with formatted output
std_sys       - POSIX system calls (Linux/macOS)
std_win32     - Windows Win32 API
std_termfx    - Terminal colors and formatting
std_terminal  - Terminal input/raw mode control
std_time      - Time and timing operations
std_vector    - Dynamic array
std_hashmap   - Hash map
std_arena     - Arena allocator
std_args      - Command-line argument parsing
std_libc      - C standard library bindings
std_thread    - Threading
```

### Module Features

- **Explicit imports**: All dependencies must be explicitly declared
- **Namespace isolation**: Imported modules are accessed through their aliases
- **Static resolution**: All module access is resolved at compile time using `::`
- **Clean syntax**: Simple `@use "module" as alias` pattern

---

## Platform Directives

### `@os` — Conditional Compilation

The `@os` block selects code based on the target operating system. This is how the standard library handles platform differences for things like syscall numbers and flags:

```luma
@os {
    "linux" -> {
        pub const SYS_WRITE: int = 1;
        pub const O_CREAT: int   = 64;
    }
    "macos" -> {
        pub const SYS_WRITE: int = 4;
        pub const O_CREAT: int   = 512;
    }
    "windows" -> {
        // Windows-specific declarations...
    }
}
```

You can also use `@os` inline inside function bodies:

```luma
const write_out -> fn (s: *byte) int {
    @os {
        "linux"   -> { return __syscall__(1, 1, cast<int>(s), len); }
        "macos"   -> { return __syscall__(4, 1, cast<int>(s), len); }
        "windows" -> { /* use WriteFile */ }
    }
}
```

Valid platform strings are `"linux"`, `"macos"`, and `"windows"`.

---

## Foreign Function Interface (FFI)

Luma can call into native shared libraries and DLLs through two complementary directives.

### `@link` — Shared Library Linking (POSIX)

`@link` declares that the entire module links against a shared library. Place it at the top of your module, before any function declarations:

```luma
@module "std_libc"

@link("libc.so.6")

pub const puts    -> fn (s: *byte) int;
pub const printf  -> fn (fmt: *byte, val: *byte) int;
pub const malloc  -> fn (size: int) *void;
// ...
```

The function body is omitted — the linker resolves it from the named library at link time. Only one `@link` per module is needed; it applies to all subsequent `pub const` declarations that have no body.

### `#lib_import` — Per-Function Library Override

When individual functions within a module come from a *different* library than the module-level `@link`, use `#lib_import` as a per-declaration attribute:

```luma
@module "std_libc"

@link("libc.so.6")

// Most functions come from libc.so.6 via @link above:
pub const malloc -> fn (size: int) *void;
pub const free   -> fn (ptr: *void) void;

// Math functions need libm — override per-function:
#lib_import("libm.so")
pub const sqrt -> fn (x: double) double;

#lib_import("libm.so")
pub const pow  -> fn (base: double, exp: double) double;

#lib_import("libm.so")
pub const sin  -> fn (x: double) double;
```

Place `#lib_import(...)` on the line immediately before the `pub const` it applies to.

### `#dll_import` — Windows DLL Imports

On Windows, use `#dll_import` instead. It accepts the DLL name and an optional calling convention:

```luma
#dll_import("kernel32.dll", callconv: "stdcall")
pub const CreateFileA -> fn (
    lpFileName: *byte,
    dwDesiredAccess: int,
    dwShareMode: int,
    lpSecurityAttributes: *void,
    dwCreationDisposition: int,
    dwFlagsAndAttributes: int,
    hTemplateFile: int
) int;

#dll_import("kernel32.dll", callconv: "stdcall")
pub const WriteFile -> fn (
    hFile: int,
    lpBuffer: *void,
    nNumberOfBytesToWrite: int,
    lpNumberOfBytesWritten: *int,
    lpOverlapped: *void
) int;
```

`#dll_import` can also appear inside `@os { "windows" -> { ... } }` blocks so the same source file compiles cross-platform:

```luma
@os {
    "windows" -> {
        #dll_import("kernel32.dll", callconv: "stdcall")
        pub const GetStdHandle -> fn (nStdHandle: int) int;
    }
}
```

### Using libc

The `std_libc` module wraps the C standard library (stdio, stdlib, string, and math) and is the easiest way to call into libc from Luma:

```luma
@module "main"

@use "std_libc" as c

pub const main -> fn (argc: int, argv: **byte) int {
    c::puts("hello from libc");

    let n: int = c::atoi("42");
    let r: double = c::sqrt(144.0);

    return 0;
}
```

---

## Built-in Functions

Luma provides several built-in functions that are always available without imports.

### Output Functions

```luma
output(...)      // Print values without newline
outputln(...)    // Print values with newline
```

Both functions are **variadic** - they accept any number of arguments of any type:

```luma
const main -> fn (argc: int, argv: **byte) int {
    output("Hello", " ", "World");           // Hello World
    outputln("The answer is:", 42);          // The answer is: 42\n
    
    let x: int = 10;
    let y: float = 3.14;
    outputln("x = ", x, ", y = ", y);       // x = 10, y = 3.14\n
    
    return 0;
}
```

### Input Functions

```luma
input<T>(prompt: *byte) -> T    // Read typed input
```

`input<T>` parses and typechecks — the intended shape is generic, reading a value of the specified type:

```luma
pub const main -> fn (argc: int, argv: **byte) int {
    let name: *byte = input<*byte>("Enter your name: ");
    let age: int = input<int>("Enter your age: ");
    let height: double = input<double>("Enter height (meters): ");

    outputln("Name: ", name);
    outputln("Age: ", age);
    outputln("Height: ", height);

    return 0;
}
```

**Not implemented yet**: codegen currently ignores the prompt and stdin entirely and evaluates every `input<T>(...)` to a zeroed `T` — the program above compiles but `age`/`height` will read as `0`, not whatever's typed.

### System Commands

```luma
system(command: *byte) -> int    // Execute system command
```

Execute shell commands from your program:

```luma
const main -> fn (argc: int, argv: **byte) int {
    system("clear");  // Clear terminal (Linux/Mac)
    system("stty -icanon -echo");  // Configure terminal
    return 0;
}
```

### Type Information

```luma
sizeof<T> -> int    // Size of type in bytes
```

Get the size of any type at compile time:

```luma
const main -> fn (argc: int, argv: **byte) int {
    outputln("int: ", sizeof<int>);           // 8
    outputln("byte: ", sizeof<byte>);         // 1
    outputln("double: ", sizeof<double>);     // 8
    
    // Use in allocations
    let buffer: *int = cast<*int>(alloc(100 * sizeof<int>));
    defer free(buffer);
    
    return 0;
}
```

---

## Type Casting System

Luma uses explicit casting with the `cast<T>()` function for all type conversions.

### Basic Syntax

```luma
cast<TargetType>(expression)
```

### Numeric Conversions

```luma
const main -> fn (argc: int, argv: **byte) int {
    // Integer to float
    let i: int = 42;
    let f: float = cast<float>(i);        // 42.0
    
    // Float to integer (truncates)
    let pi: double = 3.14159;
    let rounded: int = cast<int>(pi);     // 3
    
    // Between integer types
    let small: byte = cast<byte>(65);     // 'A'
    let large: int = cast<int>(small);    // 65
    
    return 0;
}
```

### Pointer Casting

```luma
const main -> fn (argc: int, argv: **byte) int {
    // void* to typed pointer
    let raw: *void = alloc(sizeof<int>);
    let typed: *int = cast<*int>(raw);
    *typed = 42;
    free(raw);
    
    // Between pointer types
    let int_ptr: *int = cast<*int>(alloc(sizeof<int>));
    let void_ptr: *void = cast<*void>(int_ptr);
    free(int_ptr);
    
    return 0;
}
```

### Pointer to Integer (and back)

```luma
const main -> fn (argc: int, argv: **byte) int {
    let ptr: *byte = cast<*byte>(alloc(10));
    defer free(ptr);
    
    // Pointer to integer
    let addr: int = cast<int>(ptr);
    
    // Add offset (pointer arithmetic)
    let offset_addr: int = addr + 5;
    
    // Back to pointer
    let offset_ptr: *byte = cast<*byte>(offset_addr);
    
    return 0;
}
```

---

## Array Types

Luma supports fixed-size arrays with compile-time known sizes.

### Array Declaration

```luma
// Syntax: [Type; Size]
let numbers: [int; 10];           // Array of 10 integers
let bytes: [byte; 256];           // Array of 256 bytes
let buffer: [double; 100];        // Array of 100 doubles

// Constants can be arrays too
const PRIMES: [int; 5] = [2, 3, 5, 7, 11];
```

### Array Initialization

```luma
const main -> fn (argc: int, argv: **byte) int {
    // Uninitialized (contains garbage)
    let data: [int; 5];
    
    // Initialize with literal
    let primes: [int; 5] = [2, 3, 5, 7, 11];
    
    // Initialize element by element
    let scores: [int; 3];
    scores[0] = 95;
    scores[1] = 87;
    scores[2] = 92;
    
    return 0;
}
```

### Array Access

```luma
const main -> fn (argc: int, argv: **byte) int {
    let numbers: [int; 5] = [10, 20, 30, 40, 50];
    
    // Read elements
    let first: int = numbers[0];    // 10
    let last: int = numbers[4];     // 50
    
    // Write elements
    numbers[2] = 99;
    
    // Loop through array
    loop [i: int = 0](i < 5) : (++i) {
        outputln("numbers[", i, "] = ", numbers[i]);
    }
    
    return 0;
}
```

---

## String Literals and String Types

### String Literals

String literals are null-terminated byte arrays:

```luma
const main -> fn (argc: int, argv: **byte) int {
    // String literal - type is *byte
    let message: *byte = "Hello, World!";
    outputln(message);
    
    return 0;
}
```

### Character Literals

Single characters use single quotes:

```luma
const main -> fn (argc: int, argv: **byte) int {
    let letter: byte = 'A';           // Character literal
    let newline: byte = '\n';         // Escape sequence
    let tab: byte = '\t';             // Tab character
    
    return 0;
}
```

### Escape Sequences

```luma
'\n'   // Newline
'\r'   // Carriage return
'\t'   // Horizontal tab
'\\'   // Backslash
'\''   // Single quote
'\"'   // Double quote
'\0'   // Null character
```

`\xHH` (hexadecimal byte, e.g. `"\x1b"` for ESC) is supported in **string** literals but not in single-quoted character literals yet — `let esc: byte = '\x1b';` fails to parse; use a string (`"\x1b"`) and index into it, or write the decimal/`cast<byte>(...)` form instead.

---

## Pointer Arithmetic

Luma supports pointer arithmetic for low-level memory manipulation.

### Basic Pattern

```luma
const main -> fn (argc: int, argv: **byte) int {
    let arr: *int = cast<*int>(alloc(5 * sizeof<int>));
    defer free(arr);
    
    // Initialize
    loop [i: int = 0](i < 5) : (++i) {
        arr[i] = i * 10;
    }
    
    // Pointer arithmetic: convert to int, add offset, convert back
    let addr: int = cast<int>(arr);
    let new_addr: int = addr + (2 * sizeof<int>);
    let new_ptr: *int = cast<*int>(new_addr);
    
    outputln(*new_ptr);  // arr[2] = 20
    
    return 0;
}
```

---

## Visibility and Access Control

### Public Module Members

Use `pub` to export items from a module:

```luma
@module "math"

// Public - accessible from other modules
pub const PI: double = 3.14159265359;

pub const sqrt -> fn (x: double) double {
    return x;
}

// Private - only within this module
const INTERNAL_CONSTANT: int = 42;
```

### Struct Access Control

```luma
const Person -> struct {
pub:
    name: *byte,
    age: int,
    
priv:
    ssn: *byte,
    internal_id: int
};
```

---

## Memory Management

Luma provides explicit memory management with safety-oriented features.

### Basic Memory Operations

```luma
alloc(size: int) -> *void    // Allocate memory
free(ptr: *void)             // Deallocate memory
sizeof<T> -> int             // Size of type
```

### Example Usage

```luma
const main -> fn (argc: int, argv: **byte) int {
    // Allocate memory
    let ptr: *int = cast<*int>(alloc(sizeof<int>));
    
    // Use the memory
    *ptr = 42;
    outputln("Value: ", *ptr);
    
    // Clean up
    free(ptr);
    return 0;
}
```

### The `defer` Statement

Ensure cleanup with `defer` statements that execute when leaving scope:

```luma
const process_data -> fn () void {
    let buffer: *int = cast<*int>(alloc(100 * sizeof<int>));
    defer free(buffer);  // Guaranteed to run when function exits
    
    let file: *File = open_file("data.txt");
    defer close_file(file);  // Will run even if early return
    
    // Complex processing...
    if (error_condition) {
        return; // defer statements still execute
    }
    
    // More processing...
    // defer statements execute here automatically
}
```

Multiple statements can be deferred:

```luma
defer {
    close_file(file);
    cleanup_resources();
    log("Operation completed");
}
```

**Key Benefits:**
- Ensures cleanup code runs regardless of how the function exits
- Keeps allocation and deallocation code close together
- Prevents resource leaks from early returns
- Executes in reverse order (LIFO - Last In, First Out)

### Ownership Transfer Attributes

Luma provides function attributes that document and enforce ownership semantics.

#### `#returns_ownership`

Marks functions that allocate and return pointers:

```luma
#returns_ownership
const create_buffer -> fn (size: int) *int {
    let buffer: *int = cast<*int>(alloc(size * sizeof<int>));
    return buffer;  // Caller now owns this memory
}

const main -> fn (argc: int, argv: **byte) int {
    let data: *int = create_buffer(100);
    defer free(data);  // Caller must free
    return 0;
}
```

#### `#takes_ownership`

Marks functions that take ownership of pointer arguments:

```luma
#takes_ownership
const consume_buffer -> fn (buffer: *int) void {
    outputln("Processing: ", *buffer);
    free(buffer);  // Function owns and frees the buffer
}

const main -> fn (argc: int, argv: **byte) int {
    let data: *int = cast<*int>(alloc(sizeof<int>));
    *data = 42;
    
    consume_buffer(data);  // Ownership transferred
    // Note: do not use `data` after this point
    
    return 0;
}
```

### Static Memory Analysis

Luma's compiler includes a static analyzer that tracks memory at compile time to prevent common memory management errors.

#### What the Analyzer Tracks

**Verified at Compile Time:**
- **Memory Leaks**: Detects `alloc()` calls without corresponding `free()`
- **Double-Free**: Prevents freeing the same pointer twice
- **Use-After-Free**: Catches access to freed memory within the same function
- **Ownership Transfer**: Validates `#returns_ownership` and `#takes_ownership` annotations
- **Defer Statement Cleanup**: Ensures deferred frees execute properly

**How It Works:**

```luma
const good_memory_usage -> fn () void {
    let ptr: *int = cast<*int>(alloc(sizeof<int>));
    defer free(ptr);  // Analyzer confirms cleanup
    *ptr = 42;
}  // No leak reported

const bad_memory_usage -> fn () void {
    let ptr: *int = cast<*int>(alloc(sizeof<int>));
    *ptr = 42;
    // Compiler error: memory leak - ptr never freed
}

#returns_ownership
const create_buffer -> fn (size: int) *int {
    let buffer: *int = cast<*int>(alloc(size));
    return buffer;  // Ownership transferred to caller
}  // No leak reported - caller is responsible

const main -> fn (argc: int, argv: **byte) int {
    let data: *int = create_buffer(100);
    defer free(data);  // Caller properly handles ownership
    return 0;
}
```

**Ownership Tracking:**

The analyzer understands three ownership patterns:

1. **`#returns_ownership` functions**: Allocations inside are NOT tracked as leaks because ownership transfers to the caller
2. **`#takes_ownership` functions**: Parameters marked with this receive ownership and are responsible for cleanup
3. **`defer` statements**: Deferred cleanup is tracked and validated at function exit

**Transitive Ownership:**

```luma
#returns_ownership
const create_arena_sized -> fn (size: int) Arena {
    let a: Arena;
    a.buf = alloc(size);  // Not tracked - inside #returns_ownership
    return a;
}

const create_arena -> fn () Arena {
    return create_arena_sized(1024);
    // Warning: Should add #returns_ownership annotation
    // for API clarity (ownership is being passed through)
}
```

#### Current Limitations

**Known Edge Cases:**

The analyzer currently has limitations in these areas:

1. **Struct Field Granularity**: When tracking `a.buf = alloc(...)`, the analyzer tracks the entire struct `a`, not the specific field `a.buf`. This works for single-pointer structs but may cause issues with:
   ```luma
   const Container -> struct {
       data1: *int,
       data2: *int
   };
   
   let c: Container;
   c.data1 = alloc(10);  // Tracked as "c"
   c.data2 = alloc(20);  // Also tracked as "c" - potential confusion
   free(c.data1);        // Marks "c" as freed, but c.data2 still allocated
   ```

2. **Conditional Allocations**: The analyzer may report false positives for conditional paths:
   ```luma
   let ptr: *int;
   if (condition) {
       ptr = alloc(sizeof<int>);
   }
   // May warn even if you don't need to free in else branch
   ```

3. **Allocations in Loops**: Each loop iteration's allocations should be independent, but edge cases may exist:
   ```luma
   loop [i: int = 0](i < 10) : (++i) {
       let temp: *int = alloc(4);
       // Use temp...
       free(temp);  // Should work correctly
   }
   ```

4. **Early Returns with Defer**: While generally working, complex control flow with multiple early returns may need testing:
   ```luma
   const process -> fn () int {
       let a: *int = alloc(sizeof<int>);
       defer free(a);
       
       if (error) { return -1; }  // Defer should fire
       if (warning) { return 0; } // Defer should fire
       return 1;                  // Defer should fire
   }
   ```

5. **Stack vs Heap**: The analyzer doesn't currently detect returning pointers to stack variables:
   ```luma
   const dangerous -> fn () *int {
       let local: int = 42;
       return &local;  // NOT DETECTED - returns dangling pointer
   }
   ```

6. **Arrays of Pointers**: Complex allocation patterns may not be fully tracked:
   ```luma
   let arr: [*int; 5];
   loop [i: int = 0](i < 5) : (++i) {
       arr[i] = alloc(sizeof<int>);  // Each needs individual free
   }
   ```

#### Best Practices

To work effectively with the analyzer:

1. **Use ownership annotations consistently**: Mark all functions that allocate and return resources with `#returns_ownership`
2. **Use defer for cleanup**: Always pair allocations with `defer free()` for automatic cleanup
3. **One allocation per variable**: Avoid reassigning pointer variables after allocation without freeing
4. **Clear ownership semantics**: Document which functions own their pointer parameters vs. borrowing them
5. **Test early returns**: Ensure defer statements properly handle all exit paths

The analyzer is conservative - it may report false positives to prevent missed leaks. When in doubt, it will warn about potential issues rather than silently allowing them.

## Performance

Understanding performance is crucial for systems programming.

### Zero-Cost Abstractions

Luma follows the "zero-cost abstraction" principle: abstractions should have no runtime overhead.

**Generics are designed to be zero-cost** (see [Generics](#generics-not-yet-supported) — this is the design intent, not implemented yet):
```luma
const add = fn<T>(a: T, b: T) T {
    return a + b;
}

// These calls would compile to separate, optimized functions:
let x: int = add<int>(1, 2);        // Same as: x = 1 + 2
let y: float = add<float>(1.0, 2.0); // Same as: y = 1.0 + 2.0
```

**No runtime dispatch** - once implemented, generic instantiations are meant to be resolved at compile time through monomorphization, the same way it already works for structs and switch cases today.

### Monomorphization

The plan is for Luma to generate specialized code per type, the same way C++ templates or Rust generics do (also not implemented yet):

```luma
const max = fn<T>(a: T, b: T) T {
    if (a > b) { return a; }
    return b;
}

// Compiler would generate:
// max_int(a: int, b: int) -> int { ... }
// max_float(a: float, b: float) -> float { ... }
```

**Benefits:**
- No runtime overhead
- Full optimization per type
- No vtables or dynamic dispatch

**Trade-offs:**
- Larger binary size (one copy per type)
- Longer compilation time

### Memory Layout

**Struct layout is predictable:**
```luma
const Point -> struct {
    x: int,    // Offset 0, 8 bytes
    y: int     // Offset 8, 8 bytes
};  // Total: 16 bytes
```

**Array layout is contiguous:**
```luma
let arr: [int; 10];  // 80 contiguous bytes
// arr[0] at offset 0, arr[1] at offset 8, arr[2] at offset 16...
```

### Memory Allocation Performance

**Stack allocation is fast:**
```luma
const fast_function -> fn () void {
    let buffer: [int; 1024];  // Stack allocated - instant
    // Use buffer...
}  // Automatically cleaned up
```

**Heap allocation has overhead:**
```luma
const slower_function -> fn () void {
    let buffer: *int = cast<*int>(alloc(1024 * sizeof<int>));
    defer free(buffer);
    // Use buffer...
}
```

### Optimization Guidelines

**1. Prefer stack allocation when possible:**
```luma
let temp: [int; 100];  // Good for small, fixed-size data
```

**2. Minimize pointer indirection:**
```luma
// Better: direct access
let ptr: *int;
let value: int = *ptr;   // One memory load

// Best: value directly
let value2: int = 42;     // No memory load
```

**3. Batch operations:**
```luma
// Good: one large allocation
let buffer: *int = cast<*int>(alloc(1000 * sizeof<int>));
loop [i: int = 0](i < 1000) : (++i) {
    // Use buffer[i]...
}
free(buffer);
```

**4. Avoid unnecessary copying:**
```luma
// Bad: pass large struct by value
const process -> fn (data: LargeStruct) void { }

// Good: pass by pointer
const process_fast -> fn (data: *LargeStruct) void { }
```

### Performance Summary

| Operation | Cost | Notes |
|-----------|------|-------|
| Stack variable | ~0 | Instant |
| Heap allocation | High | System call |
| Pointer dereference | Low | One memory access |
| Array index | Low | Bounds check + access |
| Function call | Low-Medium | Depends on size |
| Generic instantiation | 0 | Compile-time only |
| Struct field access | Low | Offset calculation |
| Enum comparison | ~0 | Integer comparison |

---

## Safety Features

Luma provides several safety features to prevent common bugs:

### 1. Static Memory Analysis

- Tracks allocations and deallocations at compile time
- Detects memory leaks before runtime
- Prevents double-free errors
- Identifies use-after-free bugs

### 2. Ownership Attributes

- `#returns_ownership` documents memory transfers
- `#takes_ownership` clarifies responsibility
- Compiler enforces ownership rules

### 3. Defer Statements

- Guarantees cleanup code execution
- Prevents resource leaks
- Works with early returns and errors

### 4. Strong Type System

- No implicit conversions (except safe ones)
- Explicit casting required
- Type-safe generics
- Enum exhaustiveness checking

### 5. Explicit Error Handling

- No hidden exceptions
- Clear error propagation
- Predictable control flow

---

## Quick Reference

### Keywords

```
const      let        if         elif       else
loop       break      continue   return     defer
struct     enum       pub        priv       cast
sizeof     alloc      free       switch     fn
using      static     input      system     as
```

### Directives

```
@module "name"              // Declare module name
@use "name" as alias        // Import module
@os { "linux" -> { } }      // Platform-conditional code
@link("lib.so")             // Link against shared library (module-level)
```

### Attributes

```
#returns_ownership          // Function returns allocated memory (caller must free)
#takes_ownership            // Function takes ownership of a pointer argument
#lib_import("lib.so")       // Per-function library override (POSIX)
#dll_import("dll", callconv: "stdcall")  // Per-function DLL import (Windows)
```

### Operators

```
Arithmetic:  +  -  *  /  %  ++  --
Comparison:  ==  !=  <  >  <=  >=
Logical:     &&  ||  !
Bitwise:     &  |  ^  ~  <<  >>
Assignment:  =
Access:      .   ::  []  *  &
```

### Primitive Types

```
int     double    bool    *T      [T; N]
uint    float     byte    void
```

### Common Patterns

```luma
// Allocation with cleanup
let ptr: *T = cast<*T>(alloc(sizeof<T>));
defer free(ptr);

// Array iteration
loop [i: int = 0](i < size) : (++i) {
    array[i] = value;
}

// Error checking
if (ptr == cast<*T>(0)) {
    return ERROR_CODE;
}

// Module usage
@use "std_module" as m
let x: int = m::function();

// FFI — link a shared library (POSIX)
@link("libsomething.so")
pub const some_fn -> fn (x: int) int;

// FFI — per-function library override
#lib_import("libm.so")
pub const sqrt -> fn (x: double) double;

// FFI — Windows DLL
#dll_import("user32.dll", callconv: "stdcall")
pub const MessageBoxA -> fn (hwnd: int, text: *byte, caption: *byte, utype: int) int;

// Platform-conditional code
@os {
    "linux"   -> { outputln("Linux"); }
    "macos"   -> { outputln("macOS"); }
    "windows" -> { outputln("Windows"); }
}
```

---

## Conclusion

Luma is a modern systems programming language that provides:

- **Simplicity**: Clean, consistent syntax
- **Safety**: Static analysis and ownership tracking
- **Performance**: Zero-cost abstractions and predictable behavior
- **Control**: Manual memory management with safety nets
- **Interoperability**: First-class FFI via `@link`, `#lib_import`, and `#dll_import`
- **Portability**: `@os` blocks for clean cross-platform code

The language is designed for programmers who want the performance and control of C with modern safety features and ergonomics.

For more examples, see the standard library modules and test files included with the language distribution.
