# Getting Started

This section will get you from a freshly built AskForth executable to running your first Forth code.

The build process is documented separately in [`BUILD.md`](../BUILD.md). Once AskForth has been built, you can start experimenting with the Forth environment immediately.

---

## Starting AskForth

On Linux:

```bash
./build/askforth
```

On Windows:

```powershell
.\build\askforth.exe
```

AskForth starts an interactive Forth interpreter.

You should see the AskForth prompt:

```text
>
```

You can now enter Forth code directly.

---

## Exploring the Stack

The stack is central to Forth, so it is useful to inspect it while experimenting.

Try:

```forth
10 20 30
```

Then:

```forth
.s
```

The values currently on the stack will be displayed.

You can manipulate them using words such as:

```forth
dup
drop
swap
over
rot
```

For example:

```forth
10 20 over
```

leaves:

```text
10 20 10
```

You can continue experimenting without having to write a complete program first.

This interactive style is one of the strengths of Forth.

---

## Defining Your First Word

Forth programs are built by defining new words.

Try:

```forth
: square core
    dup *
;
```

This creates a new word called `square`.

You can now use it:

```forth
5 square .
```

Result:

```text
25 ok.
```

The definition:

```forth
: square core
    dup *
;
```

can be understood as:

1. Start a new definition with `:`.
2. Name the new word `square`.
3. Select the `core` dictionary.
4. Compile `dup` and `*`.
5. Finish the definition with `;`.

AskForth compiles the definition into threaded execution data.

---

## Building Words From Words

A useful way to think about Forth is that programs are built by composing existing words.

For example:

```forth
: double core
    2 *
;
```

Now:

```forth
10 double .
```

produces:

```text
20 ok.
```

You can then build another word using `double`:

```forth
: quadruple core
    double double
;
```

And:

```forth
5 quadruple .
```

produces:

```text
20 ok.
```

This style of composition is fundamental to Forth.

Instead of building increasingly complicated syntax, you build increasingly useful words.

---

## Loading Forth Source

AskForth can execute Forth source from files using:

```forth
INCLUDE filename
```

This is useful once programs become too large to enter interactively.

---

## Using the Examples

The repository contains an [examples.fs](../examples.fs) file.

The examples include words such as:

```forth
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

You can load the examples file using:

```forth
INCLUDE examples.fs
```

After loading it, the additional words defined by the file become available to the current Forth environment.

The exact path may depend on the directory from which AskForth was started.

---

## Persistent Blocks

AskForth also supports Forth blocks.

Blocks provide persistent storage that can contain Forth source or other data.

A block can be loaded with:

```forth
3 LOAD
```

The block is interpreted as Forth source.

Blocks are particularly useful for experimenting with Forth code in environments where source is organized into numbered blocks.

AskForth currently provides persistent block storage on its supported desktop platforms.

---

## What Happens When Something Goes Wrong?

Errors do not necessarily mean that AskForth must immediately terminate.

When a Forth-level execution error occurs, AskForth can enter its **Debugger / Recovery** environment.

The debugger can be used to inspect the state of the VM.

Commands include:

```text
help
status
trace
stack
rstack
cfstack
input
continue
abort
quit
```

This is particularly useful when experimenting interactively because you can inspect the state that existed when the error occurred.

At this point, you have enough to start experimenting with AskForth.
