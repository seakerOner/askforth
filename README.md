# AskForth

> AskForth's guide: **TODO**

AskForth is a Forth implementation written in C.

It is the third Forth implementation developed by the author.

The main goals of AskForth are:

* Keep the Forth virtual machine simple.
* Make the system easy to port to different platforms.
* Provide a library system for Forth dictionaries.
* Support native and threaded Forth words.
* Provide an error tracing system.

AskForth is under active development.

Some parts of the system are not complete.

## Main Features

### Forth virtual machine

AskForth uses a virtual machine structure.

The VM stores the main execution state and the system components.

The VM contains:

* RAM.
* Data stack.
* Input buffers.
* Tokenizers.
* Block storage.
* Dictionary library.
* Error tracer.
* Parser state.
* Interpreter state.

The VM can run in two main modes:

* Interpret mode.
* Compile mode.

The main execution loop waits for input, parses the input, and executes the resulting words.

## Configurable Stack

AskForth uses a configurable data stack.

The stack can use these cell sizes:

* 8 bits.
* 16 bits.
* 32 bits.
* 64 bits.

The stack also supports two value modes:

* Signed.
* Unsigned.

The stack can change its cell size during execution.

For example:

```forth
64 BITS
```

sets the stack cell size to 64 bits.

The stack size is also adjusted for the selected cell size.

This design is different from a fixed-cell Forth stack.

The same stack implementation can store values with different cell sizes.

## Dictionary Library

AskForth has a library system for dictionaries.

A library can contain multiple dictionaries.

Each dictionary contains Forth words.

The system starts with a `core` dictionary.

New dictionaries can be created from Forth code.

Example:

```forth
ADD-DIC tools
```

The dictionary can then contain new words.

Words can be added as:

* Native words.
* Threaded words.

The library keeps the dictionary and word structures inside the AskForth memory area.

This allows the VM to manage Forth structures without depending on the host heap for each object.

## Native Words

A native word calls a C function.

For example, a word can point to a C function such as:

```c
static void askf_word_dup(void);
```

The VM calls the function when it executes the word.

Native words are useful for:

* VM operations.
* Input and output.
* Memory operations.
* Platform functions.
* Low-level operations.

## Threaded Words

AskForth also supports threaded words.

A threaded word contains execution entries in the AskForth memory area.

The threaded code can contain:

* Immediate values.
* Other threaded words.
* Native C functions.
* An end marker.

A threaded word can therefore combine Forth-level code with native operations.

A word definition uses:

```forth
: word-name dictionary-name
    ...
;
```

The VM changes to compile mode during the definition.

Normal words are compiled into the threaded word.

Immediate words execute during compilation.

## Immediate Words

A word can be marked as immediate.

Example:

```forth
IMMEDIATE
```

An immediate word executes while the VM is in compile mode.

This allows Forth words to change the compiler behavior.

This mechanism is also used by parser and control words.

## Memory System

AskForth uses a simple linear memory allocator.

The VM has a RAM area called a memory blob.

Allocations use an increasing byte index.

The main allocation operation is:

```c
askf_alloc()
```

The allocator returns memory from the AskForth RAM area.

The system does not use a general-purpose free operation for these allocations. ( TODO )

## Platform Separation

Platform-specific code is separated with compile-time conditions.

For example:

```c
#ifdef TARGET_LINUX
```

Linux-specific code currently uses operating system services such as:

* `mmap()`
* `msync()`
* `ftruncate()`
* `getcwd()`

The design keeps these operations outside the main Forth execution model.

This is important for portability.

A future platform can provide its own implementation for the required platform services.

The goal is to keep the Forth VM independent from the operating system.

## Forth Blocks

AskForth provides persistent blocks.

The current Linux implementation stores blocks in a file named:

```text
blocks.fb
```

The block file is mapped into memory.

The default configuration uses:

* 1024 blocks on Linux.
* 64 blocks on other targets.
* 1024 bytes per block.

The Forth system provides words for block access.

Examples include:

```forth
BLOCK
LIST
LINE
LOAD
FLUSH
```

### `BLOCK`

`BLOCK` returns the address of a block.

Example:

```forth
3 BLOCK
```

The result is the address of block 3.

### `LIST`

`LIST` displays a block.

Example:

```forth
3 LIST
```

### `FLUSH`

`FLUSH` updates the block storage.

Example:

```forth
FLUSH
```

## Input System

AskForth uses input buffers and tokenizers.

The VM has two input paths:

* Main input.
* Secondary input.

The secondary input is useful for operations such as loading source blocks and including files.

The tokenizer converts the input buffer into tokens.

Each token contains information such as:

* Address.
* Length.
* Line end state.

The VM then searches the dictionary library for each token.

If the token is not a word, the VM tries to parse it as a number.

If the token is not a valid word or number, AskForth reports an unknown-word error.

