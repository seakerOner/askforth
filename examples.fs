\ ============================================================
\ Agnostic Seaker's Forth - Examples
\ ============================================================
\
\ These words are not required by the core.
\ They are examples of how the language can be extended
\ using the existing compilation and memory primitives
\
\ Read these as examples of what can be built on top of
\ the core, rather than as a standard library
\
\ ============================================================

\ LIT is a small helper that compiles a literal into
\ the word currently being compiled.

: LIT core
    POSTPONE LITERAL
;

\ [:] postpones the execution of : 
\
\ This allows a word to start compiling another word
\ while it is itself being executed

: [:] core 
    POSTPONE :
;

\ [;] postpones the execution of ;
\ 
\ Together with [:], this allows words to create new
\ word definitions programmatically

: [;] core
    POSTPONE ;
;

\ ['] compiles the execution token of the following word
\ as a literal.

: ['] core  ( "name" - )
    POSTPONE '
    LIT
; IMMEDIATE

\ EXEC, compiles the word EXECUTE into the current definition
\ It a small convenience word built from ['] and COMPILE,

: EXEC, core
   ['] EXECUTE COMPILE,
;

\ ALIAS: creates a new word which executes another word,
\
\ The source word is read from the input, its execution
\ token is obtained, and the new word is compiled to 
\ execute that token
\
\ Example:
\
\   ALIAS: .s print core
\   print

: ALIAS: core ( "source" "name" "dictionary" )
    POSTPONE ' [:] LIT EXEC, [;]
;

\ CONSTANT creates a word which returns a fixed value.
\
\ Notice that CONSTANT does not need a dedicated primitive:
\ it is simply a new word containing a compiled literal.

: CONSTANT core ( x "name" "dictionary" )
    [:] LIT [;]
;

\ A constant can also be created directly using the core 
\ compilation words:
\
\   10 : coolname core LITERAL ;
\
\ Or, equivalently:
\
\   : coolname core [ 10 ] LITERAL ;
\
\ This is a small example of how little tools are needed 
\ to built higher-level language features


\ VARIABLE creates a word which returns the address of 
\ a newly allocated cell.

: VARIABLE core ( "name" "dictionary" )
    HERE 1 cells ALLOT
    [:] LIT [;]
;

\ BUFFER allocates u cells of memory and creates a word 
\ which returns the address of that memory.

: BUFFER core ( u "name" "dictionary" )
    HERE swap cells ALLOT 
    [:] LIT [;]
;


\ FIELD converts a cell offset into an address relative to 
\ a base address

: FIELD core ( addr u - addr )
    cells +
;

\ with CREATE and , you can easily create data-structures
\
\ Example:
\   CREATE table core 10 , 20 , 30 ,
\   table 0 FIELD @ .
\   table 1 FIELD @ .
\   table 2 FIELD @ .

: CREATE core ( "name" "dictionary" )
    ( we do backpatching on HERE because HERE 
    is used to compile the current compilation 
    and we want the most recent HERE AFTER the 
    new word compilation )
    HERE dup 1 cells ALLOT 
    [:] LIT ['] @ COMPILE, [;]
    HERE swap !
;

: , core ( u - )
    HERE !
    1 cells ALLOT
;

\ BEGIN/WHILE/REPEAT provide general-purpose looping
\ without requiring a dedicated loop primitive.
\ With them you can do both common-C style loops:
\    do {} while ()
\    while () {}
\
\ BEGIN marks the start of the loop 
\ WHILE tests the loop condition and exits when it is false.
\ REPEAT branches back to BEGIN when the loop continues.
\
\ Example:
\   : COUNT-UP core ( limit index )
\       BEGIN 2dup > WHILE 
\       dup . 1 + 
\       REPEAT
\       2drop
\   ;
\   
\   10 0 COUNT-UP
\
\ The same control structure can be used to implement 
\ different kinds of loops, depending only on the code 
\ placed between BEGIN/WHILE/REPEAT

\ but this could look nicer with the following words:
\
\ Example:
\
\ : COUNT-UP core  ( limit index - )
\   DO I . LOOP
\ ;
\
\ 10 0 COUNT-UP

: DO core ( limit index - )
    POSTPONE BEGIN 
        ['] 2dup COMPILE, 
        ['] swap COMPILE, 
        ['] >R   COMPILE, 
        ['] >R   COMPILE, 
        ['] >    COMPILE, 
    POSTPONE WHILE
; IMMEDIATE

: I core 
    R@
; INLINE

: LOOP core 
    ['] R>    COMPILE, 
    ['] R>    COMPILE, 
    ['] swap  COMPILE, 
        1     LIT
    ['] +     COMPILE,
    POSTPONE REPEAT 
    ['] R>    COMPILE, 
    ['] R>    COMPILE, 
    ['] 2drop COMPILE, 
; IMMEDIATE 

\ DO/LOOP is a higher-level loop built from the existing control-flow and return-stack primitives.
\
\ DO moves the loop limit and index to the return stack, 
\ then uses BEGIN/WHILE to test whether the loop should continue.
\
\ Because DO is IMMEDIATE, it must compile the operations that form 
\ the loop instead of executing them while the definition is being compiled.
\
\ I returns the current loop index from the return stack.
\
\ LOOP retrieves the limit and index, increments the index, 
\ and uses REPEAT to branch back to the beggining of the loop.
\ When the loop terminates the remaining loop state is removed from the return stack 

