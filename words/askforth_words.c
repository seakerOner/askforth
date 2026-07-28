#include "askforth_words.h"
#include "../library/library.h"

#include "../input/input.h"
#include "../stack/stack.h"

void askf_word_dot( void ) {
    AskForth_Cell cell = {0};
    u32 res = askf_stack_pop( &cell, askf_get_global_vm()->stack );

    // TODO: throw error stack underflow
    if ( !res ) {
        return;
    }

    askf_print_cell(&cell);
    askf_print( (ascii*)" ", 1 );
}

void askf_word_stack_depth( void ) {
    AskForthVm* vm = askf_get_global_vm();

    if ( vm == NULL ) {
        // TODO: throw error
        return;
    }

    u8 depth            = vm->stack->index;
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    cell.val._8u        = depth;

    askf_stack_push( &cell, vm->stack );
}

void askf_word_dot_stack ( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u8 bits = vm->stack->cell_scale;
    cell.val._8u = bits;
    askf_print_cell( &cell );
    askf_print( (ascii*)" BITS ", 6 );

    u8 depth            = vm->stack->index;
    cell.val._8u        = depth;

    askf_print( (ascii*)"<", 1 );
    askf_print_cell( &cell );
    askf_print( (ascii*)"> ", 2 );

    for ( u8 x = 0; x < depth; x++ ) {
        switch ( *cell.cell_scale ) {
            case ASKF_BITS64:
                cell.val._64u = vm->stack->cells.space_64[x];
                break;
            case ASKF_BITS32:
                cell.val._32u = vm->stack->cells.space_32[x];
                break;
            case ASKF_BITS16:
                cell.val._16u = vm->stack->cells.space_16[x];
                break;
            case ASKF_BITS8:
                cell.val._8u  = vm->stack->cells.space_8[x];
                break;
            default:
                break;
        }
            askf_print_cell( &cell );
            askf_print( (ascii*)" ", 1 );
    }
}

void askf_word_dup ( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &cell, vm->stack );
    askf_stack_push( &cell, vm->stack );
    askf_stack_push( &cell, vm->stack );
}

void askf_word_swap( void ) {
    AskForthVm* vm          = askf_get_global_vm();
    AskForth_Cell cell_ts   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_ss   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &cell_ts, vm->stack );
    askf_stack_pop( &cell_ss, vm->stack );

    askf_stack_push( &cell_ts, vm->stack );
    askf_stack_push( &cell_ss, vm->stack );
}

void askf_word_drop( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &cell, vm->stack );
}

void askf_word_over( void ) {
    AskForthVm* vm          = askf_get_global_vm();
    AskForth_Cell cell_ts   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_ss   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &cell_ts, vm->stack );
    askf_stack_pop( &cell_ss, vm->stack );

    askf_stack_push( &cell_ss, vm->stack );
    askf_stack_push( &cell_ts, vm->stack );
    askf_stack_push( &cell_ss, vm->stack );
}

void askf_word_negate( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &cell, vm->stack );
    cell.val._64s = 0 - cell.val._64u;
    askf_stack_push( &cell, vm->stack );
}

void _askf_print_failed_word( AskForthToken* tkn ) {
    askf_print( (ascii*)"Failed adding '", 15 );
    askf_print( tkn->base, tkn->length );
    askf_print( (ascii*)"' word to 'Core' Dictionary\n", 29 );
}

void askf_add_core_words( void ) {
    AskForthToken core_dic_name = {0};
    core_dic_name.base          = (ascii*)"core";
    core_dic_name.length        = 4;

    // TODO: throw errors for words
    
    // DOT 
    AskForthToken scratch_word_name = {0};
    scratch_word_name.base          = (ascii*)".";
    scratch_word_name.length        = 1;

    boolean added_dot = askf_dic_add_word_native( core_dic_name, askf_word_dot, scratch_word_name );

    if ( !added_dot )
        _askf_print_failed_word( &scratch_word_name );

    // DEPTH
    scratch_word_name.base            = (ascii*)"depth";
    scratch_word_name.length          = 5;

    boolean added_stack_depth = 
        askf_dic_add_word_native( core_dic_name, askf_word_stack_depth, scratch_word_name );

    if ( !added_stack_depth )
        _askf_print_failed_word( &scratch_word_name );

    scratch_word_name.base            = (ascii*)".s";
    scratch_word_name.length          = 2;

    // DOT STACK 
    boolean added_dot_stack = 
        askf_dic_add_word_native( core_dic_name, askf_word_dot_stack, scratch_word_name );

    if ( !added_dot_stack )
        _askf_print_failed_word( &scratch_word_name );

    // DUP
    scratch_word_name.base            = (ascii*)"dup";
    scratch_word_name.length          = 3;

    boolean added_dup = 
        askf_dic_add_word_native( core_dic_name, askf_word_dup, scratch_word_name );

    if ( !added_dup )
        _askf_print_failed_word( &scratch_word_name );

    // SWAP
    scratch_word_name.base            = (ascii*)"swap";
    scratch_word_name.length          = 4;

    boolean added_swap = 
        askf_dic_add_word_native( core_dic_name, askf_word_swap, scratch_word_name );

    if ( !added_swap )
        _askf_print_failed_word( &scratch_word_name );

    // DROP
    scratch_word_name.base            = (ascii*)"drop";
    scratch_word_name.length          = 4;

    boolean added_drop = 
        askf_dic_add_word_native( core_dic_name, askf_word_drop, scratch_word_name );

    if ( !added_drop )
        _askf_print_failed_word( &scratch_word_name );

    // OVER
    scratch_word_name.base            = (ascii*)"over";
    scratch_word_name.length          = 4;

    boolean added_over = 
        askf_dic_add_word_native( core_dic_name, askf_word_over, scratch_word_name );

    if ( !added_over )
        _askf_print_failed_word( &scratch_word_name );

    // negate
    scratch_word_name.base            = (ascii*)"negate";
    scratch_word_name.length          = 6;

    boolean added_negate = 
        askf_dic_add_word_native( core_dic_name, askf_word_negate, scratch_word_name );

    if ( !added_negate )
        _askf_print_failed_word( &scratch_word_name );

}
