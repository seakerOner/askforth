{  -----------------------------------------------------------------------------------------------------------------------------------------
    The optional File-Access  word set ( https://forth-standard.org/standard/file )

     Introduction:
     These words provide access to mass storage in the form of "files" under the following assumptions:

        - files are provided by a host operating system;
        - file names are represented as character strings;
        - the format of file names is determined by the host operating system;
        - an open file is identified by a single-cell file identifier (fileid);
        - file-state information (e.g., position, size) is managed by the host operating system;
        - file contents are accessed as a sequence of characters;
        - file read operations return an actual transfer count, which can differ from the requested transfer count.

    ------------------------------------------------------

    Additional terms:

    file-access method:
        - A permissible means of accessing a file, such as "read/write" or "read only". 
    file position:
        - The character offset from the start of the file. 
    input file:
        - The file, containing a sequence of lines, that is the input source. 

    ------------------------------------------------------

    Additional usage requirements:

        Data types:
         ____________________________________________________
        | SymbolData | type 	           |  Size on stack  |
        |------------|---------------------|-----------------|
        | fam 	     | file access method  |          1 cell | 
        | fileid     | file identifier 	   |          1 cell |
        |____________|_____________________|_________________|
        
        ------------------------------------------------------
        File identifiers:

        File identifiers are implementation-dependent single-cell values that are passed to file operators to designate specific files. 
        Opening a file assigns a file identifier, which remains valid until closed.

        ------------------------------------------------------
        File names:

        A character string containing the name of the file. The file name may include an implementation-dependent path name. 
        The format of file names is implementation defined. 

        ------------------------------------------------------

        Requires the `Core-Extended.fs`     Word Set
        Requires the `String.fs`            Word Set
        Requires the `Programming-Tools.fs` Word Set
        Requires the `Memory-Allocation.fs` Word Set

  ----------------------------------------------------------------------------------------------------------------------------------------- }

\ Include foreign functions (LIBC) from the OS
FUNCTION: fopen   ( u64 u64 -- u64 )                foreign 
FUNCTION: freopen ( u64 u64 u64 -- u64 )            foreign 
FUNCTION: fclose  ( u64 -- u64 )                    foreign 
FUNCTION: remove  ( u64 -- u64 )                    foreign 
FUNCTION: rename ( u64 u64 -- u64 )                 foreign 

FUNCTION: fread   ( u64 u64 u64 u64 -- u64 )        foreign 
FUNCTION: fwrite  ( u64 u64 u64 u64 -- u64 )        foreign 
FUNCTION: fgetc   ( u64 -- u64 )                    foreign 
FUNCTION: fputc   ( u64 u64 -- u64 )                foreign 
FUNCTION: ungetc  ( u64 u64 -- u64 )                foreign 
FUNCTION: ferror  ( u64 -- u64 )                    foreign 

FUNCTION: ftell   ( u64 -- u64 )                    foreign 
FUNCTION: fseek   ( u64 u64 u64 -- u64 )            foreign 

FUNCTION: fflush  ( u64 -- u64 )                    foreign 

?SYSTEM-LINUX [IF]
    FUNCTION: fileno ( u64 -- u64 )                 foreign 
    FUNCTION: ftruncate ( u64 u64 -- u64 )          foreign 
[THEN]
?SYSTEM-WINDOWS [IF]
    FUNCTION: _fileno   ( u64 -- u64 )              foreign 
    FUNCTION: _chsize_s ( u64 u64 -- u64 )          foreign 
[THEN]

:core R/O ( -- fam ) s" rb"  drop ;

:core R/W ( -- fam ) s" r+b" drop ;

:core W/O ( -- fam ) s" wb"  drop ;

{  -----------------------------------------------------------------------------------------------------------------------------------------

    BIN is a NOOP as all fams are already in binary mode. 

    This is to avoid complexity since text mode does newline translation and file termination handling differently depending on the host.

   ----------------------------------------------------------------------------------------------------------------------------------------- }

:core BIN ( -- ) ;

{  ----------------------------------------------------------------------------------------------------------------------------------------- 

    MODE-FILE is not part from the Standard Forth lexicon.

    MODE-FILE will let you change the currently active fileid's fam without closing the file stream; 

    Returns 0 on success or IOR number from the OS

   ----------------------------------------------------------------------------------------------------------------------------------------- }

:core MODE-FILE   ( fileid fam -- ior )
    swap 0 -rot
    freopen 0= IF ERRNO ELSE 0 THEN
;

\ ior is 0 if successful or returns error number from the OS
:core CLOSE-FILE  ( fileid -- ior )
    fclose 0= IF 0 ELSE ERRNO THEN
;

:core CREATE-FILE  ( c-addr u fam -- fileid ior )
    nip swap W/O fopen 
    dup 0= IF drop ERRNO EXIT THEN
    tuck swap MODE-FILE 
;

:core DELETE-FILE  ( c-addr u -- ior )
    drop remove 0= IF 0 ELSE ERRNO THEN
;

:core FILE-POSITION ( fileid -- ud ior )
    ftell dup -1 = IF ERRNO THEN 0
;

:core FILE-SIZE ( fileid -- ud ior )
    dup FILE-POSITION   0<> IF   nip ERRNO EXIT THEN

    swap \ preserve original file position back on the stack

    \ 2 is the magical number for SEEK_END macro on C
    dup 0 2 fseek       0<> IF   nip ERRNO EXIT THEN

    dup FILE-POSITION   0<> IF 2drop ERRNO EXIT THEN

    \ setting back to the original file position.
    \ 0 is the magical number for SEEK_SET macro on C
    -rot swap 0 fseek   0<> IF       ERRNO EXIT THEN 0
;

\ TODO: decide how to go about this
\ :core FILE-STATUS ( c-addr u -- x ior )
\ ;

{  ----------------------------------------------------------------------------------------------------------------------------------------- 
    NOTE for FLUSH-FILE

     if fileid is NULL (0) then FLUSH-FILE flushes ALL open output streams 
   ----------------------------------------------------------------------------------------------------------------------------------------- }

:core FLUSH-FILE ( fileid -- ior )
    fflush 0<> IF ERRNO THEN 0
;

:core OPEN-FILE ( c-addr u fam -- fileid ior )
    nip fopen dup 0= IF ERRNO ELSE 0 THEN
;

:core READ-FILE ( c-addr u1 fileid -- u2 ior )
    dup  FILE-POSITION 0<> 
                    IF 2drop 2drop 0 ERRNO EXIT THEN

    over FILE-SIZE     0<> 
                    IF 2drop 2drop 0 ERRNO EXIT THEN

    >= IF 2drop drop 0 0 EXIT THEN

    1 chars -rot
    ( c-addr size u1 fileid )
    dup >R fread 

    \ check for error on stream
    R> ferror 0<> IF ERRNO ELSE 0 THEN
;

:core READ-LINE ( c-addr u1 fileid -- u2 flag ior ) 
    dup  FILE-POSITION 0<> 
                    IF 2drop       drop 0 FALSE ERRNO EXIT THEN

    over FILE-SIZE     0<> 
                    IF 2drop 2drop drop 0 FALSE ERRNO EXIT THEN

    >= IF 2drop drop 0 FALSE 0 EXIT THEN

    0 BEGIN 
        over
        fgetc CASE
            10 (  LF ) OF nip nip nip TRUE 0 UNCASE EXIT ENDOF
            13 (  CR ) OF over 
                        fgetc dup 
                            10 = IF drop nip nip nip TRUE 0 UNCASE EXIT ELSE 
                                    over ungetc 
                                    -1 = IF nip nip nip FALSE ERRNO UNCASE EXIT THEN
                                THEN
                       ENDOF
            -1 ( EOF ) OF nip nip nip TRUE 0 UNCASE EXIT ENDOF
            ( default )
            >R >R over R@ = IF R> R> drop nip nip nip TRUE 0 UNCASE EXIT THEN 
            R> R> >R   ( c-addr u1 fileid counter ) 
            >R rot R>
            2dup + R> swap c! 1+
            \ restore order
            >R -rot R>
        ENDCASE
    AGAIN
;

:core RENAME-FILE ( c-addr1 u1 c-addr2 u2 -- ior ) 
    drop nip rename 0<> IF ERRNO ELSE 0 THEN
;

:core REPOSITION-FILE ( ud fileid -- ior ) 
    dup FILE-SIZE 0<> IF 2drop drop ERRNO EXIT THEN
    >R over R>      > IF 2drop 22 ( ERRNO for Invalid Argument ) EXIT THEN 

    swap 0 fseek 0<> IF ERRNO ELSE 0 THEN
;

{  ----------------------------------------------------------------------------------------------------------------------------------------- 

    NOTE for RESIZE-FILE file expansion behavior:

    After resizing the file the actual size may not immediatly display on FILE-SIZE or outside your program.
    For example on my machine doing on Bash: 

    ```Bash 
        ls -lah
    ```
    Said my test file 18 bytes, but:

    ```Bash
        du -sh <filename>
    ```
    Says 4K

   ----------------------------------------------------------------------------------------------------------------------------------------- }
:core RESIZE-FILE ( ud fileid -- ior ) 
    dup FLUSH-FILE drop

    [ ?SYSTEM-LINUX ]   [IF]
        fileno dup     0<> IF 2drop ERRNO EXIT THEN
        swap ftruncate 0<> IF ERRNO ELSE 0 THEN
    [THEN]

    [ ?SYSTEM-WINDOWS ] [IF]
        _fileno dup     0<> IF 2drop ERRNO EXIT THEN 
        swap _chsize_s  0<> IF ERRNO ELSE 0 THEN
    [THEN]
;

:core WRITE-FILE ( c-addr u fileid -- ior ) 
    dup  FILE-POSITION 0<> 
                    IF 2drop 2drop  ERRNO EXIT THEN

    over FILE-SIZE     0<> 
                    IF 2drop 2drop  ERRNO EXIT THEN

    >= IF 2drop drop 0 0 EXIT THEN

    1 chars -rot
    ( c-addr size u1 fileid )
    dup >R fwrite

    \ check for error on stream
    R> ferror 0<> IF ERRNO ELSE 0 THEN

;

:core WRITE-LINE ( c-addr u fileid -- ior ) 
    -rot BOUNDS
    DO 
        dup I c@  swap
        fputc -1 = IF ERRNO UNLOOP EXIT THEN
    LOOP
    [ ?SYSTEM-LINUX ]   [IF]
        10 ( LF ) swap fputc -1 = IF ERRNO ELSE 0 THEN
    [THEN]

    [ ?SYSTEM-WINDOWS ] [IF]
        13 ( CR ) over fputc -1 = IF drop ERRNO ELSE 0 THEN
        10 ( LF ) swap fputc -1 = IF      ERRNO ELSE 0 THEN
    [THEN]
;


