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

:core BIN ( fam1 -- fam2 )
;

:core CLOSE-FILE  ( fileid -- ior )
;

:core CREATE-FILE  ( c-addr u fam -- fileid ior )
;

:core DELETE-FILE  ( c-addr u -- ior )
;

:core FILE-POSITION ( fileid -- ud ior )
;

:core FILE-SIZE ( fileid -- ud ior )
;

:core FILE-STATUS ( c-addr u -- x ior )
;

:core FLUSH-FILE ( fileid -- ior )
;

:core OPEN-FILE ( c-addr u fam -- fileid ior )
;

:core R/O ( -- fam )
;

:core R/W ( -- fam )
;

:core READ-FILE ( c-addr u1 fileid -- u2 ior )
;

:core READ-LINE ( c-addr u1 fileid -- u2 flag ior ) 
;

:core RENAME-FILE ( c-addr1 u1 c-addr2 u2 -- ior ) 
;

:core REPOSITION-FILE ( ud fileid -- ior ) 
;

:core RESIZE-FILE ( ud fileid -- ior ) 
;

:core W/O ( -- fam )
;

:core WRITE-FILE ( c-addr u fileid -- ior ) 
;

:core WRITE-LINE ( c-addr u fileid -- ior ) 
;

