#include "forth_vm.h"
#include "../input/input.h"
#include <stdio.h>

AskForthVm* global_vm = NULL;

void askf_vm_to_global_state( AskForthVm* vm ) {
    global_vm = vm;
}

AskForthVm* askf_get_global_vm( void ) {
    return global_vm;
}

static void _askf_parse_input_buffer( AskForthVm* forth_vm ) {
    AskForthInputBuffer* ib     = forth_vm->input_buffer;
    AskForthTokenizer* tknzr    = forth_vm->tokenizer;

    if (ib->index == 0)
        return;

    ascii*  base_token  = ib->base;
    u64     length      = 0;
    
    for ( u64 x = 0; x < ib->index; x++ ) {
        if ( ib->base[x] == ' ' || ib->base[x] == '\n' ) {
            if ( length > 0 ) {
                AskForthToken new_token = {0};
                new_token.base          = base_token;
                new_token.length        = length;

                askf_tokenizer_add( tknzr, new_token );

                length = 0;

            } 

            if ( x + 1 < ib->index )
                base_token  = &ib->base[x + 1];
        } else  {
            length++;
        }
    }

}

void askf_exec( AskForthVm* vm ) {
    // parse input
    _askf_parse_input_buffer( vm );

    // TODO: exec on words
    
    askf_tokenizer_reset(vm->tokenizer);
    askf_reset_input_buffer( vm );
    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_BLOCKING_INPUT );
}

void askf_vm_change_cell_scale( AskForth_CellSize new_cell_size ) {
    AskForth_Stack* stack = global_vm->stack;

    if ( new_cell_size == stack->cell_scale )
        return;

    switch ( new_cell_size ) {
        case ASKF_BITS8:
            stack->current_max_depth = ASKF_STACK_8BIT_SIZE;
            break;
        case ASKF_BITS16:
            stack->current_max_depth = ASKF_STACK_16BIT_SIZE;
            break;
        case ASKF_BITS32:
            stack->current_max_depth = ASKF_STACK_32BIT_SIZE;
            break;
        case ASKF_BITS64:
            stack->current_max_depth = ASKF_STACK_64BIT_SIZE;
            break;
        default:
            // TODO: throw error
            break;

    }

    stack->index = ( stack->index * stack->cell_scale + new_cell_size - 1 ) / new_cell_size;

    if ( stack->index > stack->current_max_depth )
        stack->index = stack->current_max_depth;

    stack->cell_scale = new_cell_size;
}

void askf_vm_change_outer_state( AskForthVmOuterState new_state ) {
    switch ( new_state ) {
        case ASKF_VM_OUTER_STATE_BLOCKING_INPUT:
        case ASKF_VM_OUTER_STATE_EXECUTE:
        case ASKF_VM_OUTER_STATE_FAILED_CRITICAL:
        case ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL:
        case ASKF_VM_OUTER_STATE_SHUTDOWN_REQUEST:
            global_vm->outer_state = new_state;
            break;
        default:
            break;
   
    }
}

void askf_vm_trace_error( AskForthError error ) {
    AskForthErrorTrace* tracer  = global_vm->error_tracer;

    u64 absolute_idx            = tracer->head % tracer->capacity;

    COPY( &error, &tracer->errors[absolute_idx], sizeof( AskForthError ) );
};
