\ The optional String word set ( www.forth-standard.org/standard/string/ )
\ Requires the `Core-Extended.fs` Words

:core BLANK ( addr u -- )
    DECIMAL 32 FILL
;

:core COMPARE ( addr1 u1 addr2 u2 -- flag )
    depth 4 < IF error" COMPARE -> Expects ( addr1 u1 addr2 u2 -- -1|0|1 )" ( recovery -> ) ABORT THEN

    rot 2dup swap >R >R 
    MIN 0 DO 
        over I + c@ 
        over I + c@ 
        <> IF 
            UNLOOP R> R> 2drop 
            over I + c@ 
            over I + c@ 
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

    2dup + 1 - >R
    swap dup >R swap R> 1 - R> \ addr u1 addr-1 addr+u1-1
    -DO  
        I c@ CASE
        DECIMAL  0 OF     ENDOF
                32 OF 1 - ENDOF
            ( default )
            drop UNCASE UNLOOP EXIT
        ENDCASE
    -LOOP
;

ADD-DIC substitutions

\ example:   s" banana" t" fruit" REPLACES

:core REPLACES ( addr1 u1 addr2 u2 -- )
    PAD PAD-SIZE DO-CONCAT s" sub_" CONCAT ( addr2 u2 ) END-CONCAT .s cr \ mangle addr2 name to PAD
    HERE dup >R 
    swap dup >R  COPY R@ chars ALLOT             \ ALLOT mangled name
    R> R> swap s" substitutions" CREATE-WORD POSTPONE [ swap POSTPONE ] LIT LIT POSTPONE ; 
;


\ example:  s" My favourite fruit is %fruit%!"  buffer 100 SUBSTITUTE

:core SUBSTITUTE ( addr1 u1 addr2 u2 -- addr2 u3 n )
;


