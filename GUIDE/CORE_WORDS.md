# AskForth Core Words

This document is the reference for the words provided by the AskForth `core` dictionary.

Unlike the Guide, which introduces the language progressively, this document focuses on the exact behavior of individual core words.

Each word is described with its stack effect, purpose, and any AskForth-specific behavior or restrictions that are important when using it.

## .

```forth
( a -- )
```

Prints the value on top of the data stack.

For example:

```forth
42 .
```

produces:

42

The value is removed from the stack after being printed.
The value is printed according to the current stack signedness.

If the data stack is empty, `.` fails with a stack error.

## DEPTH

```forth
( -- depth )
```

Pushes the current data-stack depth onto the stack.

The depth is the number of cells currently stored on the data stack.

For example:

```forth
10 20 30 depth
```

leaves:

```forth
10 20 30 3
```

The returned value is itself stored using the current stack cell representation.

Because AskForth can change the cell width at runtime, the same stack contents can have different depths after a cell-width change.

For example, a stack containing two 64-bit cells may become four 32-bit cells when converted to 32-bit cells.


