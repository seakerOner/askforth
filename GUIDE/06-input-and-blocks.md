# Input and Blocks

AskForth provides words for reading input, processing source, and working with persistent Forth blocks.

The input system is separate from the block system, although blocks can be used as a source of Forth code.

## Input

AskForth reads Forth source through its input system.

Interactive input is normally entered directly at the interpreter prompt:

```forth
10 20 + .
30 ok.
```

Forth source can also be read from other input sources, such as included files or blocks.

AskForth keeps track of the current input context so that source can be processed without losing the previous input state.

## INCLUDE

INCLUDE reads and executes Forth source from a file.

For example:

```
INCLUDE examples.fs
```

The contents of the file are processed as if they had been entered as Forth source.

This is useful for loading definitions without entering them manually at the interpreter.

## a

`a` appends input to a memory address. This idea is inspired by [ed][1]

Its purpose is to make it possible to write strings directly into memory/blocks

## Blocks

AskForth supports Forth blocks for persistent source and data storage.

A block is identified by a number and contains a fixed-size region of data.

Blocks can contain Forth source, but they can also be used to store other data.

Common block words include:

```forth
BLOCK
LIST
LINE
LOAD
FLUSH
```

## BLOCK

`BLOCK` obtains access to a block using its block number.

For example:

```forth
3 BLOCK
```

returns the address of block 3.

The returned address can then be used by other memory or block words.

## LIST

LIST displays the contents of a block.

For example:

```forth
3 LIST
```

displays the contents of block 3 in a human-readable form.

This is useful when working with blocks as Forth source.

## LINE

`LINE` receives the address of a block and a line number then returns the address offseted to that line.

This allows individual lines of a block to be accessed without having to manually calculate their position. 

For example, conceptually:

```forth
1 BLOCK 3 LINE
```

produces the address associated with the requested line.

LINE is primarily useful when manipulating block contents directly. ( with `a` for example )

## LOAD

`LOAD` loads a block as Forth source.

For example:

```forth
3 LOAD
```

reads block 3 and interprets its contents as Forth code.

This makes blocks another way of storing and loading Forth programs.

## FLUSH

FLUSH writes modified block contents back to persistent storage.

After modifying a block, FLUSH can be used to save those changes. 

The exact backing storage depends on the platform.

On desktop platforms, AskForth uses persistent file storage for blocks.

[1]: https://en.wikipedia.org/wiki/Ed_(text_editor)
