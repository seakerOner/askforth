#include "./stack.h"

u32 askf_start_stack( AskForth_CellSize cell_scale, AskForth_Stack* stack ) {
    switch ( cell_scale ) {
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
             // TODO: ERROR HERE
             return 1;
    }

    stack->cell_scale = cell_scale;
    stack->index      = 0;
    stack->is_signed  = FALSE;

    FILL( &stack->cells.space_64, 0, sizeof( stack->cells ) );

    return 0;
}

void askf_stack_push( AskForth_Cell* cell, AskForth_Stack* stack ) {
    if ( stack->index >= stack->current_max_depth ) {
        // TODO: throw error
        return;
    }
    boolean is_signed = stack->is_signed;

    switch ( stack->cell_scale ) {
        case ASKF_BITS64:
            if ( is_signed )
                stack->cells.space_64[stack->index] = cell->val._64s;
            else
                stack->cells.space_64[stack->index] = cell->val._64u;
            break;
        case ASKF_BITS32:
            if ( is_signed )
                stack->cells.space_32[stack->index] = cell->val._32s;
            else
                stack->cells.space_32[stack->index] = cell->val._32u;
            break;
        case ASKF_BITS16:
            if ( is_signed )
                stack->cells.space_16[stack->index] = cell->val._16s;
            else
                stack->cells.space_16[stack->index] = cell->val._16u;
            break;
        case ASKF_BITS8:
            if ( is_signed )
                stack->cells.space_8[stack->index]  = cell->val._8s;
            else
                stack->cells.space_8[stack->index]  = cell->val._8u;
            break;
        default:
            // TODO: fallback reset
            break;
    }

    stack->index++;
}

u32 askf_stack_pop( AskForth_Cell* out_cell , AskForth_Stack* stack ) {
    if ( stack->index >= stack->current_max_depth ) {
        return 0;
    }
    if ( out_cell == NULL ) {
        return 0;
    }

    out_cell->cell_scale = &stack->cell_scale;
    stack->index--;

    boolean is_signed = stack->is_signed;

    switch ( *out_cell->cell_scale ) {
        case ASKF_BITS64:
            if ( is_signed )
                out_cell->val._64s = stack->cells.space_64[stack->index];
            else
                out_cell->val._64u = stack->cells.space_64[stack->index];
            break;
        case ASKF_BITS32:
            if ( is_signed )
                out_cell->val._32s = stack->cells.space_32[stack->index];
            else
                out_cell->val._32u = stack->cells.space_32[stack->index];
            break;
        case ASKF_BITS16:
            if ( is_signed )
                out_cell->val._16s = stack->cells.space_16[stack->index];
            else
                out_cell->val._16u = stack->cells.space_16[stack->index];
            break;
        case ASKF_BITS8:
            if ( is_signed )
                out_cell->val._8s = stack->cells.space_8[stack->index];
            else 
                out_cell->val._8u = stack->cells.space_8[stack->index];
            break;
        default:
            break;
    }


    return 1;
}
