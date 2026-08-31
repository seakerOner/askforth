# Memory

AskForth provides a Forth-managed memory area that can be accessed directly from Forth code.

Memory operations are based on addresses. An address is simply a value that identifies a location inside the VM's memory.

This allows Forth code to store, read, modify, and move data without requiring every operation to be implemented as a high-level language feature.

## Addresses

An address is a value that refers to a location in memory.

For example, `HERE` returns the current allocation address:

```forth
HERE
```

The returned value can then be used by memory words to access that location.

Forth does not require a special pointer syntax. An address is simply a value on the data stack.

This is one of the reasons the Forth stack and memory model work together so closely.

## Reading a Cell

The `@` word reads one native architecture cell from memory.

Its stack effect is:

```forth
( address -- value )
```

For example:

```forth
HERE @
```

takes the address returned by `HERE` and reads the cell stored at that location.

The number of bytes read by `@` depends on the host architecture's word size, not on the current AskForth stack cell width.

The current stack cell width must match the architecture's word size when using `@`. If it does not, `@` fails instead of attempting to use an incomplete or truncated address.

For example, on a 64-bit architecture:

```text
64 BITS → @ works
32 BITS → @ fails
16 BITS → @ fails
```

## Writing a Cell

The `!` word stores one native architecture cell into memory.

Its stack effect is:

```forth
( value address -- )
```

For example:

```forth
42 HERE !
```

stores 42 at the address returned by `HERE`.

The value is consumed from the stack and written into memory.

The corresponding read operation is:

```forth
HERE @
```

which retrieves the stored value.

The number of bytes written by `!` depends on the host architecture's word size, not on the current AskForth stack cell width.

As with `@`, the current stack cell width must match the architecture's word size. Otherwise, `!` fails before attempting to use the address.

For example, on a 64-bit architecture:

```text
64 BITS → ! works
32 BITS → ! fails
16 BITS → ! fails
```

## Character Access

AskForth also provides byte-sized memory operations.

`c@` reads one byte from memory:

```forth
( address -- char )
```

and `c!` stores one byte:

```forth
( char address -- )
```

For example:

```forth
65 HERE c!
```

stores the byte value 65 at the address returned by `HERE`.

It can then be read with:

```forth
HERE c@
```

The important difference between @ / ! and C@ / C! is the amount of memory accessed.

```text
@   → architecture word size
!   → architecture word size

C@  → one byte
C!  → one byte
```

`c@` and `c!` still operate on native addresses, so the stack cell width must match the architecture's address width when supplying an address.

## HERE

`HERE` returns the current address used by the Forth memory allocator.

The returned address can be used as the starting point for allocating data.

For example:

```forth
HERE
```

might return an address conceptually represented as:

```text
0x00001234
```

`HERE` itself does not allocate memory. It reports the current allocation position.

Because `HERE` returns a native address, the current stack cell width must match the host architecture's address width. Otherwise, `HERE` fails.

## ALLOT

`ALLOT` advances the current allocation position by a specified number of bytes.

Its stack effect is:

```forth
( bytes -- )
```

For example:

```forth
64 ALLOT
```

reserves 64 bytes of the Forth-managed memory area.

A common pattern is:

```forth
HERE
64 ALLOT
```

The memory contents are not necessarily initialized by `ALLOT`.

## cells

`cells` converts a number of cells into the corresponding number of bytes.

The size of a cell is determined by the current AskForth stack cell width.

For example:

```forth
4 cells
```

converts four cells into the corresponding byte count.

This is useful when allocating memory intended to hold cell-sized values:

```forth
10 cells ALLOT
```

For example, with 64-bit cells, 10 cells corresponds to 80 bytes. With 32-bit cells, it corresponds to 40 bytes.

## cell+

`cell+` advances an address by one cell.

Its stack effect is:

```forth
( address -- address+cell )
```

## chars

`chars` converts a number of character units into bytes.

Since AskForth characters are byte-sized, this currently corresponds to a byte count.

For example:

```forth
10 chars
```

produces:

```forth
10
```

It is useful for code that explicitly works in character-sized units rather than cell-sized units.

## char+

`char+` advances an address by one character.

Its stack effect is:

```forth
( address -- address+1 )
```

Since AskForth characters are byte-sized, this advances the address by one byte.

## FILL

`FILL` writes a value into a range of memory.

Its stack effect is:

```forth
( address length value -- )
```

For example:

```forth 
HERE 64 0 FILL
```

fills 64 bytes starting at `HERE` with zero.

## MOVE

MOVE copies a region of memory from one location to another.

Its stack effect is:

```forth
( source destination length -- )
```

## Memory Is Explicit

AskForth does not hide memory access behind a high-level object model.

Forth code can explicitly work with:
    - addresses
    - bytes
    - cells
    - allocated regions

This is intentional.

The same primitives used to manipulate memory can be used to construct higher-level abstractions.

For example, the concepts behind variables, buffers, fields, and created data structures can be implemented using these memory primitives.

## Higher-Level Memory Facilities

Some familiar Forth facilities are not part of the AskForth core dictionary.

For example:
    - VARIABLE
    - BUFFER
    - CREATE

are demonstrated as higher-level functionality in [examples.fs](../examples.fs).

These words build upon the lower-level memory and compilation mechanisms provided by the core.
