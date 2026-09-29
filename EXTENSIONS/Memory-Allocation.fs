\ The optional Memory-Allocation  word set ( https://forth-standard.org/standard/memory )

\ Requires the `Core-Extended.fs` Words
\ Requires the `String.fs` Words
\ Requires the `Programming-Tools.fs` Words

?SYSTEM-LINUX   [IF] FOREIGN libc.so.6    [THEN]
?SYSTEM-WINDOWS [IF] FOREIGN ucrtbase.dll [THEN]

ADD-DIC foreign

FUNCTION: malloc  ( u64 -- addr )           foreign
FUNCTION: realloc ( addr u64 -- addr )      foreign
FUNCTION: free    ( addr -- void )          foreign

?SYSTEM-LINUX   [IF] FUNCTION: __errno_location ( void -- addr ) foreign [THEN]
?SYSTEM-WINDOWS [IF] FUNCTION: _get_errno ( addr -- addr )      foreign VARIABLE WIN_ERRNO [THEN]

:core ERRNO 
    [ ?SYSTEM-LINUX ] 
        [IF] [FROM] foreign [FIND] __errno_location LITERAL EXECUTE @ [THEN]

    [ ?SYSTEM-WINDOWS ] 
        [IF] WIN_ERRNO [FROM] foreign [FIND] _get_errno LITERAL EXECUTE ?dup 0= IF WIN_ERRNO @ THEN [THEN]
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
