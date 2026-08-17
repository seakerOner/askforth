\ helloooooo
( "banana" )

: LIT core
    POSTPONE LITERAL
;

: [:] core 
POSTPONE :
;

: [;] core
POSTPONE ;
;


\ compile the execution token of the following word as a literal

: ['] core  ( "name" - )
    POSTPONE '
    LIT
IMMEDIATE ;

: EXEC, core
   ['] EXECUTE COMPILE,
;

\ create a word that executes another word

: ALIAS: core ( "source" "name" "dictionary" )
    POSTPONE ' [:] LIT EXEC, [;]
;

\ create a constant word

: CONSTANT core ( x "name" "dictionary" )
    [:] LIT [;]
;

\ or you could simply do...

10 : coolname core LITERAL ; ( or ) : coolname core [ 10 ] LITERAL ;

\ and now a variable word

: VARIABLE core ( "name" "dictionary" )
    HERE 1 cells ALLOT
    [:] LIT [;]
;

\ allocate a buffer and create a word returning its address

: BUFFER core ( u "name" "dictionary" )
    HERE swap ALLOT 
    [:] LIT [;]
;

