# Introduction

Welcome to the AskForth Guide.

AskForth is an experimental Forth implementation written in C. It is built around a small and explicit virtual machine, with the goal of keeping the execution model simple while allowing more advanced language features to be built on top of it.

This guide introduces the Forth language as implemented by AskForth and explains the features that make AskForth different from a more traditional Forth system.

If you have never used Forth before, this guide should give you enough context to start writing and experimenting with Forth code.

---

## What is Forth?

Forth is a stack-based programming language.

Instead of writing expressions in the usual mathematical form:

```text
10 + 20
```

Forth places values on a data stack and executes words that consume and produce those values:

```forth
10 20 +
```

After executing `10 20`, the stack contains:

```text
10 20
```

The `+` word removes the two values, adds them, and places the result back on the stack:

```text
30
```

A word that prints the top stack value can then be used:

```forth
10 20 + .
```

Result:

```text
30
```

This stack-based model is one of the most important concepts to understand when learning Forth.

---

## Words

Forth programs are primarily composed of **words**. 

A word is simply a sequence of characters separated by whitespace. This is one of the fundamental 
ideas behind Forth's sintax: the language does not require complex syntax to represent most operations.

A word is a named operation that can manipulate the stack, access memory, perform I/O, change compilation behavior, or execute other words.

For example:

```forth
dup
drop
swap
+
-
*
.
```

Some words consume values from the stack and produce new values.

For example:

```forth
10 dup
```

produces:

```text
10 10
```

while:

```forth
10 20 swap
```

produces:

```text
20 10
```

Words can also be defined using other words.

```forth
: square core
    dup *
;
```

After defining `square`, it can be used like any other word:

```forth
5 square .
```

Result:

```text
25
```

This ability to build new words from existing ones is central to Forth.

---

## The Data Stack

The data stack is the main mechanism used to pass values between words.

For example:

```forth
10 20
```

leaves two values on the stack:

```text
10
20 
```

The most recently pushed value is on top.

Executing:

```forth
+
```

consumes both values and produces one:

```text
30
```

You can inspect the stack with:

```forth
.s
```

For example:

```text
10 20 30 .s
```

---

## Interpretation

When AskForth is running interactively, input is read and interpreted as a sequence of words and values.

For example:

```text
> 10 20 + .
30 ok.
```

AskForth reads:

```text
10
20
+
.
```

Numbers are placed on the stack and words are executed.

This makes the interactive interpreter useful not only for running programs, but also for exploring the language.

You can experiment directly:

```text
> 10 20 +
> .s
```

and inspect the resulting stack.

---

## Compilation

Forth can also compile new words.

A definition starts with `:` and ends with `;`:

```forth
: square core
    dup *
;
```

During the definition, AskForth enters compile mode.

The words inside the definition are compiled into the new word rather than immediately executed.

The resulting word can then be executed:

```forth
5 square .
```

producing:

```text
25
```

AskForth uses threaded execution for these compiled words.

The important idea for now is simply that Forth code can be used to create new Forth words, allowing the language to extend itself.

---

## Forth is Small by Design

A characteristic of Forth is that a relatively small set of primitive operations can be used to construct much larger parts of the language.

AskForth follows this idea closely.

Not every feature needs to be implemented directly inside the virtual machine.

Instead, the VM provides general mechanisms for:

* Stack manipulation
* Memory access
* Compilation
* Execution
* Control flow
* Literals
* Dictionary management

Higher-level functionality can then be implemented using those mechanisms.

For example, features such as:

```text
CONSTANT
VARIABLE
CREATE
DO
I
LOOP
```

can be built using existing Forth facilities rather than requiring each one to become a special VM primitive.

This approach is demonstrated in [examples.fs](../examples.fs).

---

## What Makes AskForth Different?

AskForth is not intended to be a strict recreation of a particular traditional Forth implementation.

It uses the Forth model as a foundation while experimenting with a few ideas in the VM and runtime.

### Dynamic Cell Sizes

The data stack can use different cell sizes:

```text
8-bit
16-bit
32-bit
64-bit
```

The selected cell size is a property of the running VM and can be changed during execution.

For example:

```forth
64 BITS
```

changes the current stack cell size.

AskForth also supports signed and unsigned operation modes:

```forth
SIGNED
UNSIGNED
```

This means the representation of stack values is not simply fixed when the program is compiled.

---

### Multiple Dictionaries

AskForth organizes dictionaries inside a library.

Instead of having only one global collection of words, a library can contain multiple dictionaries:

```text
Library
├── core
├── tools
├── user
└── ...
```

A word therefore belongs to a dictionary, and dictionaries can be created and populated from Forth code.

This provides a foundation for separating groups of words while keeping them inside the same Forth environment.

---

### Debugger / Recovery

AskForth also has an interactive Debugger / Recovery environment.

When a Forth-level execution error occurs, the VM can preserve enough execution state to enter the debugger instead of immediately terminating.

The debugger can inspect information such as:

* Data stack
* Return stack
* Control-flow stack
* Interpreter state
* Input state
* Error history
* Threaded execution state

The execution can then be aborted, or in some cases resumed using the preserved context.

This is particularly useful while experimenting interactively with Forth.

---

## A Note About the Core

AskForth's `core` dictionary contains the primitive functionality provided by the implementation.

It is important to distinguish between **core words** and higher-level words implemented in Forth.

For example, some familiar Forth functionality is demonstrated in [examples.fs](../examples.fs) rather than being part of the `core` dictionary itself.

In particular, words such as:

```forth
CONSTANT
VARIABLE
BUFFER
CREATE
,
DO
I
LOOP
```

are **not part of the AskForth core dictionary**.

They demonstrate how higher-level functionality can be constructed from the existing primitives.
