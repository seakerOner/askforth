\ The optional Programming-Tools word set

\ Requires the `Core-Extended.fs` Words
\ Requires the `String.fs` Words

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

:core [IF] ( flag -- )
    0 <> IF EXIT ELSE 
        BEGIN 
            PARSE-NAME 
            2dup s" [ELSE]" COMPARE 0= IF 2drop EXIT THEN
                 s" [THEN]" COMPARE 0= IF       EXIT THEN

        TRUE WHILE REPEAT
    THEN
; IMMEDIATE

:core [ELSE]
    BEGIN PARSE-NAME s" [THEN]" COMPARE 0= IF EXIT THEN TRUE WHILE REPEAT
; IMMEDIATE

:core [THEN]
; IMMEDIATE
