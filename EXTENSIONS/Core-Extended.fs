\ The optional Core-Extended word set
\
\ This optional word set is not from the Forth Standard, but many of the included words are 
\ what could be native Forth words but due to simplicity they are defined inside Forth itself
\ 
\ All other optional word sets require ATLEAST this word-set.

#64 BITS UNSIGNED

\ to allocate parsed words we want to persist in memory
: SAVE-NAME core  ( c-addr u -- c-addr )
    ALIGN HERE swap dup >R COPY
    HERE R> chars ALLOT
;

\ old_addr new_addr n_bytes
: :core core PARSE-NAME tuck SAVE-NAME swap s" core" CREATE-WORD ; 

DICTIONARY vars 

: :vars core PARSE-NAME tuck SAVE-NAME swap s" vars" CREATE-WORD ; 

:core -rot rot rot ;

:core 1+ 1 + ; INLINE

:core 1- 1 - ; INLINE

:core LIT
    POSTPONE LITERAL ;

\ RLITERAL is for storing HERE addresses in threaded code in a image portable way

:core RLIT
    POSTPONE RLITERAL ;

:core [:] 
    POSTPONE : ;

:core [:vars] 
    POSTPONE :vars ;

:core [;] 
    POSTPONE ; ;

:core ['] 
    POSTPONE ' RLIT ; IMMEDIATE

:core EXEC,
   ['] EXECUTE COMPILE, ;

\ not suitable for HERE addresses
:core CONSTANT ( w "name" -- )
   [:vars] LIT [;] ;

\ suitable for HERE addresses
:core ADDRESS ( w "name" -- )
   [:vars] RLIT [;] ;

:core VARIABLE ( "name"  -- )
    ALIGN HERE 1 cells ALLOT
    [:vars] RLIT [;] ;

:core BUFFER ( u "name" -- )
    ALIGN HERE swap cells ALLOT 
    [:vars] RLIT [;] ;

:core CBUFFER ( u "name" -- )
    ALIGN HERE swap chars ALLOT 
    [:vars] RLIT [;] ;

:core FIELD ( addr u -- addr )
    cells + ;

\ CREATE and , are not suitable for storing portable HERE addresses
:core CREATE ( "name" "dictionary" -- )
    ALIGN HERE dup 1 cells ALLOT 
    [:vars] RLIT ['] @ COMPILE, [;]
    ALIGN HERE swap ! ;

:core , ( d -- )
    HERE !
    1 cells ALLOT ;

:core DO ( limit index -- )
    POSTPONE BEGIN 
        ['] 2dup COMPILE, 
        ['] swap COMPILE, 
        ['] >R   COMPILE, 
        ['] >R   COMPILE, 
        ['] >    COMPILE, 
    POSTPONE WHILE ; IMMEDIATE

:core I ( -- d )
    R@ ; INLINE

:core I+ ( -- ) 
    R> 1+ >R ;

:core LOOP ( limit index -- )
    ['] R>    COMPILE, 
    ['] R>    COMPILE, 
    ['] swap  COMPILE, 
        1     LIT
    ['] +     COMPILE,
    POSTPONE REPEAT 
    ['] R>    COMPILE, 
    ['] R>    COMPILE, 
    ['] 2drop COMPILE, ; IMMEDIATE 

:core -DO ( limit index -- )
    POSTPONE BEGIN 
        ['] 2dup COMPILE, 
        ['] swap COMPILE, 
        ['] >R   COMPILE, 
        ['] >R   COMPILE, 
        ['] <>   COMPILE, 
    POSTPONE WHILE ; IMMEDIATE

:core -LOOP  
    ['] R>    COMPILE, 
    ['] R>    COMPILE, 
    ['] swap  COMPILE, 
        1     LIT
    ['] -     COMPILE,
    POSTPONE REPEAT 
    ['] R>    COMPILE, 
    ['] R>    COMPILE, 
    ['] 2drop COMPILE, ; IMMEDIATE 

:core UNLOOP
    R> R> 2drop ;

:core UNTIL 
    ['] invert COMPILE,
    POSTPONE WHILE 
    POSTPONE REPEAT ; IMMEDIATE

:core AGAIN
    ['] TRUE COMPILE,
    POSTPONE WHILE 
    POSTPONE REPEAT ; IMMEDIATE

:core CASE ( n -- )
    ['] dup COMPILE, 
    ['] >R  COMPILE,
    0 ; IMMEDIATE

:core OF   ( n -- )
    ['] = COMPILE, 
    POSTPONE IF ; IMMEDIATE

:core ?OF  ( flag -- )
    POSTPONE IF ; IMMEDIATE

:core ENDOF   
    POSTPONE ELSE 
    1+
    ['] R@ COMPILE, ; IMMEDIATE

:core ENDCASE 
    0 DO POSTPONE THEN LOOP
    ['] R>   COMPILE,
    ['] drop COMPILE, ; IMMEDIATE

:core UNCASE
    R> drop  ;

:core [CHAR] 
    PARSE-NAME drop
    c@
    ?COMPTIME TRUE = IF LIT THEN ; IMMEDIATE

:core TO 
    [FROM] vars RLITERAL \ the dictionary we will search for words (could also be the 'tmp' dictionary)

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
    ENDCASE ; IMMEDIATE

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
       0 TO concat@idx ;

:core CONCAT ( addr u -- )
    dup concat@idx @ + concat@capacity @ >= IF error" CONCAT -> Buffer overflow." ( recovery ) ABORT THEN

    concat@items @ concat@idx @ + >R         \ end of base string to append
    dup concat@idx @ + TO concat@idx         \ update the base index
    R> swap
    \ str2_addr str1_addr_end str2_len copy
    COPY ;

:core END-CONCAT
    depth 1 > IF CONCAT THEN  

    0 
    concat@idx   @ 
    concat@items @ + !     \ store string null terminator

    concat@items @
    concat@idx   @ ;

[FROM] vars [FORGET] concat@capacity
[FROM] vars [FORGET] concat@items
[FROM] vars [FORGET] concat@idx

:core MIN ( n1 n2 -- n3 )
    2dup > IF swap THEN drop ;

:core MAX ( n1 n2 -- n3 )
    2dup < IF swap THEN drop ;

\ ALIGN HERE #512 chars ALLOT ADDRESS PAD
#512 CONSTANT PAD-SIZE
#512 CBUFFER  PAD

\ ADDRESS TMPSTRING 

#128 CONSTANT TMPSTRING>cap  
#128  CBUFFER TMPSTRING
     VARIABLE TMPSTRING>idx 
  #0       TO TMPSTRING>idx

\ for temporary string stored on a transient buffer!

:core t" 
   0  TMPSTRING>idx !

   BEGIN 
        SOURCE drop >IN @ + 
        SOURCE +  
        >= IF error" t'' ->  Delimiter not found " ABORT THEN

        TMPSTRING>idx @ 1+ 
        TMPSTRING>cap > IF error" t'' -> temporary string can only hold < 128 ascii characteres." ABORT THEN

        SOURCE drop >IN @ + c@ 
        TMPSTRING 
        TMPSTRING>idx @ + c!    \ store >IN character

        TMPSTRING>idx @  1+
        TMPSTRING>idx !         
        >IN @ 1+ >IN !                         \ next character

   SOURCE drop >IN @ + c@ #34 = UNTIL    \ loop until we find '"'

   TMPSTRING    
   TMPSTRING>idx @

   \ adjust >IN to the start of next character or end of buffer
   BEGIN
    >IN @ 1+ >IN !                         
   SOURCE drop >IN @ + c@ #32 <> 
   SOURCE  nip >IN @ <> and UNTIL ; IMMEDIATE

#32 CONSTANT BL 

:core BOUNDS ( addr u1 -- addr+u1 addr ) over + swap ;
