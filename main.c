
#include "inttype.h"
#include "./memory/backend_blob.h"
#include "./stack/stack.h"
#include "./library/library.h"
#include "./input/input.h"
#include "./vm/forth_vm.h"

#define ASKFORTH_RAW_RAM_START_ADDRESS NULL
#define ASKFORTH_INPUT_BUFFER_MAX_CHARS 512

#ifdef TARGET_LINUX
    #define RAW_RAM_START_ADDRESS 0x0
#else
    #define RAW_RAM_START_ADDRESS POISON
#endif

int main( void ) {
    AskForthVm          vm                          = {0};
    AskForth_Ram        ram                         = {0};
    AskForth_Stack      stack                       = {0};
    AskForthInputBuffer input_buffer                = {0};
    AskForth_CellSize   initial_cell_base_scale     = ASKF_BITS64;

    ascii scratch[ASKFORTH_INPUT_BUFFER_MAX_CHARS]  = {0};
    input_buffer.base                               = scratch;
    input_buffer.capacity                           = ASKFORTH_INPUT_BUFFER_MAX_CHARS;
    input_buffer.index                              = 0;

    u64 ram_size = 0;

    #ifdef TARGET_LINUX 
        ram_size = MB( 16 );
    #else
        ram_size = MB( 4 );
    #endif

    if ( RAW_RAM_START_ADDRESS == POISON ) {
        // Something is not quite right..
    }

    askf_create_backend_blob( ram_size, ( void* )RAW_RAM_START_ADDRESS, &ram );
    askf_start_stack( initial_cell_base_scale, &stack );

    vm.ram          = &ram;
    vm.stack        = &stack;
    vm.input_buffer = &input_buffer;

    vm.lib          = ( void* )askf_create_library( &vm );

    while ( TRUE ) {
        askf_read_input_blocking( &vm );

        if ( vm.input_buffer->index > 0 ) 
            askf_exec( &vm );
    };

    return 0;
}
