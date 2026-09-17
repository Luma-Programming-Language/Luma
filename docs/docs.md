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
    x: i64,
    y: i64,

    distance_to -> fn (other: Point) f32 {
        let dx: i64 = other.x - self.x;
        let dy: i64 = other.y - self.y;
        return cast<f32>(sqrt(cast<f64>(dx * dx + dy * dy)));
    },
};

const Status -> enum {
    Active,
    Inactive,
    Pending,
};

pub const main -> fn (argc: i64, argv: **byte) i64 {
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

### Primitive Types (Quick Reference)

```text
i8       - Signed 8-bit integer
i16      - Signed 16-bit integer
i32      - Signed 32-bit integer (default type of integer literals)
i64      - Signed 64-bit integer
u8       - Unsigned 8-bit integer
u16      - Unsigned 16-bit integer
u32      - Unsigned 32-bit integer
u64      - Unsigned 64-bit integer
f32      - Single-precision floating point (32-bit, default type of float literals)
f64      - Double-precision floating point (64-bit)
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

```text
*T       - Pointer type (declares a pointer to type T)
[T; N]   - Array type (fixed-size array of N elements of type T)
```

**Pointer Operators:**

```text
*expr    - Dereference operator (access value pointed to)
&expr    - Address-of operator (get pointer to value)
```

**Example:**

```luma
let x: i64 = 42;           // x is an i64
let ptr: *i64 = &x;        // ptr is a pointer to i64, holds address of x
let value: i64 = *ptr;     // value is 42 (dereferenced ptr)
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

// Can cast to i64 if needed
let dir_value: i64 = cast<i64>(Direction::North);  // 0
```

### Structures

Structures group related data with optional access control:

```luma
const Point -> struct {
    x: i64,
    y: i64
};

// With explicit access modifiers and methods
const Player -> struct {
pub:
    name: *byte,
    score: i64,

    // Methods can be defined inside structs — fields are reached through
    // `self`, never as bare names. Currently, a method must be declared in
    // the `pub:` group to typecheck correctly — one declared after `priv:`
    // doesn't get `self` resolved (a known compiler bug).
    get_info -> fn () void {
        outputln("Player: ", self.name, " Score: ", self.score);
    },
priv:
    internal_id: i64,
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
const move_point -> fn (p: *Point, dx: i64, dy: i64) void {
    p.x = p.x + dx;   // not p->x
    p.y = p.y + dy;
}

pub const main -> fn (argc: i64, argv: **byte) i64 {
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

pub const main -> fn (argc: i64, argv: **byte) i64 {
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
    x: f32,
    y: f32,

    move -> fn (dx: f32, dy: f32) void {
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
    move -> fn (dx: f32, dy: f32) void {
        self.x = self.x + (dx * 2.0);
        self.y = self.y + (dy * 2.0);
    },
};

pub const main -> fn (argc: i64, argv: **byte) i64 {
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

    move -> fn (dx: f32, dy: f32) void {
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
    x: f32,
    y: f32,

    static origin -> fn () Point {
        return Point { x: 0.0, y: 0.0 };
    },

    static at -> fn (x: f32, y: f32) Point {
        return Point { x: x, y: y };
    },

    move -> fn (dx: f32, dy: f32) void {
        self.x = self.x + dx;
        self.y = self.y + dy;
    },
};

pub const main -> fn (argc: i64, argv: **byte) i64 {
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
    age: i64,

    #returns_ownership
    static create -> fn (name: *byte, age: i64) *Person {
        let p: *Person = cast<*Person>(alloc(sizeof<Person>));
        p.name = name;
        p.age = age;
        return p;
    },

    destroy -> fn () void {
        free(self.name);
    },
};

pub const main -> fn (argc: i64, argv: **byte) i64 {
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
let x: i64 = 42;
let y: i64 = x;  // OK

// Different types require explicit cast
let f: f32 = cast<f32>(x);  // OK
let z: i64 = f;  // ERROR: must use cast<i64>(f)

// Pointer type safety
let int_ptr: *i64 = &x;
let void_ptr: *void = cast<*void>(int_ptr);  // Explicit cast required
```

See [Type Casting System](#type-casting-system) for full details on conversions.

---

## Generics

**Generic functions, generic structs (`struct<T>`) and generic enums
(`enum<T>`) are implemented.** Generic structs and enums use type parameters
in angle brackets, such as `struct<T>` and `enum<T>`.

### Generic Functions

Generic functions are declared with type parameters in angle brackets `<>`
right after the `fn` keyword, same position as every other function
declaration's parameter list:

```luma
const add -> fn<T> (a: T, b: T) T {
    return a + b;
}

const swap -> fn<T> (a: *T, b: *T) void {
    let temp: T = *a;
    *a = *b;
    *b = temp;
}

const identity -> fn<T> (x: T) T {
    return x;
}
```

### Using Generic Functions

Generic functions require **explicit type arguments** at the call site —
`add<i64>(1, 2)`, not just `add(1, 2)`:

```luma
pub const main -> fn (argc: i64, argv: **byte) i64 {
    // Integer arithmetic
    outputln("add(1, 2) = ", add<i64>(1, 2));

    // Floating-point arithmetic
    outputln("add(1.5, 2.5) = ", add<f32>(1.5, 2.5));

    // Swapping integers
    let x: i64 = 5;
    let y: i64 = 10;
    swap<i64>(&x, &y);
    outputln("After swap: x = ", x, ", y = ", y);

    // identity<*byte> and identity<i64> are two independent instantiations
    let s: *byte = identity<*byte>("hello");
    let n: i64 = identity<i64>(42);

    return 0;
}
```

Why explicit, rather than inferring `T` from the arguments the way most
languages with generics do: `add<i64>(...)` and `a < i64 > (...)` (a chained
comparison) are genuinely ambiguous to parse — Luma's parser has no symbol
table, so it can't tell "add" apart from an ordinary variable at parse time.
It resolves this the same way C++ effectively does: on seeing `ident <`, it
*speculatively* tries to parse a type-argument list followed immediately by
`(` or `{`; if that doesn't parse cleanly, it rolls back with zero side
effects and falls through to ordinary `<` (less-than). This is also why the
type arguments are mandatory rather than optional — inference would remove
the very shape (`<Type,...>(`) the parser depends on to disambiguate.

### Performance Monomorphization

Luma uses **monomorphization**: the compiler generates a separate, concrete
function for each distinct set of type arguments actually used, the first
time it's used — not type erasure, and not a runtime dispatch of any kind.

```luma
const identity -> fn<T> (x: T) T { return x; }

// These calls generate two independent, separately-typechecked functions:
let a: i64   = identity<i64>(42);        // -> identity__int
let c: *byte = identity<*byte>("hello"); // -> identity___byte
```

A generic function's body is duck-typed, like a C++ template: it's only
fully typechecked once per concrete instantiation, not at the generic
declaration itself. `add<T>`'s `a + b` isn't checked against every possible
`T` up front — `add<*byte>(...)` would fail with an ordinary Type Error
right at that call site (`+` isn't defined for `*byte`), the same as if
you'd written a `*byte`-specific function with `+` in it directly.

### Generic Structs

Structs can carry generic fields, declared with type parameters in angle
brackets right after the `struct` keyword:

```luma
const Entry -> struct<K, V> {
    key: K,
    val: V,
};

const Map -> struct<K, V> {
    size: i64,
    first: *Entry<K, V>,
};
```

A concrete struct is named with explicit type arguments in **type position**,
the same `Name<T, ...>` shape generic calls use:

```luma
let e: Entry<i64, i64> = Entry<i64, i64> { key: 7, val: 9 };  // type + literal
```

Both the type reference and the generic struct literal
(`Entry<i64, i64> { ... }`) monomorphize the template to a concrete
`Entry__int__int`. Fields that reference generic types (`*Entry<K, V>` inside
`Map`) are substituted with the caller's concrete types during
instantiation, so nested generics work.

#### Methods on Generic Structs

Methods declared inside the struct body inherit the struct's type
parameters — no need to redeclare them on the method:

```luma
const Box -> struct<K, V> {
    content: *Entry<K, V>,
    cap: u64,

    #returns_ownership
    static make -> fn (key: K, value: V) *Box<K, V> {
        let b: *Box<K, V> = cast<*Box<K, V>>(alloc(sizeof<Box<K, V>> * 1));
        b.content = cast<*Entry<K, V>>(alloc(sizeof<Entry<K, V>>));
        b.content.key = key;
        b.content.value = value;
        b.cap = sizeof<Entry<K, V>>;
        return b;
    },

    get_key -> fn () K { return self.content.key; },
};
```

A static method is called with explicit type arguments, as a function would
be: `Box::make<i64, *byte>(7, "seven")`. `Box<K, V>` stays valid in type
position inside the template itself (`sizeof<Box<K, V>>`, `*Box<K, V>`),
substituted and mangled per instantiation.

### Generic Enums

Enums can take type parameters too, right after the `enum` keyword:

```luma
const State -> enum<T> {
    Idle,
    Running,
    Done,
};
```

Because Luma enums are plain C-style name constants — they carry no
associated data — there is no field of type `T` to store. The type
parameter instead acts as a **phantom/type tag**: `State<i64>` and
`State<*byte>` are distinct nominal types, each with its own set of
(mangled) member constants, generated by the same monomorphization that
drives generic structs.

Members are written with the explicit type arguments on the left of the
`::`, matching the `Name::Member` syntax ordinary enums use:

```luma
let s: State<i64> = State<i64>::Idle;

switch (s) {
    State<i64>::Idle    -> { /* ... */ }
    State<i64>::Running -> outputln("running");
    State<i64>::Done    -> return 0;
}
```

Type compatibility is **per-instantiation**: the phantom argument is part of
the enum's identity. A `State<i64>` value isn't assignable to a `State<*byte>`
variable without an explicit cast — they name different nominal types
(`State__int` vs `State___byte`, following the same mangling as generic
functions). This makes a generic enum a safe way to tag a value with which
concrete types it belongs to.

### Constraints

- **Explicit type arguments are mandatory** everywhere — functions, structs,
  and enums — both because Luma's parser has no symbol table (it can't infer
  from context at parse time) and because the explicit `<Type,...>` shape is
  what lets `<` be disambiguated from a chained comparison. A bare `State`
  (no arguments) doesn't name any instantiation and is an error.
- **Right arity is required** — naming more or fewer type arguments than the
  template declares is a `Generics Error` at the reference site.
- **Cross-module works** — an imported generic struct, enum, or function is
  referenced as `ALIAS::Name<T,...>` / `ALIAS::Name::make<T>(...)`, and every
  concrete instantiation is codegen'd in the module that uses it (mangled
  `MOD__Name__T1__T2`, so the same template can be instantiated from several
  modules).
- **Generic methods on plain structs** — a non-generic struct may declare a
  method with its own type parameters (`static make -> fn<T> (...) T`).
  These are instantiated per call site exactly like a generic function
  (`Thing::make<*Thing>` → `Thing_make___Thing`); the raw template is never
  emitted to C, and the type parameters it owns must not escape to module
  scope (that was the "unknown type name" bug).

---

## Top-Level Bindings with `const`

Luma uses the `const` keyword as a **unified declaration mechanism** for all top-level bindings. Whether you're declaring variables, functions, types, or enums, `const` provides a consistent syntax that enforces immutability at the binding level.

### Declaration Examples

```luma
const NUM: i64 = 42;                                  // Immutable variable
const Direction -> enum { North, South, East, West };  // Enum definition
const Point -> struct { x: i64, y: i64 };              // Struct definition
const add -> fn (a: i64, b: i64) i64 {                 // Function definition
    return a + b;
}
```

(`fn<T>`, `struct<T>` and `enum<T>` bindings work today — see [Generics](#generics).)

### Why This Design?

**Unified syntax**: One parsing rule handles all top-level declarations, simplifying both the compiler and developer experience.

**Semantic clarity**: The binding itself is immutable—you cannot reassign or shadow a top-level `const`. This prevents accidental redefinition bugs.

**Compiler optimization**: Immutable bindings enable better optimization opportunities.

**Future extensibility**: This approach naturally supports compile-time metaprogramming and uniform import behavior.

### Important Notes

```luma
const x: i64 = 5;
x = 10; // Error: `x` is immutable

const add -> fn (a: i64, b: i64) i64 { return a + b; }
add = something_else; // Error: cannot reassign function binding
```

---

## Variables and Mutability

Inside functions, use `let` to declare local variables:

```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    let x: i64 = 10;        // Mutable local variable
    x = 20;                 // Can be reassigned

    let y: i64 = 5;
    y = y + 1;              // Can be modified

    let counter: i64 = 0;
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
const add -> fn (a: i64, b: i64) i64 {
    return a + b;
}

// Function with no parameters
const greet -> fn () void {
    outputln("Hello!");
}

// Function with no return value
const print_number -> fn (n: i64) void {
    outputln("Number: ", n);
}
```

A function declaration's closing `}` never takes a trailing `;` — that's only for `struct`/`enum` declarations and ordinary statements (see [Top-Level Bindings with `const`](#top-level-bindings-with-const)).

### main()

`main` can declare zero, one, or two parameters — `argc: i64` and `argv: **byte`, in that order. Its C-level signature is always `int main(int, char**)` regardless; declaring fewer just leaves the rest unnamed on the Luma side.

```luma
pub const main -> fn () i64 {
    return 0;
}

pub const main -> fn (argc: i64) i64 {
    return 0;
}

pub const main -> fn (argc: i64, argv: **byte) i64 {
    return 0;
}
```

### Function Calls

```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    let result: i64 = add(5, 3);
    outputln("5 + 3 = ", result);

    greet();
    print_number(42);

    return 0;
}
```

### Function Parameters

Parameters are passed by value by default:

```luma
const modify -> fn (x: i64) void {
    x = 100;  // Modifies local copy only
}

const main -> fn (argc: i64, argv: **byte) i64 {
    let num: i64 = 10;
    modify(num);
    outputln(num);  // Still 10
    return 0;
}
```

To modify the caller's variable, use pointers:

```luma
const modify_ptr -> fn (x: *i64) void {
    *x = 100;  // Modifies original value
}

const main -> fn (argc: i64, argv: **byte) i64 {
    let num: i64 = 10;
    modify_ptr(&num);  // Pass address
    outputln(num);     // Now 100
    return 0;
}
```

### Return Values

```luma
// Single return value
const square -> fn (x: i64) i64 {
    return x * x;
}

// Multiple return values via struct
const DivResult -> struct {
    quotient: i64,
    remainder: i64
};

const divide -> fn (a: i64, b: i64) DivResult {
    return DivResult {
        quotient: a / b,
        remainder: a % b
    };
}

const main -> fn (argc: i64, argv: **byte) i64 {
    let result: DivResult = divide(17, 5);
    outputln("17 / 5 = ", result.quotient, " R ", result.remainder);
    return 0;
}
```

### Early Returns

```luma
const find_positive -> fn (numbers: *i64, size: i64) i64 {
    loop [i: i64 = 0](i < size) : (++i) {
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
let distance: f32 = origin.distance_to(destination);
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
const x: i64 = 7;

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
loop [i: i64 = 0](i < 10) {
    outputln("Iteration: ", i);
    ++i;
}

// For loop with post-increment
loop [i: i64 = 0](i < 10) : (++i) {
    outputln("i = ", i);
}

```

**Not currently working**: multiple loop-init variables (`loop [i: i64 = 0, j: i64 = 0](...) ...`) parse but generate invalid C (`for (long long i = 0, long long j = 0; ...)`, which repeats the type where C expects a bare `j = 0`) — the build fails at the C compile step. For now, declare the second counter with `let` above the loop instead:

```luma
let j: i64 = 0;
loop [i: i64 = 0](i < 10) : (++i) {
    outputln("i = ", i, ", j = ", j);
    ++j;
}
```

#### While-Style Loops

```luma
// Condition-only loop
let counter: i64 = 0;
loop (counter < 5) {
    outputln("Count: ", counter);
    counter = counter + 1;
}

// While loop with post-action
let j: i64 = 0;
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
loop [i: i64 = 0](i < 100) : (++i) {
    if (i == 50) {
        break;  // Exit loop
    }
    process(i);
}

// Continue: skip to next iteration
loop [i: i64 = 0](i < 10) : (++i) {
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

pub const main -> fn (argc: i64, argv: **byte) i64 {
    classify_day(WeekDay::Monday);   // Output: Weekday => 1
    classify_day(WeekDay::Saturday); // Output: Weekend => 6
    return 0;
}
```

### Switch with Default Case

When you need to handle unexpected values or want a catch-all case, use the default wildcard pattern `_`:

```luma
const handle_status_code -> fn (code: i64) void {
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

pub const main -> fn (argc: i64, argv: **byte) i64 {
    // Access imported functions with namespace
    let result: f64 = c::sqrt(16.0);
    let len: i64 = string::strlen("hello");

    outputln("sqrt(16): ", result);
    outputln("Length: ", len);
    return 0;
}
```

`@use` only declares the dependency — it doesn't locate the file. Every module you `@use` also has to be passed to the compiler explicitly with `-l`:

```sh
luma main.lx -l std/libc.lx std/cstring.lx -name main
```

### Standard Library Module Names

All standard library modules use the `std_` prefix:

```txt
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
        pub const SYS_WRITE: i64 = 1;
        pub const O_CREAT: i64   = 64;
    }
    "macos" -> {
        pub const SYS_WRITE: i64 = 4;
        pub const O_CREAT: i64   = 512;
    }
    "windows" -> {
        // Windows-specific declarations...
    }
}
```

You can also use `@os` inline inside function bodies:

```luma
const write_out -> fn (s: *byte) i64 {
    @os {
        "linux"   -> { return __syscall__(1, 1, cast<i64>(s), len); }
        "macos"   -> { return __syscall__(4, 1, cast<i64>(s), len); }
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

pub const puts    -> fn (s: *byte) i64;
pub const printf  -> fn (fmt: *byte, val: *byte) i64;
pub const malloc  -> fn (size: i64) *void;
// ...
```

The function body is omitted — the linker resolves it from the named library at link time. Only one `@link` per module is needed; it applies to all subsequent `pub const` declarations that have no body.

### `#lib_import` — Per-Function Library Override

When individual functions within a module come from a *different* library than the module-level `@link`, use `#lib_import` as a per-declaration attribute:

```luma
@module "std_libc"

@link("libc.so.6")

// Most functions come from libc.so.6 via @link above:
pub const malloc -> fn (size: i64) *void;
pub const free   -> fn (ptr: *void) void;

// Math functions need libm — override per-function:
#lib_import("libm.so")
pub const sqrt -> fn (x: f64) f64;

#lib_import("libm.so")
pub const pow  -> fn (base: f64, exp: f64) f64;

#lib_import("libm.so")
pub const sin  -> fn (x: f64) f64;
```

Place `#lib_import(...)` on the line immediately before the `pub const` it applies to.

### `#dll_import` — Windows DLL Imports

On Windows, use `#dll_import` instead. It accepts the DLL name and an optional calling convention:

```luma
#dll_import("kernel32.dll", callconv: "stdcall")
pub const CreateFileA -> fn (
    lpFileName: *byte,
    dwDesiredAccess: i64,
    dwShareMode: i64,
    lpSecurityAttributes: *void,
    dwCreationDisposition: i64,
    dwFlagsAndAttributes: i64,
    hTemplateFile: i64
) i64;

#dll_import("kernel32.dll", callconv: "stdcall")
pub const WriteFile -> fn (
    hFile: i64,
    lpBuffer: *void,
    nNumberOfBytesToWrite: i64,
    lpNumberOfBytesWritten: *i64,
    lpOverlapped: *void
) i64;
```

`#dll_import` can also appear inside `@os { "windows" -> { ... } }` blocks so the same source file compiles cross-platform:

```luma
@os {
    "windows" -> {
        #dll_import("kernel32.dll", callconv: "stdcall")
        pub const GetStdHandle -> fn (nStdHandle: i64) i64;
    }
}
```

### Using libc

The `std_libc` module wraps the C standard library (stdio, stdlib, string, and math) and is the easiest way to call into libc from Luma:

```luma
@module "main"

@use "std_libc" as c

pub const main -> fn (argc: i64, argv: **byte) i64 {
    c::puts("hello from libc");

    let n: i64 = c::atoi("42");
    let r: f64 = c::sqrt(144.0);

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
const main -> fn (argc: i64, argv: **byte) i64 {
    output("Hello", " ", "World");           // Hello World
    outputln("The answer is:", 42);          // The answer is: 42\n

    let x: i64 = 10;
    let y: f32 = 3.14;
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
pub const main -> fn (argc: i64, argv: **byte) i64 {
    let name: *byte = input<*byte>("Enter your name: ");
    let age: i64 = input<i64>("Enter your age: ");
    let height: f64 = input<f64>("Enter height (meters): ");

    outputln("Name: ", name);
    outputln("Age: ", age);
    outputln("Height: ", height);

    return 0;
}
```

`input<T>` prints `prompt` (skipped if empty), then reads a line from stdin and
parses it as `T`. Supported `T`: `i64`, `u64`, `f32`, `f64`, `bool`,
`byte`, and `*byte` (string). `byte` is the exception to "a line" — it reads a
single raw byte (via `read(2)`, not line-buffered stdio), which is what
`std/terminal.lx`'s raw-mode key readers (`getch`, `getch_raw`, `getpass`, ...)
rely on. Any other `T` (structs, enums, ...) isn't supported yet and evaluates
to a zeroed value.

### System Commands

```luma
system(command: *byte) -> i64    // Execute system command
```

Execute shell commands from your program:

```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    system("clear");  // Clear terminal (Linux/Mac)
    system("stty -icanon -echo");  // Configure terminal
    return 0;
}
```

### Type Information

```luma
sizeof<T> -> i64    // Size of type in bytes
```

Get the size of any type at compile time:

```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    outputln("i64: ", sizeof<i64>);           // 8
    outputln("byte: ", sizeof<byte>);         // 1
    outputln("f64: ", sizeof<f64>);     // 8

    // Use in allocations
    let buffer: *i64 = cast<*i64>(alloc(100 * sizeof<i64>));
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
const main -> fn (argc: i64, argv: **byte) i64 {
    // Integer to f32
    let i: i64 = 42;
    let f: f32 = cast<f32>(i);        // 42.0

    // Float to integer (truncates)
    let pi: f64 = 3.14159;
    let rounded: i64 = cast<i64>(pi);     // 3

    // Between integer types
    let small: byte = cast<byte>(65);     // 'A'
    let large: i64 = cast<i64>(small);    // 65

    return 0;
}
```

### Pointer Casting

```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    // void* to typed pointer
    let raw: *void = alloc(sizeof<i64>);
    let typed: *i64 = cast<*i64>(raw);
    *typed = 42;
    free(raw);

    // Between pointer types
    let int_ptr: *i64 = cast<*i64>(alloc(sizeof<i64>));
    let void_ptr: *void = cast<*void>(int_ptr);
    free(int_ptr);

    return 0;
}
```

### Pointer to Integer (and back)

```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    let ptr: *byte = cast<*byte>(alloc(10));
    defer free(ptr);

    // Pointer to integer
    let addr: i64 = cast<i64>(ptr);

    // Add offset (pointer arithmetic)
    let offset_addr: i64 = addr + 5;

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
let numbers: [i64; 10];           // Array of 10 integers
let bytes: [byte; 256];           // Array of 256 bytes
let buffer: [f64; 100];        // Array of 100 doubles

// Constants can be arrays too
const PRIMES: [i64; 5] = [2, 3, 5, 7, 11];
```

### Array Initialization

```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    // Uninitialized (contains garbage)
    let data: [i64; 5];

    // Initialize with literal
    let primes: [i64; 5] = [2, 3, 5, 7, 11];

    // Initialize element by element
    let scores: [i64; 3];
    scores[0] = 95;
    scores[1] = 87;
    scores[2] = 92;

    return 0;
}
```

### Array Access

```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    let numbers: [i64; 5] = [10, 20, 30, 40, 50];

    // Read elements
    let first: i64 = numbers[0];    // 10
    let last: i64 = numbers[4];     // 50

    // Write elements
    numbers[2] = 99;

    // Loop through array
    loop [i: i64 = 0](i < 5) : (++i) {
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
const main -> fn (argc: i64, argv: **byte) i64 {
    // String literal - type is *byte
    let message: *byte = "Hello, World!";
    outputln(message);

    return 0;
}
```

### Character Literals

Single characters use single quotes:

```luma
const main -> fn (argc: i64, argv: **byte) i64 {
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
const main -> fn (argc: i64, argv: **byte) i64 {
    let arr: *i64 = cast<*i64>(alloc(5 * sizeof<i64>));
    defer free(arr);

    // Initialize
    loop [i: i64 = 0](i < 5) : (++i) {
        arr[i] = i * 10;
    }

    // Pointer arithmetic: convert to i64, add offset, convert back
    let addr: i64 = cast<i64>(arr);
    let new_addr: i64 = addr + (2 * sizeof<i64>);
    let new_ptr: *i64 = cast<*i64>(new_addr);

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
pub const PI: f64 = 3.14159265359;

pub const sqrt -> fn (x: f64) f64 {
    return x;
}

// Private - only within this module
const INTERNAL_CONSTANT: i64 = 42;
```

### Struct Access Control

```luma
const Person -> struct {
pub:
    name: *byte,
    age: i64,

priv:
    ssn: *byte,
    internal_id: i64
};
```

---

## Memory Management

Luma provides explicit memory management with safety-oriented features.

### Basic Memory Operations

```luma
alloc(size: i64) -> *void    // Allocate memory
free(ptr: *void)             // Deallocate memory
sizeof<T> -> i64             // Size of type
```

### Example Usage

```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    // Allocate memory
    let ptr: *i64 = cast<*i64>(alloc(sizeof<i64>));

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
    let buffer: *i64 = cast<*i64>(alloc(100 * sizeof<i64>));
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
const create_buffer -> fn (size: i64) *i64 {
    let buffer: *i64 = cast<*i64>(alloc(size * sizeof<i64>));
    return buffer;  // Caller now owns this memory
}

const main -> fn (argc: i64, argv: **byte) i64 {
    let data: *i64 = create_buffer(100);
    defer free(data);  // Caller must free
    return 0;
}
```

#### `#takes_ownership`

Marks functions that take ownership of pointer arguments:

```luma
#takes_ownership
const consume_buffer -> fn (buffer: *i64) void {
    outputln("Processing: ", *buffer);
    free(buffer);  // Function owns and frees the buffer
}

const main -> fn (argc: i64, argv: **byte) i64 {
    let data: *i64 = cast<*i64>(alloc(sizeof<i64>));
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
    let ptr: *i64 = cast<*i64>(alloc(sizeof<i64>));
    defer free(ptr);  // Analyzer confirms cleanup
    *ptr = 42;
}  // No leak reported

const bad_memory_usage -> fn () void {
    let ptr: *i64 = cast<*i64>(alloc(sizeof<i64>));
    *ptr = 42;
    // Compiler error: memory leak - ptr never freed
}

#returns_ownership
const create_buffer -> fn (size: i64) *i64 {
    let buffer: *i64 = cast<*i64>(alloc(size));
    return buffer;  // Ownership transferred to caller
}  // No leak reported - caller is responsible

const main -> fn (argc: i64, argv: **byte) i64 {
    let data: *i64 = create_buffer(100);
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
const create_arena_sized -> fn (size: i64) Arena {
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
       data1: *i64,
       data2: *i64
   };

   let c: Container;
   c.data1 = alloc(10);  // Tracked as "c"
   c.data2 = alloc(20);  // Also tracked as "c" - potential confusion
   free(c.data1);        // Marks "c" as freed, but c.data2 still allocated
   ```

2. **Conditional Allocations**: The analyzer may report false positives for conditional paths:

   ```luma
   let ptr: *i64;
   if (condition) {
       ptr = alloc(sizeof<i64>);
   }
   // May warn even if you don't need to free in else branch
   ```

3. **Allocations in Loops**: Each loop iteration's allocations should be independent, but edge cases may exist:

   ```luma
   loop [i: i64 = 0](i < 10) : (++i) {
       let temp: *i64 = alloc(4);
       // Use temp...
       free(temp);  // Should work correctly
   }
   ```

4. **Early Returns with Defer**: While generally working, complex control flow with multiple early returns may need testing:

   ```luma
   const process -> fn () i64 {
       let a: *i64 = alloc(sizeof<i64>);
       defer free(a);

       if (error) { return -1; }  // Defer should fire
       if (warning) { return 0; } // Defer should fire
       return 1;                  // Defer should fire
   }
   ```

5. **Stack vs Heap**: The analyzer doesn't currently detect returning pointers to stack variables:

   ```luma
   const dangerous -> fn () *i64 {
       let local: i64 = 42;
       return &local;  // NOT DETECTED - returns dangling pointer
   }
   ```

6. **Arrays of Pointers**: Complex allocation patterns may not be fully tracked:

   ```luma
   let arr: [*i64; 5];
   loop [i: i64 = 0](i < 5) : (++i) {
       arr[i] = alloc(sizeof<i64>);  // Each needs individual free
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

**Generics are zero-cost** (see [Generics](#generics)):

```luma
const add -> fn<T> (a: T, b: T) T {
    return a + b;
}

// These calls compile to separate, optimized functions:
let x: i64 = add<i64>(1, 2);        // Same as: x = 1 + 2
let y: f32 = add<f32>(1.0, 2.0); // Same as: y = 1.0 + 2.0
```

**No runtime dispatch** - generic instantiations are resolved at compile time through monomorphization.

### Monomorphization

Luma generates specialized code per type, the same way C++ templates do:

```luma
const max -> fn<T> (a: T, b: T) T {
    if (a > b) { return a; }
    return b;
}

// Compiler generates, on first use of each:
// max__int(a: long long, b: long long) -> long long { ... }
// max__float(a: f32, b: f32) -> f32 { ... }
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
    x: i64,    // Offset 0, 8 bytes
    y: i64     // Offset 8, 8 bytes
};  // Total: 16 bytes
```

**Array layout is contiguous:**

```luma
let arr: [i64; 10];  // 80 contiguous bytes
// arr[0] at offset 0, arr[1] at offset 8, arr[2] at offset 16...
```

### Memory Allocation Performance

**Stack allocation is fast:**

```luma
const fast_function -> fn () void {
    let buffer: [i64; 1024];  // Stack allocated - instant
    // Use buffer...
}  // Automatically cleaned up
```

**Heap allocation has overhead:**

```luma
const slower_function -> fn () void {
    let buffer: *i64 = cast<*i64>(alloc(1024 * sizeof<i64>));
    defer free(buffer);
    // Use buffer...
}
```

### Optimization Guidelines

**1. Prefer stack allocation when possible:**

```luma
let temp: [i64; 100];  // Good for small, fixed-size data
```

**2. Minimize pointer indirection:**

```luma
// Better: direct access
let ptr: *i64;
let value: i64 = *ptr;   // One memory load

// Best: value directly
let value2: i64 = 42;     // No memory load
```

**3. Batch operations:**

```luma
// Good: one large allocation
let buffer: *i64 = cast<*i64>(alloc(1000 * sizeof<i64>));
loop [i: i64 = 0](i < 1000) : (++i) {
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
| ----------- | ------ | ------- |
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

```txt
const      let        if         elif       else
loop       break      continue   return     defer
struct     enum       pub        priv       cast
sizeof     alloc      free       switch     fn
using      static     input      system     as
```

### Directives

```luma
@module "name"              // Declare module name
@use "name" as alias        // Import module
@os { "linux" -> { } }      // Platform-conditional code
@link("lib.so")             // Link against shared library (module-level)
```

### Attributes

```luma
#returns_ownership          // Function returns allocated memory (caller must free)
#takes_ownership            // Function takes ownership of a pointer argument
#lib_import("lib.so")       // Per-function library override (POSIX)
#dll_import("dll", callconv: "stdcall")  // Per-function DLL import (Windows)
```

### Operators

```txt
Arithmetic:  +  -  *  /  %  ++  --
Comparison:  ==  !=  <  >  <=  >=
Logical:     &&  ||  !
Bitwise:     &  |  ^  ~  <<  >>
Assignment:  =
Access:      .   ::  []  *  &
```

### Primitive Types

```txt
i8      i16     i32     i64     u8      u16     u32     u64
f32     f64     bool    byte    void    *T      [T; N]
```

### Common Patterns

```luma
// Allocation with cleanup
let ptr: *T = cast<*T>(alloc(sizeof<T>));
defer free(ptr);

// Array iteration
loop [i: i64 = 0](i < size) : (++i) {
    array[i] = value;
}

// Error checking
if (ptr == cast<*T>(0)) {
    return ERROR_CODE;
}

// Module usage
@use "std_module" as m
let x: i64 = m::function();

// FFI — link a shared library (POSIX)
@link("libsomething.so")
pub const some_fn -> fn (x: i64) i64;

// FFI — per-function library override
#lib_import("libm.so")
pub const sqrt -> fn (x: f64) f64;

// FFI — Windows DLL
#dll_import("user32.dll", callconv: "stdcall")
pub const MessageBoxA -> fn (hwnd: i64, text: *byte, caption: *byte, utype: i64) i64;

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
