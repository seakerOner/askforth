#include "askforth_words.h"
#include "../library/library.h"

#include "../input/input.h"
#include "../stack/stack.h"

void askf_word_dot( void ) {
    AskForth_Cell cell = {0};
    u32 res = askf_stack_pop( &cell, askf_get_global_vm()->stack );

    if ( !res )
        return;

    // TODO: print number 
}

void askf_add_core_words( void ) {
    AskForthToken core_dic_name = {0};
    core_dic_name.base      = (ascii*)"core";
    core_dic_name.length    = 4;

    AskForthToken dot_word_name = {0};
    dot_word_name.base      = (ascii*)".";
    dot_word_name.length    = 1;

    askf_dic_add_word_native( core_dic_name, askf_word_dot, dot_word_name );
}