## Parser Support

AskForth has two parser types:

```text
ASKF_MAIN_PARSER
ASKF_X_PARSER
```

The main parser is used for normal interactive input.

The secondary parser is used by operations that need to execute another input source.

This design allows Forth words to execute additional source without replacing the main input context.

## Strings and Parsing

AskForth provides words for parsing and string handling.

Examples include:

```forth
PARSE-WORD
."
TYPE
EMIT
CR
```

A string can be printed with:

```forth
." Hello AskForth"
```

AskForth also supports comments.

The system provides:

```forth
(
\
```

The parenthesis comment ends at `)`.

The line comment ends at the end of the input line.

## Memory Words

AskForth provides low-level memory operations.

Examples include:

```forth
@
!
C@
C!
HERE
ALLOT
CELLS
CELL+
CHARS
CHAR+
FILL
MOVE
```

These words allow Forth code to work directly with memory.

For example:

```forth
HERE
```

returns the current address in the AskForth RAM area.

`ALLOT` moves the current memory position.

Example:

```forth
64 ALLOT
```

This reserves 64 bytes.

## Stack Words

AskForth provides common stack manipulation words.

Examples include:

```forth
DUP
2DUP
DROP
2DROP
SWAP
2SWAP
ROT
NIP
TUCK
OVER
2OVER
DEPTH
```

The implementation performs explicit stack checks for many operations.

For example:

```forth
DUP
```

requires one value on the stack.

```forth
2DUP
```

requires two values.

If the required values are not available, AskForth reports a word failure.

## Arithmetic and Comparison

AskForth provides basic arithmetic operations:

```forth
+
-
*
/MOD
NEGATE
```

It also provides comparison words:

```forth
=
<>
<
<=
>
>=
0=
0<>
0<
0>
```

The stack signedness affects comparison operations.

The stack can be changed to signed mode with:

```forth
SIGNED
```

and to unsigned mode with:

```forth
UNSIGNED
```

## Logic and Bit Operations

AskForth provides bit operations:

```forth
AND
OR
XOR
INVERT
LSHIFT
RSHIFT
```

These operations work on the current stack cell representation.

## Boolean Values

AskForth provides:

```forth
TRUE
FALSE
```

`TRUE` pushes `-1`.

`FALSE` pushes `0`.

## Stack Information

The stack can be inspected with:

```forth
DEPTH
.S
```

`.STACK` displays information about the current stack.

The output includes:

* Cell size.
* Signedness.
* Stack depth.
* Stack values.

## Defining Words

A new threaded word can be created with:

```forth
: hello core
    ." Hello"
;
```

The first name is the new word.

The second name selects the dictionary.

The VM enters compile mode after `:`.

The word returns to interpret mode after `;`.

The `IMMEDIATE` word marks the current word as immediate.

## Loading Source

AskForth supports loading source from blocks.

Example:

```forth
3 LOAD
```

The system reads block 3 into the secondary input buffer and executes it.

On Linux, AskForth also provides file inclusion.

Example:

```forth
INCLUDE myfile.fs
```

The file is read and executed by the secondary parser.

## Development Status

AskForth is an experimental Forth implementation.

The core execution model is present, but some parts still require development.

Known areas include:

* More complete shutdown handling.
* More complete error handling.
* More platform backends.
* More portable input and file handling.
* More complete bounds checking.
* More complete memory error handling.
* More tests.
* More Forth words.
* A complete build system.
* Better documentation for the Forth language layer.

Some functions currently contain `TODO` sections.

These sections identify work that is still required.

## Portability

Portability is a main design goal of AskForth.

The VM should not require a specific operating system.

The system separates platform-specific operations from the Forth core where possible.

The current implementation contains Linux-specific code for:

* Virtual memory.
* Block file mapping.
* File input.
* File synchronization.
* Current working directory handling.

Other targets can provide their own backend.

## Error Handling

AskForth has an error structure and an error tracer.

The VM keeps a history of recent errors.

The error tracer uses a fixed capacity.

This allows the VM to keep error information without requiring a dynamic error list.

Errors can contain:

* Error zone.
* Error type.
* Optional message.
* Token context.

The tokenizer also stores the current token and token index.

This information can help identify where an error occurred.

## Example Session

A simple session can look like:

```text
> 10 20 + .
30 ok.
```

Stack operations can be used directly:

```text
> 10 20 2DUP . . . .
10 20 10 20 ok.
```

A new word can be compiled:

```forth
: square core
    DUP *
;
```

Then:

```forth
> 5 square .
25 ok.
```

## Current Platform Notes

The Linux target is the main platform in the current implementation.

The non-Linux configuration already defines different memory and block limits.

However, a complete non-Linux backend is still required for full portability.

Therefore, the project should not be considered fully portable yet.

The architecture is designed to make this work easier.

