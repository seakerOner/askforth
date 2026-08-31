# Control Flow

Forth does not use the same block and statement syntax found in languages such as C.

Control flow is built from words that manipulate execution directly.

AskForth provides conditional branches, loops, and control-flow words that can be combined to build higher-level structures.

## IF and ELSE

`IF` conditionally executes part of a definition based on a value on the data stack.

Its basic form is:

```forth
condition IF
    ...
THEN
```

For example:

```forth
: positive? core
    0 > IF
        1
    THEN
;
```

If the value is greater than zero, 1 is placed on the stack.

The condition is consumed by `IF`.

A conditional can also have an ELSE branch:

```forth
: abs core
    dup 0 < IF
        negate
    ELSE
        dup
    THEN
;
```

The code before `ELSE` is executed when the condition is true.

The code after `ELSE` is executed when the condition is false.

## Conditions

`IF` treats the value on top of the stack as a condition.

A zero value is false.

A non-zero value is true.

Conditions are usually produced by comparison words.

For example:

```forth
10 20 <
```

produces a boolean value which can then be used by `IF`:

```forth
10 20 < IF
    100
THEN
```

## BEGIN WHILE REPEAT

AskForth uses `BEGIN`, `WHILE`, and `REPEAT` to construct loops.

The basic form is:

```forth
BEGIN
    condition WHILE
        ...
REPEAT
```

The condition is tested before each iteration.

If it is false, execution leaves the loop.

If it is true, the body is executed and REPEAT returns to BEGIN.

For example:

```forth
: count-example core ( index - )
    BEGIN
        dup 10 <
    WHILE
        dup .
        1+
    REPEAT
    drop
;
```

This continues while the value is less than 10.

The same words can also be used to implement both common loop forms found in C.

A while loop tests the condition before executing the body:

```c
while (condition) {
    body
}
```

This maps directly to:

```forth
BEGIN
    condition WHILE
        body
REPEAT
```

A do while loop executes the body before testing the condition. The condition can be placed at the end of the loop:

```forth
BEGIN
    body
    condition WHILE
REPEAT
```

This executes body at least once before the condition is tested.

The position of WHILE therefore determines whether the condition is tested before or after the loop body.
