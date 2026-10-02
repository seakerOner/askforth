{  -----------------------------------------------------------------------------------------
    The optional Memory-Allocation  word set ( https://forth-standard.org/standard/memory )

    Requires the `Core-Extended.fs` Word Set
    Requires the `String.fs` Word Set
    Requires the `Programming-Tools.fs` Word Set
  ------------------------------------------------------------------------------------------ }

?SYSTEM-LINUX   [IF] FOREIGN libc.so.6    [THEN]
?SYSTEM-WINDOWS [IF] FOREIGN ucrtbase.dll [THEN]

DICTIONARY foreign

FUNCTION: malloc  ( u64 -- addr )           foreign
FUNCTION: realloc ( addr u64 -- addr )      foreign
FUNCTION: free    ( addr -- void )          foreign

?SYSTEM-LINUX   [IF] 
    FUNCTION: __errno_location ( void -- addr ) foreign 
[THEN]
?SYSTEM-WINDOWS [IF] 
    FUNCTION: _get_errno ( addr -- addr )       foreign 
    VARIABLE WIN_ERRNO 
[THEN] 

FUNCTION: strerror ( u64 -- u64 )               foreign

: ERRNO foreign
    [ ?SYSTEM-LINUX ] 
        [IF] [FROM] foreign [FIND] __errno_location LITERAL EXECUTE @ [THEN]

    [ ?SYSTEM-WINDOWS ] 
        [IF] 
            WIN_ERRNO [FROM] foreign [FIND] _get_errno LITERAL EXECUTE 
            ?dup 0= IF WIN_ERRNO @ THEN 
        [THEN] 
;

{ This string should not be modified or preserved, you may copy the string into your own buffer and then preserved it that way ( e.g: usin ERROR ) }
:core IOR.STR ( ior -- c-addr u )
    strerror
    { strerror doesnt tell us the length of the string... so we search for the null terminator }
    0 BEGIN 
        2dup + c@ 
    0<> WHILE 1+ REPEAT 
;

:core ALLOCATE ( u -- a-addr ior )
    [FROM] foreign [FIND] malloc LITERAL EXECUTE 
    dup 0= IF ERRNO ELSE 0 THEN
;
:core FREE ( a-addr -- ior )
    [FROM] foreign [FIND] free LITERAL EXECUTE 0
;
:core RESIZE ( a-addr1 u -- a-addr2 ior )
    over -rot [FROM] foreign [FIND] realloc LITERAL EXECUTE
    ?dup 0= IF ERRNO ELSE rot drop 0 THEN
;
