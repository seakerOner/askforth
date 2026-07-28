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

    u8 depth           = vm->stack->index;
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    cell.val._8u = depth;

    askf_stack_push( &cell, vm->stack );
}

void askf_add_core_words( void ) {
    AskForthToken core_dic_name = {0};
    core_dic_name.base          = (ascii*)"core";
    core_dic_name.length        = 4;

    AskForthToken scratch_word_name = {0};
    scratch_word_name.base          = (ascii*)".";
    scratch_word_name.length        = 1;

    boolean added_dot = askf_dic_add_word_native( core_dic_name, askf_word_dot, scratch_word_name );

    // TODO: throw errors for words
    if ( !added_dot )
        askf_print( (ascii*)"Failed adding '.' word to 'Core' Dictionary\n", 44 );

    scratch_word_name.base            = (ascii*)"depth";
    scratch_word_name.length          = 5;

    boolean added_dot_stack_depth = 
        askf_dic_add_word_native( core_dic_name, askf_word_stack_depth, scratch_word_name );

    if ( !added_dot_stack_depth )
        askf_print( (ascii*)"Failed adding 'depth' word to 'Core' Dictionary\n", 49 );
}
