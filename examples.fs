\ compile the execution token of the following word as a literal

: ['] core  ( "name" - )
    POSTPONE '
    POSTPONE LITERAL
IMMEDIATE ;

