\ The optional Programming-Tools word set

\ Requires the `Core-Extended.fs` Words
\ Requires the `String.fs` Words

\ Different from the Forth standard `[DEFINED]` and `[UNDEFINED]' expect a dictionary xt (using `[FROM] <dict_name>`)

:core [DEFINED] ( dict_xt -- addr ) 
    depth 0= IF error" [DEFINED] -> Expects Dictionary address" ( recovery ) ABORT ELSE
    ?dup  0= IF error" [DEFINED] -> NULL address of Dictionary" ( recovery ) ABORT THEN
    POSTPONE [FIND] 0<>
; IMMEDIATE

:core [UNDEFINED] ( dict_xt -- addr )
    depth 0= IF error" [UNDEFINED] -> Expects Dictionary address" ( recovery ) ABORT ELSE
    ?dup  0= IF error" [UNDEFINED] -> NULL address of Dictionary" ( recovery ) ABORT THEN
    POSTPONE [FIND] 0=
; IMMEDIATE

:core [IF] ( flag -- )
    0 <> IF EXIT ELSE 
        BEGIN 
            PARSE-NAME ?dup 0= 
            IF drop 
                REFILL FALSE = IF error" [IF] -> Could not find [ELSE] or [THEN] in input buffer." ABORT THEN
            ELSE
                2dup s" [ELSE]" COMPARE 0= IF 2drop EXIT THEN
                     s" [THEN]" COMPARE 0= IF       EXIT THEN
            THEN
        TRUE WHILE REPEAT
    THEN
; IMMEDIATE

:core [ELSE]
    BEGIN 
        PARSE-NAME  ?dup 0= 
        IF drop 
            REFILL FALSE = IF error" [ELSE] -> Could not find [THEN] in input buffer" THEN
        ELSE 
            s" [THEN]" COMPARE 0= IF EXIT THEN 
        THEN
    TRUE WHILE REPEAT
; IMMEDIATE

:core [THEN] ; IMMEDIATE

:core ? ( addr -- ) @ . ;
