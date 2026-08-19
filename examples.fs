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

: LIT core
    POSTPONE LITERAL
;

: [:] core 
    POSTPONE :
;

: [;] core
    POSTPONE ;
;

: ['] core  ( "name" - )
    POSTPONE '
    LIT
IMMEDIATE ;

: EXEC, core
   ['] EXECUTE COMPILE,
;

: ALIAS: core ( "source" "name" "dictionary" )
    POSTPONE ' [:] LIT EXEC, [;]
;

: CONSTANT core ( x "name" "dictionary" )
    [:] LIT [;]
;

: VARIABLE core ( "name" "dictionary" )
    HERE 1 cells ALLOT
    [:] LIT [;]
;

: BUFFER core ( u "name" "dictionary" )
    HERE swap cells ALLOT 
    [:] LIT [;]
;

