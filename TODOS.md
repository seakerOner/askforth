# List of TODOS

## Comment word '\' 
    - comments copied over from a browser fail when the VM tokenizes the input buffer,
    making comments fail
    - Comments that need to perserve state across buffers need to be tested

## Implement custom input reader
    - This will allow to moveback the cursor, delete characters etc
    - It will also provide consistent behavior across platforms

## add number base (global state) for input parsing
    - The VM should have a num base global state for when trying to parse a token into a number

## add a fallback debug loop system
    - When a user encounters an error instead of simply failing and giving the last error we shoud:
        - add a read input loop where the user can do a list of options 
            ( continue execution, 
            restore execution,
            show all errors from error tracer,
            display stack,
            etc.. )
