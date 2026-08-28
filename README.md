# AskForth
> An experimental Forth implementation in C, focused on a small VM, runtime introspection, flexible stacks, extensible dictionaries, and recoverable execution.

> AskForth's guide: **TODO**

> How to build AskForth: [GUIDE](./BUILD.md)

AskForth is a Forth implementation written in C and built around a small, explicit virtual machine.

It is the third Forth implementation developed by the author ( me :D ).

The goal is not to reproduce a particular traditional Forth implementation, but to explore how far a small Forth system can go when its VM exposes a few powerful primitives and higher-level language features are built on top of them.

AskForth is currently under active development.

## Platform Status

| Platform | Status |
|----------|--------|
| Linux    | Supported |
| Windows  | Supported |
| ESP32    | Planned |
| skdojo   | Planned |

### Windows Builds

The Windows executable is currently **unsigned**.

Depending on the Windows security configuration, Windows Smart App Control or SmartScreen may prevent the executable from running or display a security warning.

This is expected for development builds.

The project is open-source, and code signing for distributed Windows releases may be added in the future.

---

## Why AskForth?

AskForth explores a few ideas that are unusual, or at least not typical, in a Forth implementation.

### Dynamic Cell Sizes

The AskForth data stack supports 8, 16, 32 and 64-bit cells.

More importantly, the cell size can be changed **during execution**.

```forth
64 BITS
```
The existing stack contents are preserved when the cell size changes, and the stack depth is adjusted to match the new cell width.

The stack also supports signed and unsigned modes:

```forth
SIGNED
UNSIGNED
```

This makes the stack representation a runtime property of the VM rather than a fixed property of the build.

---

### Library → Dictionaries

Traditional Forth systems commonly revolve around a global dictionary.

AskForth instead organizes dictionaries inside a **library**.

```text
Library
├── core
├── tools
├── user
└── ...
```

A library can contain multiple dictionaries, allowing words to be grouped into separate namespaces.

Dictionaries are part of the Forth environment and can be created and populated from Forth code.

Words can be implemented as either:

* Native C words
* Threaded Forth words

---

### Debugger / Recovery

When an execution error occurs, AskForth can enter an interactive **Debugger / Recovery** environment.

Instead of immediately terminating the VM, execution state can be inspected.

```text
[ DEBUGGER / RECOVERY ]

@> help
@> status
@> trace
@> stack
@> rstack
@> cfstack
@> input
@> continue
@> abort
@> quit
```

The debugger can inspect:

* VM state
* Interpreter state
* Data stack
* Return stack
* Control-flow stack
* Input and parser state
* Error history
* Threaded execution trace

The most interesting part is that execution context can be preserved after a Forth-level failure.

For example, `continue` can skip the failed word and resume execution from the preserved execution context.

```text
error
  ↓
Debugger / Recovery
  ↓
inspect state
  ↓
continue
  ↓
resume execution
```

This is intended for interactive development and experimentation rather than replacing proper error handling.

---

## A Small Core, Powerful Composition

One of the design goals of AskForth is to avoid making every language feature a VM primitive.

Instead, the VM provides relatively general compilation and execution mechanisms, and higher-level features can be built using Forth itself.

For example, `CONSTANT`, `VARIABLE`, `BUFFER`, `CREATE`, `,`, `DO`, `I` and `LOOP` can be implemented using existing compilation, memory and control-flow words.

A simplified example:

```forth
: CONSTANT core
    [:] LIT [;]
;
```

A constant is therefore just a word containing a compiled literal.

Likewise, higher-level control structures can be constructed from existing control-flow primitives.

This is an important part of the AskForth design:

> **Keep the VM primitives general, and build language facilities on top of them.**

The `examples.fs` file contains examples of this approach.

---

## Feature Overview

### Execution

AskForth supports:

* Interactive interpretation
* Compilation of threaded words
* Native C words
* Threaded Forth words
* Immediate words
* Runtime execution frames
* Explicit interpreter and compiler states

A normal definition looks like:

```forth
: square core
    dup *
;
```

The resulting word is represented as threaded execution data managed by the VM.

---

### Native and Threaded Words

AskForth combines two kinds of executable words.

**Native words** directly call C functions.

They are useful for:

* VM operations
* I/O
* memory primitives
* platform functionality
* low-level operations

**Threaded words** contain execution entries managed by the Forth VM.

Threaded code can combine:

* literals
* other threaded words
* native C functions
* control-flow instructions
* termination markers

This provides a bridge between the low-level VM and the Forth language.

---

## Memory

AskForth provides a Forth-managed memory area with words such as:

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

For example:

```forth
HERE
```

returns the current allocation address.

```forth
64 ALLOT
```

advances the allocation position by 64 bytes.

---

## Persistent Blocks

AskForth provides Forth blocks for persistent source and data storage.

Typical words include:

```forth
BLOCK
LIST
LINE
LOAD
FLUSH
```

For example:

```forth
3 LOAD
```

loads block 3 as Forth source.

On Linux and Windows, blocks are backed by persistent file storage.

---

## Input and Parsing

AskForth separates input, tokenization and execution.

The VM supports multiple input contexts, allowing Forth code to process additional sources without simply replacing the main interpreter input.

This is used by functionality such as:

```forth
LOAD
INCLUDE
```

The tokenizer keeps source context such as the current token and its position, which also allows the debugger to report where execution failed.

---

## Error Tracing

AskForth maintains a fixed-size error trace.

Errors can contain information such as:

* Error type
* Error zone
* Optional message
* Token context

The VM also maintains execution frames for threaded words.

Together, these provide enough information to produce an execution trace such as:

```text
foo -> bar -> baz
              ^
           failed
```

This information is available from the Debugger / Recovery environment.

---

## Example

A simple interactive session:

```text
> 10 20 + .
30 ok.
```

Define a word:

```forth
: square core
    dup *
;
```

Then execute it:

```text
> 5 square .
25 ok.
```

Change the stack representation:

```forth
> 64 BITS UNSIGNED
```

and inspect it:

```text
> .s
```

The current cell width and signedness are properties of the running VM.

---

## Examples

The repository contains an `examples.fs` file demonstrating how higher-level Forth functionality can be built from the existing core.

Examples include:

```forth
LIT
[:]
[;]
[']
EXEC,
ALIAS:
CONSTANT
VARIABLE
BUFFER
FIELD
CREATE
,
DO
I
LOOP
```

The purpose of these examples is not to define an official standard library.

They demonstrate how the core can be extended from within Forth itself.

---

## Current Status

AskForth is **experimental but functional**.

The main execution model is implemented and the system is already usable as an interactive Forth environment.

Current areas of development include:

* More platform backends
* More robust fault handling
* More Forth words
* Improved documentation
* Completing the VM reset functionality
* Further refinement of the Debugger / Recovery system
