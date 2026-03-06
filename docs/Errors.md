# Known Bug: Struct Field Offset with Embedded Structs

When a struct contains another struct as its first field (e.g. `base: AstNode`), the compiler incorrectly calculates the memory offset of all fields that come after it.

The compiler treats the embedded struct as occupying 1 slot (8 bytes) when computing offsets, regardless of its actual size. So if `AstNode` is 32 bytes, the first field after `base` is placed at offset 8 instead of the correct offset 32 — off by 24 bytes.

## Workaround

Instead of using field access syntax (`node.name = x`), write directly to memory using integer array indexing:

```luma
let f: *int = cast<*int>(node);
f[0] = kind;       // base.kind
f[1] = category;   // base.category
f[2] = line;       // base.line
f[3] = col;        // base.col
f[4] = cast<int>(name);  // first concrete field
```

## Fix

The struct layout codegen needs to recursively compute the size of embedded struct fields (same logic as the `sizeof` fix) when calculating field offsets.