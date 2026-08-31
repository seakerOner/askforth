# The Stack

The data stack is the central mechanism used by Forth to pass values between words.

AskForth uses the data stack for arithmetic, memory operations, comparisons, function-like arguments, and many other operations.

Understanding how values move through the stack is essential to writing Forth code.

---

## Pushing Values

Numbers entered into the interpreter are pushed onto the data stack.

For example:

```forth
10
```

leaves:

```text
10
```

on the stack.

Multiple values can be pushed:

```forth
10 20 30
```

The stack can be visualized as:

```text
30  <- top
20
10
```

The most recently pushed value is always the top of the stack.

---

## Consuming Values

Words can remove values from the stack and produce new ones.

For example:

```forth
10 20 +
```

starts with:

```text
20  <- top
10
```

The `+` word consumes the two values and pushes their sum:

```text
30
```

The general pattern is:

```text
a b +  ->  a+b
```

This way of describing stack behavior is commonly used when discussing Forth words.

---

## Stack Effects

A word's **stack effect** describes which values it consumes and produces.

For example, addition can be described as:

```text
( a b -- a+b )
```

The values before `--` are consumed by the word.

The values after `--` are produced by the word.

> Note:
> The forth word `(` is a comment that ends at `)`. The contents of the comment are ignored by the interpreter

For example:

```forth
( a b -- c )
```

means that the word consumes two values and produces one.

Stack effects are one of the most useful ways to understand Forth code.

Consider:

```forth
10 20 + 5 *
```

The operations can be described as:

```text
10 20 +   -> 30
30 5 *    -> 150
```

The complete expression therefore leaves:

```text
150
```

on the stack.

---

## Inspecting the Stack

AskForth provides `.s` to inspect the current data stack.

For example:

```forth
10 20 30
.s
```

The values remain on the stack after `.s`.

This makes `.s` particularly useful while experimenting interactively.

You can also inspect the stack in the Debugger / Recovery environment using the `stack` command.

---

## Duplicating Values

`dup` duplicates the value on top of the stack.

```forth
10 dup
```

produces:

```text
10 10
```

Its stack effect is:

```forth
( a -- a a )
```

For example:

```forth
5 dup *
```

produces:

```text
25
```

---

## Dropping Values

`drop` removes the value on top of the stack.

```forth
10 20 drop
```

leaves:

```text
10
```

Its stack effect is:

```text
( a -- )
```

---

## Swapping Values

`swap` exchanges the top two values.

```forth
10 20 swap
```

leaves:

```text
20 10
```

Its stack effect is:

```text
( a b -- b a )
```

This is often useful for arranging arguments in the order required by another word.

---

## Copying the Second Value

`over` copies the second value from the top and places the copy on top.

```forth
10 20 over
```

produces:

```text
10 20 10
```

Its stack effect is:

```text
( a b -- a b a )
```

Unlike `dup`, `over` does not duplicate the top value. It duplicates the value immediately below it.

---

## Rotating Values

`rot` rotates the top three values.

```forth
10 20 30 rot
```

produces:

```text
20 30 10
```

Its stack effect is:

```text
( a b c -- b c a )
```

`rot` is useful when several values need to be rearranged before being consumed by another word.

---

## Stack Depth

The number of values currently stored on the data stack is its **depth**.

For example:

```forth
10 20 30 
```

has a stack depth of:

```text
3
```

Stack depth is useful when reasoning about how many values are currently available to a word.

---

## Stack Underflow

A word cannot consume values that are not present.

For example:

```forth
10 20 +
```

is valid because `+` receives two values.

But:

```forth
10 +
```

does not provide enough values for `+`.

This is a **stack underflow**.

When AskForth detects an execution error such as this, it can enter the Debugger / Recovery environment rather than immediately terminating execution.

---

## The AskForth Stack Cell

Unlike many traditional Forth implementations, AskForth does not use a single fixed cell size for its data stack.

The stack supports:

```text
8-bit
16-bit
32-bit
64-bit
```

It can be changed during execution with:

```forth
8 BITS
16 BITS
32 BITS
64 BITS
```

For example:

```forth
64 BITS
```

selects 64-bit cells.

This is an AskForth-specific feature and is not simply a compile-time configuration.

---

## Changing Cell Size

The cell size can be changed while values already exist on the stack.

Changing the cell size does not convert existing values into values of the new size. Instead, the existing stack storage is reinterpreted using the new cell width.

For example, a stack containing two 64-bit cells has a depth of 2:

```text
64-bit cells

[ 64-bit cell ]
[ 64-bit cell ]
```

Changing the cell size to 32 bits results in four cells using the same underlying stack data:

```text
32-bit cells

[ 32-bit cell ]
[ 32-bit cell ]
[ 32-bit cell ]
[ 32-bit cell ]
```

The stack depth therefore depends on the current cell width.

---

## Signed and Unsigned Values

AskForth also allows the stack to operate in signed or unsigned mode.

Use:

```forth
SIGNED
```

for signed operation, or:

```forth
UNSIGNED
```

for unsigned operation.

---

## Reading Stack-Based Code

When reading Forth code, it is often useful to follow the stack after every word.

For example:

```forth
10 2 + 3 *
```

can be followed as:

```text
10 2
10 2 +
12
12 3
12 3 *
36
```

The final result is:

```text
36
```

This technique is particularly useful when learning Forth.

When a piece of code is confusing, stop and write down the stack after each word. 

As a counterpoint, if the stack gets difficult to reason about, that can be as sign that the code should be refactored. Consider redesigning the words so that their stack effects are simpler and easier to follow
