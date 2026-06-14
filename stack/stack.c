#include "./stack.h"

AskForth_Stack stack = {0};

u32 askf_start_stack( CellSize cell_scale ) {
    switch ( cell_scale ) {
        case ASKF_BITS8:
            stack.current_max_depth = ASKF_STACK_8BIT_SIZE;
             break;
        case ASKF_BITS16:
            stack.current_max_depth = ASKF_STACK_16BIT_SIZE;
             break;
        case ASKF_BITS32:
            stack.current_max_depth = ASKF_STACK_32BIT_SIZE;
             break;
        case ASKF_BITS64:
            stack.current_max_depth = ASKF_STACK_64BIT_SIZE;
             break;
        default:
             // TODO: ERROR HERE
             return 1;
    }

    stack.cell_scale = cell_scale;
    stack.index      = 0;

    FILL( &stack.cells.space_64, 0, sizeof( stack.cells ) );

    return 0;
}
