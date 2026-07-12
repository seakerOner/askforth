#include "forth_vm.h"

AskForthVm* global_vm = NULL;

void askf_vm_to_global_state( AskForthVm* vm ) {
    global_vm = vm;
}

void askf_exec( AskForthVm* vm ) {

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
