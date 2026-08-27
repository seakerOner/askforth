#if defined( TARGET_LINUX )
    #define _GNU_SOURCE

    #define RAW_RAM_START_ADDRESS           0x0
    #define ASKFORTH_BLOCKS_MAX             1024
    #define ASKFORTH_BLOCKS_SIZE            1024
#elif defined( TARGET_WINDOWS )
    #define RAW_RAM_START_ADDRESS           0x0
    #define ASKFORTH_BLOCKS_MAX             1024
    #define ASKFORTH_BLOCKS_SIZE            1024
#else
    #define RAW_RAM_START_ADDRESS           POISON
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
#include "./fallback_loop/fallback.h"

#define ASKFORTH_RAW_RAM_START_ADDRESS  NULL
#define ASKFORTH_INPUT_BUFFER_MAX_CHARS 1024

#define ASKFORTH_ERROR_TRACER_CAPACITY  64

int main( void ) {
    AskForthVm          vm                          = {0};
    AskForth_Ram        ram                         = {0};
    AskForth_Stack      stack                       = {0};
    AskForth_Stack      cf_stack                    = {0};
    AskForth_Stack      r_stack                     = {0};
    AskForthInputBuffer input_buffer                = {0};
    AskForthInputBuffer input_buffer_x              = {0};
    AskForthInputBuffer fallback_input_buffer       = {0};
    AskForth_CellSize   initial_cell_base_scale     = ASKF_BITS64;
    AskForthErrorTrace  tracer                      = {0};
    AskForthTokenizer   tokenizer                   = {0};
    AskForthTokenizer   tokenizer_x                 = {0};
    AskForthTokenizer   fallback_tokenizer          = {0};
    AskForthBlocks      blocks                      = {0};
    AskForthThreadedFramesStack tframe_stack        = {0};

    ascii scratch[ASKFORTH_INPUT_BUFFER_MAX_CHARS]  = {0};
    input_buffer.base                               = scratch;
    input_buffer.capacity                           = ASKFORTH_INPUT_BUFFER_MAX_CHARS;
    input_buffer.index                              = 0;

    ascii scratch_x[ASKFORTH_INPUT_BUFFER_MAX_CHARS]  = {0};
    input_buffer_x.base                               = scratch_x;
    input_buffer_x.capacity                           = ASKFORTH_INPUT_BUFFER_MAX_CHARS;
    input_buffer_x.index                              = 0;

    ascii scratch_fallback[ASKFORTH_INPUT_BUFFER_MAX_CHARS / 2]  = {0};
    fallback_input_buffer.base                        = scratch_fallback;
    fallback_input_buffer.capacity                    = ASKFORTH_INPUT_BUFFER_MAX_CHARS / 2;
    fallback_input_buffer.index                       = 0;


    u64 ram_size = 0;

    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        ram_size = MB( 16 );
    #else
        ram_size = MB( 4 );
    #endif

    if ( RAW_RAM_START_ADDRESS == POISON ) {
        // Something is not quite right..
    }

    if ( !askf_create_backend_blob( ram_size, ( void* )RAW_RAM_START_ADDRESS, &ram ) )  {
        askf_print( (ascii*)"Failed to allocate VM's RAM", 27 );
        return 1;
    }

    askf_start_stack( initial_cell_base_scale, &stack );
    askf_start_stack( initial_cell_base_scale, &cf_stack );
    askf_start_stack( initial_cell_base_scale, &r_stack );

    tframe_stack.index      = 0;
    tframe_stack.capacity   = ASKF_THREADEDFRAMES_STACK_CAPACITY;

    askf_start_error_tracer( &ram, &tracer, ASKFORTH_ERROR_TRACER_CAPACITY );

    vm.ram                  = &ram;
    vm.stack                = &stack;
    vm.cf_stack             = &cf_stack;
    vm.rstack               = &r_stack;
    vm.tframes_stack        = &tframe_stack;
    vm.input_buffer         = &input_buffer;
    vm.input_buffer_x       = &input_buffer_x;
    vm.fallback_input       = &fallback_input_buffer;
    vm.blocks               = &blocks;
    vm.outer_state          = ASKF_VM_OUTER_STATE_BLOCKING_INPUT;
    vm.interpret_state      = ASKF_INTERPRET;
    vm.num_base             = ASKF_DECIMAL;
    vm.error_tracer         = &tracer;
    vm.tokenizer            = &tokenizer;
    vm.tokenizer_x          = &tokenizer_x;
    vm.fallback_tokenizer   = &fallback_tokenizer;

    vm.parse_type        = ASKF_MAIN_PARSER;

    vm.lib              = ( void* )askf_create_library( &vm );

    askf_vm_to_global_state( &vm );

    askf_blocks_start( ASKFORTH_BLOCKS_MAX , ASKFORTH_BLOCKS_SIZE );

    askf_tokenizer_new( &tokenizer, ( ASKFORTH_INPUT_BUFFER_MAX_CHARS / 2 ));
    askf_tokenizer_new( &tokenizer_x, ( ASKFORTH_INPUT_BUFFER_MAX_CHARS / 2 ));
    askf_tokenizer_new( &fallback_tokenizer, ( ASKFORTH_INPUT_BUFFER_MAX_CHARS / 4 ));

    askf_add_core_words();

    askf_print( ( ascii* )"Welcome to the Agnostic Seaker's Forth :D", 41 );
    askf_print_char( (ascii)'\n' );

    while ( vm.outer_state != ASKF_VM_OUTER_STATE_SHUTDOWN_REQUEST ) {

        switch ( vm.outer_state ) {
            case ASKF_VM_OUTER_STATE_BLOCKING_INPUT:
                askf_print( (ascii*)">", 1 );
                askf_read_input_blocking( &vm );

                askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_EXECUTE );
                break;
            case ASKF_VM_OUTER_STATE_EXECUTE_CONTINUE:
                askf_exec( &vm, vm.parse_type );
                if ( vm.outer_state == ASKF_VM_OUTER_STATE_EXECUTE )
                    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_BLOCKING_INPUT );
                break;
            case ASKF_VM_OUTER_STATE_EXECUTE:
                askf_exec( &vm, ASKF_MAIN_PARSER );
                if ( vm.outer_state == ASKF_VM_OUTER_STATE_EXECUTE )
                    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_BLOCKING_INPUT );
                break;
            case ASKF_VM_OUTER_STATE_FAILED_CRITICAL:
            case ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL:
                askforth_fallbackloop_run( &vm );
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
