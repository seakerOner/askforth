# Defining Words

One of the most important features of Forth is the ability to define new words.

Instead of writing large programs as a sequence of low-level operations, Forth programs are built by creating small words and composing them together.

AskForth follows this model using its compiler and threaded execution system.

---

## Basic Definitions

A new word is defined using `:` and `;`.

For example:

```forth
: square core
    dup *
;
```

This defines a word named `square` inside the `core` dictionary.

This is different from a traditional Forth system where definitions are commonly added to one global dictionary.

A definition has three important parts:

```forth
: name dictionary
    ...
;
```

The first part starts the definition:

```forth
:
```

The second specifies the name and dictionary:

```forth
square core
```

The body contains the words that make up the definition:

```forth
dup *
```

Finally:

```forth
;
```

terminates the current definition.

---

## Compilation

When `:` is interpreted, AskForth enters compilation mode.

Words encountered inside the definition are normally **compiled** into the new word rather than immediately executed.

For example:

```forth
: double core
    2 *
;
```

The body:

```forth
2 *
```

becomes part of the compiled representation of `double`.

When `double` is later executed, its compiled execution data runs:

```forth
2 *
```

This is the basis of AskForth's threaded execution model.

---

## Literals

Numbers inside a definition are compiled as literals.

For example:

```forth
: five core
    5
;
```

When `five` executes, the value `5` is placed on the data stack.

Therefore:

```forth
five .
```

produces:

```text
5
```

The important distinction is that the `5` is not pushed onto the stack while the definition is being compiled. It becomes part of the compiled word and is pushed when the word executes.

---

## Calling Other Words

A definition can contain other words.

For example:

```forth
: square core
    dup *
;
```

The compiled word refers to the words needed to execute:

```text
dup
*
```

Definitions can therefore be composed from previously defined functionality.

For example:

```forth
: double core
    2 *
;

: quadruple core
    double double
;
```

`quadruple` is built entirely from the existing `double` word.

This composition is one of the fundamental ways Forth programs grow.

---

## The Semicolon

The `;` word terminates a definition.

When `;` is encountered, AskForth finishes the current definition and returns to interpretation mode.

After this point, newly entered words are interpreted normally again.

---

## Immediate Words

Not every word encountered during compilation should be compiled.

Some words need to execute **during compilation**.

These are called **immediate words**.

An immediate word therefore changes the normal behavior of compilation:

```text
Normal word:
    compiler → compile word

Immediate word:
    compiler → execute word now
```

This distinction is fundamental to understanding Forth metaprogramming.

---

## Why Immediate Words Exist

Immediate words are useful when a word needs to interact with the compiler itself.

For example, control-flow structures need to perform work while a definition is being compiled.

A compiler may need to:

* emit compilation data
* remember a location
* create a branch
* resolve a previous branch
* manipulate compiler state
* change how subsequent source is interpreted

These operations cannot always be represented simply by compiling the word itself.

Immediate words provide a mechanism for performing these actions during compilation.

---

## Immediate Words in AskForth

AskForth has explicit support for immediate words.

An immediate word is marked so that the compiler knows that it must be executed when encountered during compilation.

For example, when compiling:

```forth
: example core
    ...
;
```

the compiler normally treats words in the body as compilation targets.

If it encounters an immediate word, that word is executed by the compiler instead.

Conceptually:

```text
Interpretation:

    word
      ↓
    execute


Compilation:

    normal word
      ↓
    compile

    immediate word
      ↓
    execute during compilation
```

This distinction is important when reading AskForth's compiler-related words and the higher-level definitions found in `examples.fs`.
