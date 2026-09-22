\ The optional Core-Extended word set
\
\ This optional word set is not from the Forth Standard, but many of the included words are 
\ what could be native Forth words but due to simplicity they are defined inside Forth itself
\ 
\ All other optional word sets require ATLEAST this word-set.

DECIMAL 64 BITS UNSIGNED

: :core core PARSE-NAME s" core" CREATE-WORD ; 

ADD-DIC vars
: :vars core PARSE-NAME s" vars" CREATE-WORD ; 

ADD-DIC tmp
: :tmp core PARSE-NAME s" tmp"   CREATE-WORD ; 

:core -rot
    rot rot
;

:core LIT
    POSTPONE LITERAL
;

:core [:] 
    POSTPONE :
;
:core [:vars] 
    POSTPONE :vars
;

:core [;] 
    POSTPONE ;
;

:core ['] 
    POSTPONE '
    LIT
; IMMEDIATE

:core EXEC,
   ['] EXECUTE COMPILE,
;

:core CONSTANT ( w "name" -- )
    [:vars] LIT [;]
;

:core VARIABLE ( "name"  -- )
    HERE 1 cells ALLOT
    [:vars] LIT [;]
;

:core BUFFER ( u "name" -- )
    HERE swap cells ALLOT 
    [:vars] LIT [;]
;

:core FIELD ( addr u -- addr )
    cells +
;

:core CREATE ( "name" "dictionary" -- )
    HERE dup 1 cells ALLOT 
    [:vars] LIT ['] @ COMPILE, [;]
    HERE swap !
;

:core , ( d -- )
    HERE !
    1 cells ALLOT
;

:core DO ( limit index -- )
    POSTPONE BEGIN 
        ['] 2dup COMPILE, 
        ['] swap COMPILE, 
        ['] >R   COMPILE, 
        ['] >R   COMPILE, 
        ['] >    COMPILE, 
    POSTPONE WHILE
; IMMEDIATE

:core I ( -- d )
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

:core -DO ( limit index -- )
    POSTPONE BEGIN 
        ['] 2dup COMPILE, 
        ['] swap COMPILE, 
        ['] >R   COMPILE, 
        ['] >R   COMPILE, 
        ['] <>   COMPILE, 
    POSTPONE WHILE
; IMMEDIATE

:core -LOOP  
    ['] R>    COMPILE, 
    ['] R>    COMPILE, 
    ['] swap  COMPILE, 
        1     LIT
    ['] -     COMPILE,
    POSTPONE REPEAT 
    ['] R>    COMPILE, 
    ['] R>    COMPILE, 
    ['] 2drop COMPILE, 
; IMMEDIATE 


:core UNLOOP
    R> R> 2drop 
;

:core UNTIL 
    ['] invert COMPILE,
    POSTPONE WHILE 
    POSTPONE REPEAT
; IMMEDIATE

\ FIX: case is leaking to RSTACK uppon use

:core CASE ( n -- )
    ['] dup COMPILE, 
    ['] >R  COMPILE,
    0
; IMMEDIATE

:core OF   ( n -- )
    ['] = COMPILE, 
    POSTPONE IF   
; IMMEDIATE

:core ?OF  ( flag -- )
    POSTPONE IF
; IMMEDIATE

:core ENDOF   
    POSTPONE ELSE 
    1 +
    ['] R@ COMPILE,
; IMMEDIATE

:core ENDCASE 
    0 DO POSTPONE THEN LOOP
    ['] R>   COMPILE,
    ['] drop COMPILE,
; IMMEDIATE

:core UNCASE
    R> drop 
;

:core [DEFINED]
    depth 0= IF error" [DEFINED] -> Expects Dictionary address" ( recovery ) ABORT ELSE
    ?dup  0= IF error" [DEFINED] -> NULL address of Dictionary" ( recovery ) ABORT THEN
    POSTPONE [FIND] 0<>
; IMMEDIATE

:core [UNDEFINED]
    depth 0= IF error" [UNDEFINED] -> Expects Dictionary address" ( recovery ) ABORT ELSE
    ?dup  0= IF error" [UNDEFINED] -> NULL address of Dictionary" ( recovery ) ABORT THEN
    POSTPONE [FIND] 0=
; IMMEDIATE

:core TO 
    [FROM] vars LITERAL \ the dictionary we will search for words (could also be the 'tmp' dictionary)

    TRUE CASE 
        ?COMPTIME OF
            POSTPONE [FIND] 
            ?dup 0= IF 
                error" TO -> Unknown VARIABLE." ABORT 
            THEN COMPILE,
            ['] ! COMPILE,
        ENDOF 
        ?INTERPTIME OF
            POSTPONE [FIND] 
            ?dup 0= IF 
                error" TO -> Unknown VARIABLE." ABORT 
            THEN
            EXECUTE !
        ENDOF
        drop 
    ENDCASE
; IMMEDIATE

VARIABLE concat@capacity
VARIABLE concat@items 
VARIABLE concat@idx

0 TO concat@capacity
0 TO concat@items
0 TO concat@idx

\ addr is the scratch buffer where the string will be copied
\ u is the capacity of that buffer
:core DO-CONCAT ( addr u -- ) 
         TO concat@capacity
         TO concat@items
       0 TO concat@idx   
;

:core CONCAT ( addr u -- )
    dup [ concat@idx ] LITERAL @ + [ concat@capacity ] LITERAL @ >= IF error" CONCAT -> Buffer overflow." ( recovery ) ABORT THEN

    [ concat@items ]   LITERAL @ concat@idx @ + >R         \ end of base string to append
    dup [ concat@idx ] LITERAL @ + TO concat@idx         \ update the base index
    R> swap
    \ str2_addr str1_addr_end str2_len copy
    COPY
;

:core END-CONCAT
    depth 1 > IF CONCAT THEN  

    0
    [ concat@idx ]   LITERAL  @ 
    [ concat@items ] LITERAL  @ + !     \ store string null terminator

    [ concat@items ] LITERAL @
    [ concat@idx   ] LITERAL @
;

[FROM] vars [FORGET] concat@capacity
[FROM] vars [FORGET] concat@items
[FROM] vars [FORGET] concat@idx

:core MIN ( n1 n2 -- n3 )
    2dup > IF swap THEN drop
;

:core MAX ( n1 n2 -- n3 )
    2dup < IF swap THEN drop
;

HERE 512 chars ALLOT CONSTANT PAD
512 CONSTANT PAD-SIZE

HERE 128 chars ALLOT 

CREATE TMPSTRING  , 0 , 128 ,

\ for temporary string stored on a transient buffer!

:core t" 
   0  [ TMPSTRING 1 FIELD ] LITERAL !

   BEGIN 
        SOURCE drop >IN @ + 
        SOURCE +  
        >= IF error" t'' ->  Delimiter not found " ABORT THEN

        [ TMPSTRING 1 FIELD ] LITERAL @ 1 + 
        [ TMPSTRING 2 FIELD ] LITERAL @ > IF error" t'' -> temporary string can only hold < 128 ascii characteres." ABORT THEN

        SOURCE drop >IN @ + c@ 
        [ TMPSTRING 0 FIELD ] LITERAL @
        [ TMPSTRING 1 FIELD ] LITERAL @ + c!    \ store >IN character

        [ TMPSTRING 1 FIELD ] LITERAL @  1 +
        [ TMPSTRING 1 FIELD ] LITERAL !         
        >IN @ 1 + >IN !                         \ next character

   SOURCE drop >IN @ + c@ DECIMAL 34 = UNTIL    \ loop until we find '"'

   [ TMPSTRING 0 FIELD ] LITERAL @
   [ TMPSTRING 1 FIELD ] LITERAL @

   \ adjust >IN to the start of next character or end of buffer
   BEGIN
    >IN @ 1 + >IN !                         
   SOURCE drop >IN @ + c@ DECIMAL 32 <> 
   SOURCE  nip >IN @ <> and UNTIL

; IMMEDIATE
