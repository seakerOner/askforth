
#ifdef TARGET_LINUX
    #define _GNU_SOURCE

    #define RAW_RAM_START_ADDRESS 0x0
    #define ASKFORTH_BLOCKS_MAX             1024
    #define ASKFORTH_BLOCKS_SIZE            1024
#else
    #define RAW_RAM_START_ADDRESS POISON
    #define ASKFORTH_BLOCKS_MAX             64
    #define ASKFORTH_BLOCKS_SIZE            1024
#endif

#include "inttype.h"
#include "./memory/backend_blob.h"
#include "./memory/blocks.h"
#include "./stack/stack.h"
#include "./library/library.h"
#include "./input/input.h"
#include "./input/tokenizer.h"
#include "./vm/forth_vm.h"
#include "./errors/errors.h"
#include "./errors/error_thrower.h"
#include "./words/askforth_words.h"

#define ASKFORTH_RAW_RAM_START_ADDRESS  NULL
#define ASKFORTH_INPUT_BUFFER_MAX_CHARS 512

#define ASKFORTH_ERROR_TRACER_CAPACITY  64

int main( void ) {
    AskForthVm          vm                          = {0};
    AskForth_Ram        ram                         = {0};
    AskForth_Stack      stack                       = {0};
    AskForthInputBuffer input_buffer                = {0};
    AskForth_CellSize   initial_cell_base_scale     = ASKF_BITS64;
    AskForthErrorTrace  tracer                      = {0};
    AskForthTokenizer   tokenizer                   = {0};
    AskForthBlocks      blocks                      = {0};

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

    askf_start_error_tracer( &ram, &tracer, ASKFORTH_ERROR_TRACER_CAPACITY );


    vm.ram              = &ram;
    vm.stack            = &stack;
    vm.input_buffer     = &input_buffer;
    vm.blocks           = &blocks;
    vm.outer_state      = ASKF_VM_OUTER_STATE_BLOCKING_INPUT;
    vm.interpret_state  = ASKF_INTERPRET;
    vm.num_base         = ASKF_DECIMAL;
    vm.error_tracer     = &tracer;
    vm.tokenizer        = &tokenizer;

    vm.lib              = ( void* )askf_create_library( &vm );

    askf_vm_to_global_state( &vm );

    askf_blocks_start( ASKFORTH_BLOCKS_MAX , ASKFORTH_BLOCKS_SIZE );

    askf_tokenizer_new( &tokenizer, ( ASKFORTH_INPUT_BUFFER_MAX_CHARS / 2 ));

    askf_add_core_words();

    ascii* message = ( ascii* )"Welcome to the Agnostic Seaker's Forth :D \n";
    askf_print( message, sizeof( message ) );

    AskForthError* err = NULL;
    while ( vm.outer_state != ASKF_VM_OUTER_STATE_SHUTDOWN_REQUEST ) {

        switch ( vm.outer_state ) {
            case ASKF_VM_OUTER_STATE_BLOCKING_INPUT:
                askf_print( (ascii*)">", 1 );
                askf_read_input_blocking( &vm );

                askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_EXECUTE );
                break;
            case ASKF_VM_OUTER_STATE_EXECUTE:
                askf_exec( &vm );

                if ( vm.outer_state == ASKF_VM_OUTER_STATE_EXECUTE )
                    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_BLOCKING_INPUT );

                break;
            case ASKF_VM_OUTER_STATE_FAILED_CRITICAL:
            case ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL:
                // TODO: make user choose what to do
                err = askf_vm_get_most_recent_error();
                askf_print_error(err);

                askf_tokenizer_reset( vm.tokenizer );
                askf_reset_input_buffer( &vm );
                askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_BLOCKING_INPUT );
                break;
            case ASKF_VM_OUTER_STATE_SHUTDOWN_REQUEST:
            default:
                break;
        }
    };

    // TODO: shutdown protocol and memory conservation
    askf_blocks_close();

    return 0;
}
