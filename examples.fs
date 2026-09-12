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

\ if you want a global dictionary-like sintax to define your words like in a traditional Forth 
\ do the following:
\
\ s" :" s" core" CREATE-WORD PARSE-NAME s" core" [ swap ] LITERAL LITERAL CREATE-WORD ;

\ for the following of this documente I will use the word :core as a nicety
\ this is a hint on how to use the dictionaries

: :core core PARSE-NAME s" core" [ swap ] LITERAL LITERAL CREATE-WORD ; 

ADD-DIC vars
: :vars core PARSE-NAME s" vars" [ swap ] LITERAL LITERAL CREATE-WORD ; 

ADD-DIC tmp
: :tmp core PARSE-NAME s" tmp"   [ swap ] LITERAL LITERAL CREATE-WORD ; 

\ LIT is a small helper that compiles a literal into
\ the word currently being compiled.

:core LIT
    POSTPONE LITERAL
;

\
\ Now the sintax for ':' is " : word_name {body} ; "

\ [:] postpones the execution of : 
\
\ This allows a word to start compiling another word
\ while it is itself being executed

:core [:] 
    POSTPONE :
;
:core [:vars] 
    POSTPONE :vars
;
\ [;] postpones the execution of ;
\ 
\ Together with [:], this allows words to create new
\ word definitions programmatically

:core [;]
    POSTPONE ;
;

\ ['] compiles the execution token of the following word
\ as a literal.

:core ['] ( "name" - )
    POSTPONE '
    LIT
; IMMEDIATE

\ EXEC, compiles the word EXECUTE into the current definition
\ It a small convenience word built from ['] and COMPILE,

:core EXEC,
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

:core ALIAS: ( "source" "name" "dictionary" )
    POSTPONE ' [:] LIT EXEC, [;]
;

\ CONSTANT creates a word which returns a fixed value.
\
\ Notice that CONSTANT does not need a dedicated primitive:
\ it is simply a new word containing a compiled literal.

:core CONSTANT ( x "name" )
    [:vars] LIT [;]
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

:core VARIABLE ( "name" )
    HERE 1 cells ALLOT
    [:vars] LIT [;]
;

\ BUFFER allocates u cells of memory and creates a word 
\ which returns the address of that memory.

:core BUFFER ( u "name" )
    HERE swap cells ALLOT 
    [:vars] LIT [;]
;


\ FIELD converts a cell offset into an address relative to 
\ a base address

:core FIELD ( addr u - addr )
    cells +
;

\ with CREATE and , you can easily create data-structures
\
\ Example:
\   CREATE table 10 , 20 , 30 ,
\   table 0 FIELD @ .
\   table 1 FIELD @ .
\   table 2 FIELD @ .

:core CREATE ( "name" "dictionary" )
    ( we do backpatching on HERE because HERE 
    is used to compile the current compilation 
    and we want the most recent HERE AFTER the 
    new word compilation )
    HERE dup 1 cells ALLOT 
    [:vars] LIT ['] @ COMPILE, [;]
    HERE swap !
;

:core , ( u - )
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

:core DO ( limit index - )
    POSTPONE BEGIN 
        ['] 2dup COMPILE, 
        ['] swap COMPILE, 
        ['] >R   COMPILE, 
        ['] >R   COMPILE, 
        ['] >    COMPILE, 
    POSTPONE WHILE
; IMMEDIATE

\ At the moment of writing this the search order for words on AskForth is oldest_dictionary->newest_dictionary 
\ And on each dictionary the search order is newest_word->oldest_word
\ 
\ In case of duplicate words or you simply want to specify the dictionary the word must come from 
\ you can do the following:
\
\ :core DO ( limit index - )
\     POSTPONE BEGIN 
\         [FROM] core [FIND] 2dup LITERAL COMPILE, 
\         [FROM] core [FIND] swap LITERAL COMPILE, 
\         [FROM] core [FIND] >R   LITERAL COMPILE, 
\         [FROM] core [FIND] >R   LITERAL COMPILE, 
\         [FROM] core [FIND] >    LITERAL COMPILE, 
\     POSTPONE WHILE
\ ; IMMEDIATE


:core I 
    R@
; INLINE

:core LOOP 
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

:core UNLOOP
    R> R> 2drop 
;


\ Example:
\ :core ensure{.s}
\   [FROM] core [DEFINED] .s IF 
\   ." .s is defined!"
\   THEN
\ ;

:core [DEFINED]
    depth 0= IF error" [DEFINED] -> Expects Dictionary address" ( recovery ) ABORT ELSE
    ?dup  0= IF error" [DEFINED] -> NULL address of Dictionary" ( recovery ) ABORT THEN
    POSTPONE [FIND] 0<>
; IMMEDIATE

\ After 'error"' its good practice to add recovery code ,
\ in the case the user decides to continue execution after the error.
\ For this example we just abort execution!

:core [UNDEFINED]
    depth 0= IF error" [UNDEFINED] -> Expects Dictionary address" ( recovery ) ABORT ELSE
    ?dup  0= IF error" [UNDEFINED] -> NULL address of Dictionary" ( recovery ) ABORT THEN
    POSTPONE [FIND] 0=
; IMMEDIATE

\ words to concat strings together
\
\ DO-CONCAT uses the current HERE as the destination buffer.
\ The programmer must not allocate or compile into that region 
\ until END-CONCAT has completed
\
\ Example:
\
\  DO-CONCAT
\       s" Hello "  CONCAT 
\       s" World! " CONCAT
\       s" This is a test! Goodbye :D" 
\  END-CONCAT TYPE
\

:core store ! ; INLINE
:core view  @ ; INLINE

CREATE concatTbl 0 , 0 , 0 ,

:tmp items  0 FIELD ;
:tmp idx    1 FIELD ;
:tmp offset 2 FIELD ;

:core DO-CONCAT 
    HERE concatTbl items store
       0 concatTbl   idx store
;


:core CONCAT ( str2_addr str2_len - )
    concatTbl items   view 
    concatTbl idx     view + >R                         \ end of base string to append
    dup concatTbl idx view + concatTbl idx store        \ update the base index
    R> swap
    \ str2_addr str1_addr_end str2_len copy
    COPY
    concatTbl offset view 1 + concatTbl offset store
;

:core END-CONCAT
    depth 1 > IF CONCAT THEN  

    concatTbl idx     view dup 
    concatTbl items   view + 0 swap store
    concatTbl offset  view - 
    concatTbl idx    store

    [ concatTbl items ] LITERAL view
    [ concatTbl idx   ] LITERAL view
;

[FROM] vars [FORGET] concatTbl
[FROM] tmp  [FORGET] items  
[FROM] tmp  [FORGET] idx   
[FROM] tmp  [FORGET] offset 
