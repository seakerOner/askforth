\ The optional String word set ( www.forth-standard.org/standard/string/ )
\
\ The words 'CMOVE' and 'CMOVE>' are not implemented, didn't feel like it

\ Requires the `Core-Extended.fs` Words

:core BLANK ( addr u -- )
    DECIMAL BL FILL
;

:core COMPARE ( addr1 u1 addr2 u2 -- flag )
    depth 4 < IF error" COMPARE -> Expects ( addr1 u1 addr2 u2 -- -1|0|1 )" ( recovery -> ) ABORT THEN

    rot 2dup swap >R >R 
    MIN 0 DO 
        over I + c@ 
        over I + c@ 
        <> IF 
            over I + c@ 
            over I + c@ 
            UNLOOP R> R> 2drop 
            < >R 2drop R> IF -1 ELSE 1 THEN EXIT
        THEN      
    LOOP
    2drop R> R> 2dup 
    = IF 2drop 0 ELSE
        < IF 
            -1
        ELSE
             1
        THEN
    THEN
;

:core /STRING ( addr1 u1 n -- addr2 u2 )
    depth 3 < IF error" /STRING -> Expects ( addr1 u1 n -- addr2 u2 )" ( recovery -> ) ABORT THEN
    dup >R - swap R> chars + swap
;

:core -TRAILING ( addr u1 -- addr u2 )
    dup 0= IF EXIT THEN

    2dup + 1- >R
    swap dup >R swap R> 1- R> \ addr u1 addr-1 addr+u1-1
    -DO  
        I c@ CASE
        DECIMAL  0 OF     ENDOF
                BL OF 1- ENDOF
            ( default )
            drop UNCASE UNLOOP EXIT
        ENDCASE
    -LOOP
;

DICTIONARY substitutions

\ example:   s" banana" t" fruit" REPLACES

:core REPLACES ( addr1 u1 addr2 u2 -- )
    PAD PAD-SIZE DO-CONCAT s" sub_" CONCAT ( addr2 u2 ) END-CONCAT \ mangle addr2 name to PAD
    HERE dup >R 
    swap dup >R  COPY R@ chars ALLOT             \ ALLOT mangled name
    R> R> swap s" substitutions" CREATE-WORD POSTPONE [ swap POSTPONE ] LIT LIT POSTPONE ; 
;

:core SEARCH ( addr1 u1 addr2 u2 -- addr3 u3 flag )
    rot >R rot R> 2dup >R >R \ preserve initial string for fail path

    over >R + R>
    DO 
        2dup I over 
        COMPARE 0= IF 
            2drop I dup UNLOOP 
            R> R> + swap -
            TRUE EXIT
        THEN
    LOOP
    \ failure path
    2drop R> R> FALSE
;

:core SLITERAL ( addr1 u -- )
    ?INTERPTIME TRUE = IF error" SLITERAL -> Cannot be used in INTERPTIME." ABORT THEN
    swap LIT LIT
; IMMEDIATE


VARIABLE SUB-COUNT 

\ example:  s" My favourite fruit is %fruit%!"  buffer 100 SUBSTITUTE

:core SUBSTITUTE ( addr1 u1 addr2 u2 -- addr2 u3 n )
    0 SUB-COUNT !
    2swap 2dup + nip 0 -rot swap 
    DO 
        [CHAR] % I c@ = IF 
            I+ [CHAR] % I c@ = IF
                2dup = IF nip -1 UNLOOP EXIT THEN
                ( this pattern or similiar will repeat a lot in this word, it's storing I c@ at addr2+u3 )
                rot 2dup + I c@ swap c! -rot 1+
            ELSE 
                over I swap s" %" SEARCH 
                FALSE = IF
                    error" SUBSTITUTE -> '%' was found but not its pair." ABORT
                ELSE
                    drop I - 1+
                    HERE UNUSED DO-CONCAT s" sub_" CONCAT I swap END-CONCAT 
                    [ [FROM] substitutions ] LITERAL -rot dup 4 - >R 1- FIND R> swap 
                    ?dup 0= IF
                        >R 2dup R@ + < IF R> drop nip -1 UNLOOP EXIT THEN
                        rot 2dup + R> I 1- swap >R swap R@ 1+ COPY -rot R@ 1+ +
                        R> R> 1- + >R ( update loop counter )
                    ELSE
                        >R >R 2dup R@ + < IF R> R> 2drop nip -1 UNLOOP EXIT THEN R> R> 
                        swap R> + 1- >R  ( update loop counter )
                        >R rot 2dup + R> EXECUTE >R swap R@ COPY -rot R> +
                        SUB-COUNT @ 1+ SUB-COUNT !
                    THEN
                THEN
            THEN
        ELSE
            2dup = IF nip -1 UNLOOP EXIT THEN
            rot 2dup + I c@ swap c! -rot 1+
        THEN
    LOOP

    nip 2dup + 0 swap c!   \ null terminator
    SUB-COUNT @
;

:core UNESCAPE ( addr1 u1 addr2 -- addr2 u2 )
    -rot 2dup + -rot drop 0 -rot
    DO 
        [CHAR] % I c@ = IF
            2dup + [CHAR] % swap c! 1+
        THEN

        2dup + I c@ swap c! 1+
    LOOP
    2dup + 0 swap c! \ null terminator
;

\ not from the standard, but these are nice for multi line comments

:core {
    BEGIN 
        PARSE-NAME ?dup 0= 
        IF drop 
            REFILL FALSE = IF error" { -> Could not find '}' pair." ABORT THEN
        ELSE 
            s" }" COMPARE 0= IF EXIT THEN 
        THEN
    TRUE WHILE REPEAT
; IMMEDIATE

:core } ; IMMEDIATE
