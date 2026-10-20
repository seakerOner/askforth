#include "askforth_words.h"

#include "../library/library.h"

#include "../input/input.h"
#include "../stack/stack.h"
#include "../stack/input_stack.h"
#include "../memory/blocks.h"
#include "../optimizer/optimizer.h"
#include "../loader/loader.h"

#if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
    #include "../ffi/load_extern_library.h"
    #include <stdio.h>
    #include <stdlib.h>
#endif

#define _CONCAT(a, b ) a##b
#define CONCAT(a, b) _CONCAT(a, b)

AskForthVm* vm            = NULL;

// global preallocated cells for native words to use 
AskForth_Cell* global_c00 = NULL;
AskForth_Cell* global_c01 = NULL;
AskForth_Cell* global_c02 = NULL;
AskForth_Cell* global_c03 = NULL;
AskForthForeignFuncSig* global_ffi_sig = NULL;

static boolean _stack_invalid_for_addresses() {
    return ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t );
}

static boolean _strequal( ascii* str, ascii* to_compare, u64 len ) {
    for ( u64 x = 0; x < len; x++ )
        if ( str[x] != to_compare[x] )
            return FALSE;

    return TRUE;
}

static void _askf_word_failed( ascii* msg, u64 len ) {
    AskForthError err = {0};
    err.error = ASKF_ERROR_WORD_FAILED;
    err.zone  = ASKF_ERROR_ZONE_INNER;

    AskForthErrorMessage* opt_msg = 
    askf_alloc_new_opt_message( msg, len );
    err.opt_message = opt_msg;
    askf_throw_error( err );
}

#define WID(name) WID_##name


#define PROC_WORD( name, func ) \
    enum { WID_##name = __COUNTER__ };  \
    void func( void )                   \

PROC_WORD( ALIGNED, askf_word_aligned ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"ALIGNED -> Stack Empty", 22 );
        return;
    }

    #if defined( ARQBITS64 )
        global_c00->val._addr_t = ALIGN_UP( global_c00->val._addr_t, 8 );
    #elif defined( ARQBITS32 )
        global_c00->val._addr_t = ALIGN_UP( global_c00->val._addr_t, 4 );
    #elif defined( ARQBITS16 )
        global_c00->val._addr_t = ALIGN_UP( global_c00->val._addr_t, 2 );
    #elif defined( ARQBITS8 )
        global_c00->val._addr_t = ALIGN_UP( global_c00->val._addr_t, 1 );
    #endif 

    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( ALIGN , askf_word_align ) {
    askf_addr_t here = (askf_addr_t)&((u8*)vm->ram->start_ptr)[vm->ram->byte_index];

    #if defined( ARQBITS64 )
        here = ALIGN_UP( here, 8 );
    #elif defined( ARQBITS32 )
        here = ALIGN_UP( here, 4 );
    #elif defined( ARQBITS16 )
        here = ALIGN_UP( here, 2 );
    #elif defined( ARQBITS8 )
        here = ALIGN_UP( here, 1 );
    #endif 

    askf_addr_t diff = here - (askf_addr_t)&((u8*)vm->ram->start_ptr)[vm->ram->byte_index];
    vm->ram->byte_index += diff;
}

PROC_WORD( DOT, askf_word_dot ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );

    // TODO: throw error stack underflow
    if ( !res ) {
        _askf_word_failed( (ascii*)". -> Stack Empty", 16 );
        return;
    }

    askf_print_cell( global_c00 );
    askf_print( (ascii*)" ", 1 );
}


PROC_WORD( SOURCE, askf_word_input_source ) {
    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"SOURCE -> cell width must match architecture word width", 55 );
        return;
    }
    global_c00->val._addr_t = ( askf_addr_t )vm->istack->sources[vm->istack->index-1].base;
    global_c01->val._addr_t = ( askf_addr_t )vm->istack->sources[vm->istack->index-1].byte_cap;

    askf_stack_push( global_c00, vm->stack );
    askf_stack_push( global_c01, vm->stack );
}

PROC_WORD( SOURCEID, askf_word_input_source_id ) {
    global_c00->val._addr_t = ( askf_addr_t )vm->istack->sources[vm->istack->index-1].source_id;

    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( SOURCEIN, askf_word_input_source_in ) {
    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)">IN -> cell width must match architecture word width", 52 );
        return;
    }
    global_c00->val._addr_t = ( askf_addr_t )&vm->istack->sources[vm->istack->index-1].in;

    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( BLK, askf_word_input_source_blk ) {
    global_c00->val._addr_t = ( askf_addr_t )&vm->istack->sources[vm->istack->index-1].blk;

    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( DEPTH, askf_word_stack_depth ) {
    global_c00->val._8u = vm->stack->index;
    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( BASE, askf_word_base ) {
    global_c00->val._addr_t = ( askf_addr_t )&vm->num_base;
    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( DECIMAL, askf_word_decimal ) {
    vm->num_base = ASKF_DECIMAL;
}

PROC_WORD( HEXADECIMAL, askf_word_hexadecimal ) {
    vm->num_base = ASKF_HEXADECIMAL;
}

PROC_WORD( OCTAL, askf_word_octal ) {
    vm->num_base = ASKF_OCTAL;
}

PROC_WORD( DOT_STACK, askf_word_dot_stack ) {
    global_c00->val._64u = (u8)vm->stack->cell_scale;
    askf_print_cell( global_c00 );
    askf_print( (ascii*)" BITS ", 6 );

    global_c00->val._64u = vm->stack->index;

    if ( vm->stack->is_signed )
        askf_print( (ascii*)"S ", 2 );
    else
        askf_print( (ascii*)"U ", 2 );

    askf_print( (ascii*)"<", 1 );
    askf_print_cell( global_c00 );
    askf_print( (ascii*)"> ", 2 );


    for ( u8 x = 0; x < vm->stack->index; x++ ) {
        switch ( vm->stack->cell_scale ) {
            case ASKF_BITS64:
                global_c00->val._64u = vm->stack->cells.space_64[x];
                break;
            case ASKF_BITS32:
                global_c00->val._32u = vm->stack->cells.space_32[x];
                break;
            case ASKF_BITS16:
                global_c00->val._16u = vm->stack->cells.space_16[x];
                break;
            case ASKF_BITS8:
                global_c00->val._8u  = vm->stack->cells.space_8[x];
                break;
            default:
                break;
        }
            askf_print_cell( global_c00 );
            askf_print( (ascii*)" ", 1 );
    }
}

PROC_WORD( DUP, askf_word_dup ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );

    if ( !res ) {
        _askf_word_failed( ( ascii* )"dup -> Stack Empty" , 18);
        return;
    }

    askf_stack_push( global_c00, vm->stack );
    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( MAYBE_DUP, askf_word_maybe_dup ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );

    if ( !res ) {
        _askf_word_failed( ( ascii* )"?dup -> Stack Empty" , 19);
        return;
    }

    askf_stack_push( global_c00, vm->stack );

    if ( global_c00->val._64u != 0 )
        askf_stack_push( global_c00, vm->stack );

}

PROC_WORD( 2DUP, askf_word_2dup ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( ( ascii* )"2dup -> Expects ( a b - )" , 25 );
        return;
    }

    AskForth_Cell* a = global_c00;
    AskForth_Cell* b = global_c01;

    askf_stack_pop( b, vm->stack );
    askf_stack_pop( a, vm->stack );

    askf_stack_push( a, vm->stack );
    askf_stack_push( b, vm->stack );
    askf_stack_push( a, vm->stack );
    askf_stack_push( b, vm->stack );
}

PROC_WORD( SWAP, askf_word_swap ) {
    AskForth_Cell* cell_ts   = global_c00;
    AskForth_Cell* cell_ss   = global_c01;

    u32 res1 = askf_stack_pop( cell_ts, vm->stack );

    if ( !res1 ) {
        _askf_word_failed( ( ascii* )"swap -> Stack Empty" , 19);
    }

    u32 res2 = askf_stack_pop( cell_ss, vm->stack );

    if ( !res2 ) {
        _askf_word_failed( ( ascii* )"swap -> Stack misses 2nd value" , 30);
    }

    askf_stack_push( cell_ts, vm->stack );
    askf_stack_push( cell_ss, vm->stack );
}

PROC_WORD( 2SWAP, askf_word_2swap ) {
    if ( vm->stack->index < 4 ) {
        _askf_word_failed( ( ascii* )"2swap -> Expects ( a b c d - )" , 30 );
        return;
    }

    AskForth_Cell* cell_a   = global_c00;
    AskForth_Cell* cell_b   = global_c01;
    AskForth_Cell* cell_c   = global_c02;
    AskForth_Cell* cell_d   = global_c03;

    askf_stack_pop( cell_d, vm->stack );
    askf_stack_pop( cell_c, vm->stack );
    askf_stack_pop( cell_b, vm->stack );
    askf_stack_pop( cell_a, vm->stack );

    askf_stack_push( cell_c, vm->stack );
    askf_stack_push( cell_d, vm->stack );
    askf_stack_push( cell_a, vm->stack );
    askf_stack_push( cell_b, vm->stack );
}

PROC_WORD( ROT, askf_word_rot ) {
    if ( vm->stack->index < 3 ) {
        _askf_word_failed( ( ascii* )"rot -> Expects ( a b c - )" , 26 );
        return;
    }

    AskForth_Cell* a  = global_c00;
    AskForth_Cell* b  = global_c01;
    AskForth_Cell* c  = global_c02;

    askf_stack_pop( c, vm->stack );
    askf_stack_pop( b, vm->stack );
    askf_stack_pop( a, vm->stack );

    askf_stack_push( b, vm->stack );
    askf_stack_push( c, vm->stack );
    askf_stack_push( a, vm->stack );
}

PROC_WORD( NIP, askf_word_nip ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( ( ascii* )"nip -> Expects ( a b - )" , 24 );
        return;
    }

    AskForth_Cell* a  = global_c00;
    AskForth_Cell* b  = global_c01;

    askf_stack_pop( b, vm->stack );
    volatile u32 dummy = askf_stack_pop( a, vm->stack );
    UNUSED(dummy);

    askf_stack_push( b, vm->stack );
}

PROC_WORD( TUCK, askf_word_tuck ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( ( ascii* )"tuck -> Expects ( a b - )" , 25 );
        return;
    }

    AskForth_Cell* a  = global_c00;
    AskForth_Cell* b  = global_c01;

    askf_stack_pop( b, vm->stack );
    askf_stack_pop( a, vm->stack );

    askf_stack_push( b, vm->stack );
    askf_stack_push( a, vm->stack );
    askf_stack_push( b, vm->stack );
}

PROC_WORD( DROP, askf_word_drop ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"drop -> Empty Stack", 19 );
    }
}

PROC_WORD( 2DROP, askf_word_2drop ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"2drop -> Expects ( a b - )", 26 );
        return;
    }

    volatile u32 dummy;

    dummy = askf_stack_pop( global_c00, vm->stack );
    dummy = askf_stack_pop( global_c00, vm->stack );
    UNUSED(dummy);
}

PROC_WORD( OVER, askf_word_over ) {
    AskForth_Cell* cell_ts   = global_c00;
    AskForth_Cell* cell_ss   = global_c01;

    u32 res1 = askf_stack_pop( cell_ts, vm->stack );

    if ( !res1 ) {
        _askf_word_failed( (ascii*)"over -> Empty Stack", 19 );
    }

    u32 res2 = askf_stack_pop( cell_ss, vm->stack );

    if ( !res2 ) {
        _askf_word_failed( (ascii*)"over -> 2nd value expected on stack", 35 );
    }

    askf_stack_push( cell_ss, vm->stack );
    askf_stack_push( cell_ts, vm->stack );
    askf_stack_push( cell_ss, vm->stack );
}

PROC_WORD( 2OVER, askf_word_2over ) {
    if ( vm->stack->index < 4 ) {
        _askf_word_failed( (ascii*)"2over -> Expects ( a b c d - )", 30 );
        return;
    }
    
    AskForth_Cell* cell_a   = global_c00;
    AskForth_Cell* cell_b   = global_c01;
    AskForth_Cell* cell_c   = global_c02;
    AskForth_Cell* cell_d   = global_c03;

    askf_stack_pop( cell_a, vm->stack );
    askf_stack_pop( cell_b, vm->stack );
    askf_stack_pop( cell_c, vm->stack );
    askf_stack_pop( cell_d, vm->stack );


    askf_stack_push( cell_d, vm->stack );
    askf_stack_push( cell_c, vm->stack );
    askf_stack_push( cell_b, vm->stack );
    askf_stack_push( cell_a, vm->stack );
    askf_stack_push( cell_d, vm->stack );
    askf_stack_push( cell_c, vm->stack );
}

PROC_WORD( NEGATE, askf_word_negate ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );
    if ( !res ) {
        _askf_word_failed( (ascii*)"negate -> Empty Stack", 21 );
        return;
    }

    global_c00->val._addr_t = 0 - global_c00->val._addr_t;
    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( LIB, askf_word_lib ) {
    AskForth_Library* lib       = (AskForth_Library*)vm->lib;

    AskForth_Dictionary* base   = lib->dictionaries_base;
    askf_print( (ascii*)"Dictionaries: ", 14 );

    while ( base ) {
        askf_print( base->name, base->name_len );
        askf_print( (ascii*)", ", 2 );

        base = base->next;
    }
}

PROC_WORD( PARSE_NAME, askf_word_parse_name ) {
    AskForth_InputSource* source = askf_istack_peek( vm->istack );

    if ( ( vm->stack->cell_scale / 8 ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
                (ascii *)"PARSE-NAME -> cell width must match architecture word width", 59 );
        return;
    }

    AskForthToken token = askf_next_token( source );

    AskForth_Cell* cell      = global_c00;

    cell->val._addr_t = (askf_addr_t)token.base;
    askf_stack_push( cell, vm->stack );
    cell->val._addr_t = (askf_addr_t)token.length;
    askf_stack_push( cell, vm->stack );
}

PROC_WORD( FROM, askf_word_from ) {
    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"[FROM] -> cell width must match architecture word width", 55 );
        return;
    }

    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"[FROM] -> Expected dictionary name", 34 );
        return;
    }

    AskForth_Cell* len   = global_c00;
    AskForth_Cell* name  = global_c01;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( name, vm->stack );

    AskForthToken tkn;
    tkn.base   = ( ascii* )name->val._addr_t;
    tkn.length = len->val._addr_t;
    
    AskForth_Dictionary* dic = askf_library_find_dic( vm, &tkn );

    global_c03->val._addr_t = ( askf_addr_t )dic;
    askf_stack_push( global_c03, vm->stack );
}

PROC_WORD( FIND, askf_word_find ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"[FIND] -> Expects dictionary address", 36 );
        return;
    }

    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"[FIND] -> cell width must match architecture word width", 55 );
        return;
    }

    AskForth_Cell* dic_cell = global_c00;
    askf_stack_pop( dic_cell, vm->stack );

    AskForth_Dictionary* dic = (AskForth_Dictionary*)dic_cell->val._addr_t;

    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"[FIND] -> Expected word name", 28 );
        return;
    }

    AskForth_Cell* len   = global_c01;
    AskForth_Cell* name  = global_c02;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( name, vm->stack );

    AskForthToken tkn;
    tkn.base   = ( ascii* )name->val._addr_t;
    tkn.length = len->val._addr_t;

    // find word from dic

    global_c03->val._addr_t = ( askf_addr_t )askf_library_find_word_from_dic( dic, &tkn );

    askf_stack_push( global_c03, vm->stack );
}

PROC_WORD( SFIND, askf_word_sfind ) {
    if ( vm->stack->index < 3 ) {
        _askf_word_failed( (ascii*)"FIND -> ( dict_xt addr u -- ) ", 30 );
        return;
    }

    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"FIND -> cell width must match architecture word width", 53 );
        return;
    }

    AskForth_Cell* dic_cell = global_c00;
    AskForth_Cell* len      = global_c01;
    AskForth_Cell* name     = global_c02;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( name, vm->stack );
    askf_stack_pop( dic_cell, vm->stack );

    AskForth_Dictionary* dic = (AskForth_Dictionary*)dic_cell->val._addr_t;

    AskForthToken tkn;
    tkn.base   = ( ascii* )name->val._addr_t;
    tkn.length = len->val._addr_t;

    // find word from dic

    global_c03->val._addr_t = ( askf_addr_t )askf_library_find_word_from_dic( dic, &tkn );

    askf_stack_push( global_c03, vm->stack );
}

PROC_WORD( FORGET, askf_word_forget ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"[FORGET] -> Expects dictionary address", 38 );
        return;
    }

    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"[FORGET] -> cell width must match architecture word width", 57 );
        return;
    }

    AskForth_Cell* dic_cell = global_c00;
    askf_stack_pop( dic_cell, vm->stack );

    AskForth_Dictionary* dic = (AskForth_Dictionary*)dic_cell->val._addr_t;

    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"[FORGET] -> Expected word name", 30 );
        return;
    }

    AskForth_Cell* len   = global_c01;
    AskForth_Cell* name  = global_c02;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( name, vm->stack );

    AskForthToken tkn;
    tkn.base   = ( ascii* )name->val._addr_t;
    tkn.length = len->val._addr_t;

    AskForth_Word* word = askf_library_find_word_from_dic( dic, &tkn );

    // clean main library

    if ( word->prev && word->next ) {
        word->prev->next = word->next;
        word->next->prev = word->prev;
    } else if ( word->prev && !word->next )  {
        word->prev->next = NULL;
        dic->recent_word = word->prev;
    } else if ( !word->prev && word->next ) {
        word->next->prev = NULL;
        dic->words_base  = word->next;
    } else if ( !word->prev && !word->next ) {
        if ( (askf_addr_t)dic->recent_word == (askf_addr_t)word ) { 
            dic->recent_word = NULL;
        }
        if ( (askf_addr_t)dic->words_base == (askf_addr_t)word ) {  
            dic->words_base  = NULL;
        }
    }


    // add word to shadow library
    AskForth_ShadowLibrary* slib = (AskForth_ShadowLibrary*)vm->shadow_lib;

    if ( slib->word_base == NULL ) {
        slib->word_base = word;
        slib->word_recent = slib->word_base;
        slib->word_base->prev = NULL;
        slib->word_base->next = NULL;
    } else {
        AskForth_Word* old_shadow_word = slib->word_recent;

        old_shadow_word->next = word;
        slib->word_recent = word;
        slib->word_recent->prev = old_shadow_word;
        slib->word_recent->next = NULL;
    }
}

PROC_WORD( XTFORGET, askf_word_xtforget ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"FORGET -> Expects ( word_xt -- )", 32 );
        return;
    }

    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"FORGET -> cell width must match architecture word width", 55 );
        return;
    }

    AskForth_Cell* word_cell = global_c00;
    askf_stack_pop( word_cell, vm->stack );

    AskForth_Word* word = (AskForth_Word*)word_cell->val._addr_t;

    // clean main library
    AskForth_Dictionary* dic = word->dict_origin;
    
    if ( word->prev && word->next ) {
        word->prev->next = word->next;
        word->next->prev = word->prev;
    } else if ( word->prev && !word->next )  {
        word->prev->next = NULL;
        dic->recent_word = word->prev;
    } else if ( !word->prev && word->next ) {
        word->next->prev = NULL;
        dic->words_base  = word->next;
    } else if ( !word->prev && !word->next ) {
        if ( (askf_addr_t)dic->recent_word == (askf_addr_t)word ) { 
            dic->recent_word = NULL;
        }
        if ( (askf_addr_t)dic->words_base == (askf_addr_t)word ) {  
            dic->words_base  = NULL;
        }
    }

     // add word to shadow library
    AskForth_ShadowLibrary* slib = (AskForth_ShadowLibrary*)vm->shadow_lib;

    if ( slib->word_base == NULL ) {
        slib->word_base = word;
        slib->word_recent = slib->word_base;
        slib->word_base->prev = NULL;
        slib->word_base->next = NULL;
    } else {
        AskForth_Word* old_shadow_word = slib->word_recent;

        old_shadow_word->next = word;
        slib->word_recent = word;
        slib->word_recent->prev = old_shadow_word;
        slib->word_recent->next = NULL;
    }
}

PROC_WORD( WORDS, askf_word_words ) {
    askf_word_parse_name();

    if ( vm->outer_state != ASKF_VM_OUTER_STATE_EXECUTE ) {
        return;
    }

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"words: -> Expects ( <dict_name> - )", 35 );
        return;
    }

    AskForth_Cell* len    = global_c00;
    AskForth_Cell* addr   = global_c01;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( addr, vm->stack );
    
    AskForthToken tkn           = {0};
    tkn.base                    = (ascii*)addr->val._64u;
    tkn.length                  = len->val._64u;
    AskForth_Dictionary* dic    = askf_library_find_dic( vm, &tkn );

    if ( !dic ) {
        _askf_word_failed( (ascii*)"words: -> dictionary not found" , 30 );
        return;
    }

    AskForth_Word* base = dic->words_base;

    #if defined( TARGET_LINUX )
        askf_print_char('\n');
    #endif
    askf_print( dic->name, dic->name_len );
    askf_print( (ascii*)" dictionary words: \n", 20 );

    if ( !base ) {
        askf_print( (ascii*)"words: -> empty dictionary " , 27 );
        return;
    }

    while ( base ) {
        askf_print( base->name, base->name_len );
        askf_print( (ascii*)" ", 1 );
        base = base->next;
    }
}

PROC_WORD( ADD, askf_word_add ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"+ -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    top_stack->val._64u = below_top__stack->val._64u + top_stack->val._64u;

    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( MINUS, askf_word_minus ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"- -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    top_stack->val._64u = below_top__stack->val._64u - top_stack->val._64u;

    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( MULTIPLY, askf_word_multiply ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"* -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    top_stack->val._64u = below_top__stack->val._64u * top_stack->val._64u;

    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( SLASHMOD, askf_word_slashmod ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"/mod -> Expects 2 values on stack" , 33 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    AskForth_Cell* below_top__stack = global_c01;
    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    u64 slash   = below_top__stack->val._64u / top_stack->val._64u;
    u64 mod     = below_top__stack->val._64u % top_stack->val._64u;

    top_stack->val._64u = slash;
    askf_stack_push( top_stack, vm->stack );
    top_stack->val._64u = mod;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( EQUALS, askf_word_equals ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"= -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;
    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    u64 val = below_top__stack->val._64u == top_stack->val._64u ? -1 : 0;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( EQUALS_ZERO, askf_word_equals_zero ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0= -> Expects 1 values on stack" , 31 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    askf_stack_pop( top_stack, vm->stack );

    u64 val = 0 == top_stack->val._64u ? -1 : 0;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( LESS_THAN, askf_word_less_than ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"< -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;
    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    u64 val = 0;
    if ( vm->stack->is_signed ) 
        val = below_top__stack->val._64s < top_stack->val._64s ? -1 : 0;
    else
        val = below_top__stack->val._64u < top_stack->val._64u ? -1 : 0;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( LESS_THAN_OR_EQUAL, askf_word_less_than_or_equal ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"< -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    u64 val = 0;
    if ( vm->stack->is_signed ) 
        val = below_top__stack->val._64s <= top_stack->val._64s ? -1 : 0;
    else
        val = below_top__stack->val._64u <= top_stack->val._64u ? -1 : 0;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( LESS_THAN_ZERO, askf_word_less_than_zero ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0< -> Expects 1 values on stack" , 31 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    askf_stack_pop( top_stack, vm->stack );

    u64 val = 0;
    if ( vm->stack->is_signed )
        val = 0 < top_stack->val._64s ? -1 : 0;
    else
        val = 0 < top_stack->val._64u ? -1 : 0;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( MORE_THAN, askf_word_more_than ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"> -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    u64 val = 0;
    
    if ( vm->stack->is_signed )
        val = below_top__stack->val._64s > top_stack->val._64s ? -1 : 0;
    else
        val = below_top__stack->val._64u > top_stack->val._64u ? -1 : 0;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( MORE_THAN_OR_EQUAL, askf_word_more_than_or_equal ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"> -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    u64 val = 0;
    
    if ( vm->stack->is_signed )
        val = below_top__stack->val._64s >= top_stack->val._64s ? -1 : 0;
    else
        val = below_top__stack->val._64u >= top_stack->val._64u ? -1 : 0;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( MORE_THAN_ZERO, askf_word_more_than_zero ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0> -> Expects 1 values on stack" , 31 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    askf_stack_pop( top_stack, vm->stack );

    u64 val = 0;

    if ( vm->stack->is_signed )
         val = 0 > top_stack->val._64s ? -1 : 0;
    else
         val = 0 > top_stack->val._64u ? -1 : 0;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( NOT_EQUAL, askf_word_not_equal ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"<> -> Expects 2 values on stack" , 31 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    u64 val = below_top__stack->val._64u != top_stack->val._64u ? -1 : 0;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( NOT_EQUAL_ZERO, askf_word_not_equal_zero ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0<> -> Expects 1 values on stack" , 32 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    askf_stack_pop( global_c00, vm->stack );

    u64 val = 0 != top_stack->val._64u ? -1 : 0;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( SIGNED, askf_word_make_stack_signed ) {
    vm->stack->is_signed = TRUE;
}

PROC_WORD( UNSIGNED, askf_word_make_stack_unsigned ) {
    vm->stack->is_signed = FALSE;
}

PROC_WORD( BITS, askf_word_bits ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)" BITS -> Empty Stack", 20 );
        return;
    }

    AskForth_CellSize new_cell_size = {0};

    switch ( global_c00->val._64u ) {
        case ASKF_BITS8:
        case ASKF_BITS16:
        case ASKF_BITS32:
        case ASKF_BITS64:
            new_cell_size = global_c00->val._64u;
            askf_vm_change_cell_scale( new_cell_size );
            break;
        default:
            goto askf_bits_err_invalid_bits;
            break;
    }

    return;

    askf_bits_err_invalid_bits: 
        _askf_word_failed( (ascii*)"BITS -> Invalid BITS size", 25 );
}

PROC_WORD( WHATBITS, askf_word_whatbits ) {
    global_c00->val._64u = (u64)vm->stack->cell_scale;
    
    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( TYPE, askf_word_type ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"TYPE -> Expects ( addr len - )", 30);
        return;
    }

    AskForth_Cell* len   = global_c00;
    AskForth_Cell* addr  = global_c01;
    
    if ( ( vm->stack->cell_scale / 8  ) 
            != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
                (ascii *)"TYPE -> cell width must match architecture word width", 53 );
        return;
    }

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( addr, vm->stack );

    askf_print( ( ascii* )addr->val._64u, len->val._32u );
    askf_print( ( ascii* )" ", 1 );
}

PROC_WORD( STORE, askf_word_store ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"! -> Expects ( val addr - )", 27);
        return;
    }
    AskForth_Cell* val   = global_c00;
    AskForth_Cell* addr  = global_c01;

    if ( ( vm->stack->cell_scale / 8  ) 
            != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
                (ascii *)"! -> cell width must match architecture word width", 50 );
        return;
    }

    askf_stack_pop( addr, vm->stack );
    askf_stack_pop( val, vm->stack );

    askf_addr_t* ptr = ( askf_addr_t* )addr->val._addr_t;
    *ptr = val->val._addr_t;
}

PROC_WORD( BYTE_STORE, askf_word_byte_store ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"! -> Expects ( val addr - )", 27);
        return;
    }
    AskForth_Cell* val   = global_c00;
    AskForth_Cell* addr  = global_c01;

    if ( ( vm->stack->cell_scale / 8  ) 
            != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
                (ascii *)"! -> cell width must match architecture word width", 50 );
        return;
    }

    askf_stack_pop( addr, vm->stack );
    askf_stack_pop( val, vm->stack );

    u8* ptr = ( u8* )addr->val._addr_t;
    *ptr = val->val._8u;
}

PROC_WORD( LOAD, askf_word_load_ptr ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"@ -> Expects ( addr - )", 23 );
        return;
    }
    AskForth_Cell* addr  = global_c00;

    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"@ -> cell width must match architecture word width", 50 );
        return;
    }       

    askf_stack_pop( addr, vm->stack );

    askf_addr_t* ptr = ( askf_addr_t* )addr->val._addr_t;

    addr->val._addr_t = *ptr;

    askf_stack_push( addr, vm->stack );
}

PROC_WORD( BYTE_LOAD, askf_word_load_byte_ptr ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"c@ -> Expects ( addr - )", 24);
        return;
    }
    AskForth_Cell* addr  = global_c00;

    askf_stack_pop( addr, vm->stack );

    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"c@ -> cell width must match architecture word width", 51 );
        return;
    }

    u8* ptr = ( u8* )addr->val._addr_t;

    addr->val._addr_t = 0;
    addr->val._8u = *ptr;

    askf_stack_push( addr, vm->stack );
}

PROC_WORD( HERE, askf_word_here ) {
    AskForth_Cell* addr  = global_c00;

    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"HERE -> cell width must match architecture word width", 53 );
        return;
    }

    addr->val._addr_t = ( askf_addr_t )( (( u8* )vm->ram->start_ptr ) + vm->ram->byte_index );

    askf_stack_push( addr, vm->stack );
}

PROC_WORD( ALLOT, askf_word_allot ) {
    AskForth_Cell* val  = global_c00;

    u32 res = askf_stack_pop( val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"ALLOT -> Expects ( bytes - )", 28 );
        return;
    }


    if ( vm->interpret_state == ASKF_COMPILE && 
            ((AskForth_Library*)vm->lib)->curr_compiling.here ) {
        askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_skippable );
        vm->ram->byte_index += val->val._64u;
        askf_compile_threaded_memory( val->val._64u );
    } else {
        vm->ram->byte_index += val->val._64u;
    }
}

PROC_WORD( UNUSED, askf_word_unused ) {
    AskForth_Cell* val  = global_c00;

    val->val._addr_t = vm->ram->length - vm->ram->byte_index;

    askf_stack_push( val, vm->stack );
}

PROC_WORD( CELLS, askf_word_cells ) {
    AskForth_Cell* val  = global_c00;

    u32 res = askf_stack_pop( val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"cells -> Expects ( n - )", 24 );
        return;
    }

    val->val._64u *= sizeof( askf_addr_t );

    askf_stack_push( val, vm->stack );
}

PROC_WORD( CELLS_ADD, askf_word_cell_add ) {
    AskForth_Cell* val  = global_c00;

    u32 res = askf_stack_pop( val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"cell+ -> Expects ( n - )", 24 );
        return;
    }

    val->val._64u += sizeof( askf_addr_t );

    askf_stack_push( val, vm->stack );
}

PROC_WORD( CHARS, askf_word_chars ) {
    AskForth_Cell* val  = global_c00;

    u32 res = askf_stack_pop( val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"chars -> Expects ( n - )", 24 );
        return;
    }

    val->val._64u *= sizeof( u8 );

    askf_stack_push( val, vm->stack );
}

PROC_WORD( CHARS_ADD, askf_word_char_add ) {
    AskForth_Cell* val  = global_c00;

    u32 res = askf_stack_pop( val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"char+ -> Expects ( n - )", 24 );
        return;
    }

    val->val._64u += sizeof( u8 );

    askf_stack_push( val, vm->stack );
}

PROC_WORD( PRINT_STRING, askf_word_print_string ) {
    AskForth_InputSource* source = askf_istack_peek(vm->istack);
    AskForthToken token = {0};

    ascii*  string_base     = &source->base[source->in];
    askf_addr_t len         = 0;
    boolean got_terminator  = FALSE;

    while ( TRUE ) {
        token = askf_next_token( source );
        if ( token.length == 0 && token.base == NULL ) 
            break;

        if ( token.base[token.length - 1] == '"' ) {
            got_terminator = TRUE;
            len = (askf_addr_t)( token.base + token.length ) - (askf_addr_t)string_base;
            token.base[token.length - 1] = '\0';
            len -= 1;
            break;
        } 
    }

    if ( !got_terminator ) {
        _askf_word_failed( (ascii*)".\" -> Terminator not found on input buffer", 42 );
        return;
    }

    switch ( vm->interpret_state ) {
        case ASKF_INTERPRET: {
            AskForth_Cell* cell = global_c00;
            cell->val._addr_t = (askf_addr_t)string_base;
            askf_stack_push( cell, vm->stack );
            cell->val._addr_t = len;
            askf_stack_push( cell, vm->stack );

            if ( _stack_invalid_for_addresses() ) {
                _askf_word_failed( 
                    (ascii *)".\" -> cell width must match architecture word width", 51 );
                return;
            }

            askf_word_type();
            } break;
        case ASKF_COMPILE: {

            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_skippable );


            ascii* ptr = (ascii*)askf_alloc( len+1 );
            COPY( string_base, ptr, len+1 );

            askf_addr_t here = (askf_addr_t)&((u8*)vm->ram->start_ptr)[vm->ram->byte_index];
            askf_word_align();
            askf_addr_t c_here = (askf_addr_t)&((u8*)vm->ram->start_ptr)[vm->ram->byte_index];
            askf_addr_t alignment = c_here - here;

            askf_compile_threaded_memory( len+1+alignment );

            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_literal );
            askf_compile_threaded_memory( (u64)ptr );

            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_literal );
            askf_compile_threaded_memory( (u64)len );

            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_native  );
            askf_compile_threaded_memory( (u64)askf_word_type );
            AskForthToken type_token = {0};
            type_token.base     = (ascii*)"TYPE";
            type_token.length   = 4;
            type_token.line_end = FALSE;
            AskForth_Word* type_word = askf_library_find_word( vm, &type_token );

            if ( type_word )
                askf_compile_threaded_memory( (u64)type_word );
            else 
                askf_compile_threaded_memory( (u64)0 );
        }
            break;
        default:
            break;
    }
}

PROC_WORD( ERROR_WITH_STRING, askf_word_error_with_string ) {
    AskForth_InputSource* source = askf_istack_peek(vm->istack);
    AskForthToken token = {0};
    
    ascii*  string_base     = &source->base[source->in];
    askf_addr_t len         = 0;
    boolean got_terminator  = FALSE;

    while ( TRUE ) {
        token = askf_next_token( source );
        if ( token.length == 0 && token.base == NULL ) 
            break;

        if ( token.base[token.length - 1] == '"' ) {
            got_terminator = TRUE;
            len = (askf_addr_t)( token.base + token.length ) - (askf_addr_t)string_base;
            token.base[token.length - 1] = '\0';
            len -= 1;
            break;
        } 
    }

    if ( !got_terminator ) {
        _askf_word_failed( (ascii*)"error\" -> Terminator not found on input buffer", 46 );
        return;
    }

    if ( string_base[len-1] == '"' )
        string_base[len-1] = ' ';

    switch ( vm->interpret_state ) {
        case ASKF_INTERPRET: {
            ascii tmp = string_base[len];
            string_base[len] = '\0';
            _askf_word_failed( string_base, len );
            string_base[len] = tmp;
        } break;
        case ASKF_COMPILE: {

            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_skippable );

            ascii* ptr = (ascii*)askf_alloc( len+1 );
            COPY( string_base, ptr, len+1 );
            ptr[len] = '\0';

            askf_addr_t here = (askf_addr_t)&((u8*)vm->ram->start_ptr)[vm->ram->byte_index];
            askf_word_align();
            askf_addr_t c_here = (askf_addr_t)&((u8*)vm->ram->start_ptr)[vm->ram->byte_index];
            askf_addr_t alignment = c_here - here;

            askf_compile_threaded_memory( len+1+alignment );

            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_dispatch_error );

            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_literal );
            askf_compile_threaded_memory( (u64)ptr );

            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_literal );
            askf_compile_threaded_memory( (u64)len );
        }
            break;
        default:
            break;
    }
}

PROC_WORD( STORE_STRING, askf_word_store_string ) {
    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
            (ascii *)".\" -> cell width must match architecture word width", 51 );
        return;
    }

    AskForth_InputSource* source = askf_istack_peek(vm->istack);
    AskForthToken token = {0};
    
    ascii*  string_base     = &source->base[source->in];
    askf_addr_t len         = 0;
    boolean got_terminator  = FALSE;

    while ( TRUE ) {
        token = askf_next_token( source );
        if ( token.length == 0 && token.base == NULL ) 
            break;

        if ( token.base[token.length - 1] == '"' ) {
            got_terminator = TRUE;
            len = (askf_addr_t)( token.base + token.length ) - (askf_addr_t)string_base;
            token.base[token.length - 1] = '\0';
            len -= 1;
            break;
        } 
    }

    if ( !got_terminator ) {
        _askf_word_failed( (ascii*)"s\" -> Terminator not found on input buffer", 42 );
        return;
    }

    if ( vm->interpret_state == ASKF_COMPILE )
        askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_skippable );

    if ( vm->interpret_state == ASKF_INTERPRET )
        askf_word_align();

    ascii* new_base = askf_alloc( sizeof(ascii) * len+1 );

    if ( vm->interpret_state == ASKF_COMPILE ) {
        askf_addr_t here = (askf_addr_t)&((u8*)vm->ram->start_ptr)[vm->ram->byte_index];
        askf_word_align();
        askf_addr_t c_here = (askf_addr_t)&((u8*)vm->ram->start_ptr)[vm->ram->byte_index];
        askf_addr_t alignment = c_here - here;

        askf_compile_threaded_memory( len+1+alignment );
    }

    COPY( string_base, new_base, len );
    new_base[len+1] = '\0';

    if ( vm->interpret_state == ASKF_COMPILE ) {
        askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_literal_relocatable );
        askf_compile_threaded_memory( (u64)new_base );
        askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_literal );
        askf_compile_threaded_memory( (u64)len );
    } else {
        AskForth_Cell* cell_addr = global_c00;
        AskForth_Cell* cell_len  = global_c01;

        cell_addr->val._addr_t = (askf_addr_t) new_base;
        cell_len->val._addr_t  = len;

        askf_stack_push( cell_addr, vm->stack );
        askf_stack_push( cell_len, vm->stack );
    }
}

PROC_WORD( COMMENT_PAREN, askf_word_comment_parenteshis ) {
    AskForth_InputSource* source = askf_istack_peek(vm->istack);
    AskForthToken token = {0};

    while ( TRUE ) {
        token = askf_next_token( source );
        if ( token.length == 0 && token.base == NULL ) {
            if ( source->blk > 0 ) {
                _askf_word_failed( (ascii*)"( -> Could not find ')' inside the current block", 48 );
                return;
            }
            askf_word_refill();
            continue;
        }
        if ( token.base[token.length - 1] == ')' ) {
            return;
        } 
    }
}

PROC_WORD( COMMENT_SLASH, askf_word_comment_slash ) {
    AskForth_InputSource* source = askf_istack_peek(vm->istack);
    AskForthToken token = {0};

    while ( TRUE ) {
        token = askf_next_token( source );
        if ( token.length == 0 && token.base == NULL ) {
            return;
        }
        if ( token.line_end == TRUE )  {
            return;
        }
    }
}

PROC_WORD( CR, askf_word_cr ) {
    askf_print( (ascii*)"\n", 1 );
}

PROC_WORD( EMIT, askf_word_emit ) {
    AskForth_Cell* ascii_char = global_c00;
    u32 res = askf_stack_pop( ascii_char, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"EMIT -> Expects ( ascii_char - )", 32 );
        return;
    }

    askf_print( &ascii_char->val._8u, 1 );
}

PROC_WORD( LSHIFT, askf_word_lshift ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"lshift -> Expects 2 values on stack" , 35 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;
    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    top_stack->val._64u = below_top__stack->val._64u << top_stack->val._64u;

    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( RSHIFT, askf_word_rshift ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"rshift -> Expects 2 values on stack" , 35 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;
    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    top_stack->val._64u = below_top__stack->val._64u >> top_stack->val._64u;

    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( XOR, askf_word_xor ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"xor -> Expects 2 values on stack" , 32 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    top_stack->val._64u = below_top__stack->val._64u ^ top_stack->val._64u;

    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( AND, askf_word_and ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"and -> Expects 2 values on stack" , 32 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    top_stack->val._64u = below_top__stack->val._64u & top_stack->val._64u;

    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( OR, askf_word_or ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"or -> Expects ( a b - )" , 23 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    top_stack->val._64u = below_top__stack->val._64u | top_stack->val._64u;

    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( INVERT, askf_word_invert ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"invert -> Expects ( n - )" , 25 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    askf_stack_pop( top_stack, vm->stack );

    top_stack->val._64u = ~top_stack->val._64u;

    askf_stack_push( top_stack, vm->stack );
}

PROC_WORD( 0SP, askf_word_clearstack ) {
    vm->stack->index = 0;
}

PROC_WORD( TRUE, askf_word_true ) {
    global_c00->val._64u = -1;

    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( FALSE, askf_word_false ) {
    global_c00->val._64u = 0;

    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( FILL, askf_word_fill ) {
    if ( vm->stack->index < 3 ) {
        _askf_word_failed( (ascii*)"FILL -> Expects ( addr n char - )", 33 );
    }

    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"FILL -> cell width must match architecture word width", 53 );
        return;
    }

    AskForth_Cell* _char = global_c00;
    AskForth_Cell* bytes = global_c01;
    AskForth_Cell* addr  = global_c02;

    askf_stack_pop( _char, vm->stack );
    askf_stack_pop( bytes, vm->stack );
    askf_stack_pop( addr , vm->stack );

    FILL( ((u8*)addr->val._64u), _char->val._64u , bytes->val._64u );
}

PROC_WORD( COPY, askf_word_copy ) {
    if ( vm->stack->index < 3 ) {
        _askf_word_failed( (ascii*)"COPY -> Expects ( old_addr new_addr n_bytes - )", 47 );
    }

    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
            (ascii *)"COPY -> cell width must match architecture word width", 53 );
        return;
    }

    AskForth_Cell* bytes     = global_c00;
    AskForth_Cell* new_addr  = global_c01;
    AskForth_Cell* old_addr  = global_c02;

    askf_stack_pop( bytes, vm->stack );
    askf_stack_pop( new_addr, vm->stack );
    askf_stack_pop( old_addr, vm->stack );

    COPY( ((u8*)old_addr->val._64u) , ((u8*)new_addr->val._64u), bytes->val._64u );
}

PROC_WORD( FLUSH, askf_word_flush ) {
    askf_blocks_update();

    if ( askf_blocks_update() != 0 ) {
        _askf_word_failed( (ascii*)"FLUSH -> Failed to update BLOCKS", 32 );
    };
}

PROC_WORD( LIST, askf_word_list ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"LIST -> Expects ( n - )", 23);
        return;
    }
    AskForth_Cell* block_id = global_c00;
    askf_stack_pop( block_id, vm->stack );

    if ( block_id->val._64u > vm->blocks->capacity ) {
        _askf_word_failed( (ascii*)"LOAD -> OOB BLOCK", 17 );
        return;
    }
    if ( block_id->val._64u == 0 ) {
        _askf_word_failed( (ascii*)"LOAD -> 0 is not a BLOCK", 24 );
        return;
    }

    ascii* start_block = (ascii*)
         vm->blocks->start_blocks +  vm->blocks->block_size * (block_id->val._64u -1 );

    u64 max_lines      = 16;
    u64 max_line_chars = vm->blocks->block_size / max_lines;
    AskForth_Cell* cell = global_c01;

    #if defined( TARGET_LINUX )
        askf_print_char('\n');
    #endif

    for ( u64 x = 0; x < max_lines; x++) {
        if ( x < 10 )
            askf_print( (ascii*)"  ", 2);
        else if ( x < 100 )
            askf_print( (ascii*)" ", 1);

        cell->val._64u = x;
        askf_print_cell( cell );
        if ( x < 10 )
            askf_print( (ascii*)"  | ", 4);
        else if ( x < 100 )
            askf_print( (ascii*)"  | ", 4);
        else 
            askf_print( (ascii*)" | ", 3);

        for ( u64 c = 0; c < max_line_chars; c++ )  
            askf_print_char( start_block[(x*max_line_chars) + c] );

        askf_print( (ascii*)"\n", 0);
    }

}

PROC_WORD( BLOCK, askf_word_block ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"BLOCK -> Expects ( n - )", 24);
        return;
    }
    AskForth_Cell* cell = global_c00;
    askf_stack_pop( cell, vm->stack );

    if ( cell->val._64u > vm->blocks->capacity ) {
        _askf_word_failed( (ascii*)"LOAD -> OOB BLOCK", 17 );
        return;
    }
    if ( cell->val._64u == 0 ) {
        _askf_word_failed( (ascii*)"LOAD -> 0 is not a BLOCK", 24 );
        return;
    }
    if ( cell->val._64u > vm->blocks->capacity ) {
        _askf_word_failed( (ascii*)"BLOCK -> OOB BLOCK", 18 );
        return;
    }

    ascii* block_start = vm->blocks->start_blocks + ( vm->blocks->block_size * ( cell->val._64u - 1 ) );

    cell->val._64u = ( u64 )block_start;

    askf_stack_push( cell, vm->stack );
}

PROC_WORD( BLOCK_SIZE, askf_word_block_size ) {
    global_c00->val._64u = vm->blocks->block_size;
    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( LINE, askf_word_line ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"LINE -> Expects ( block_addr n - )", 32 );
        return;
    }

    AskForth_Cell* line       = global_c00;
    AskForth_Cell* blk_addr   = global_c01;

    askf_stack_pop( line, vm->stack );
    askf_stack_pop( blk_addr, vm->stack );

    if ( line->val._64u > 24 ) {
        _askf_word_failed( (ascii*)"LINE -> line > 16", 17);
        return;
    }

    u64 max_line_len     = vm->blocks->block_size / 16;

    blk_addr->val._64u = blk_addr->val._64u + ( max_line_len * line->val._64u );

    askf_stack_push( blk_addr, vm->stack );
}

PROC_WORD( MAX_LINES, askf_word_max_lines ) {
    global_c00->val._64u        = 16;

    askf_stack_push( global_c00, vm->stack );

}

PROC_WORD( CREATE_WORD, askf_word_create_word ) {
    if ( vm->stack->index < 4 ) {
        _askf_word_failed( (ascii*)"CREATE-WORD -> Expects 'str_word_addr len str_dic_name len' ", 60 );
        return;
    }

    AskForth_Cell* cell = global_c00;

    AskForthToken dic_name = {0};

    askf_stack_pop( cell, vm->stack );
    dic_name.length = cell->val._64u;
    askf_stack_pop( cell, vm->stack );
    dic_name.base   = (ascii*)cell->val._64u;

    AskForthToken word_name = {0};

    askf_stack_pop( cell, vm->stack );
    word_name.length = cell->val._64u;
    askf_stack_pop( cell, vm->stack );
    word_name.base   = (ascii*)cell->val._64u;

    AskForth_Dictionary* dic = askf_library_find_dic( vm, &dic_name );

    if ( !dic ) {
        _askf_word_failed( (ascii*)"CREATE-WORD -> Dictionary not found", 35 );

        AskForthError err = {0};
        err.error = ASKF_ERROR_UNKNOWN_DIC;
        err.zone  = ASKF_ERROR_ZONE_INNER;
        err.opt_message = askf_alloc_new_opt_message( dic_name.base, dic_name.length );
        askf_throw_error(err);
        return;
    }

    askf_word_align();
    askf_dic_add_word_threaded( dic, word_name );

    vm->interpret_state  = ASKF_COMPILE;

}

PROC_WORD( COLON, askf_word_colon ) {
    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
            (ascii *)": -> cell width must match architecture word width", 50 );
        return;
    }
    
    askf_word_parse_name();
    askf_word_parse_name();

    if ( vm->stack->index < 4 ) {
        _askf_word_failed( (ascii*)": -> Expects ': word_name dic_name ' ", 37 );
        return;
    }

    AskForth_Cell* cell = global_c00;

    AskForthToken dic_name = {0};

    askf_stack_pop( cell, vm->stack );
    dic_name.length = cell->val._64u;
    askf_stack_pop( cell, vm->stack );
    dic_name.base   = (ascii*)cell->val._64u;

    AskForthToken word_name = {0};

    askf_stack_pop( cell, vm->stack );
    word_name.length = cell->val._64u;
    askf_stack_pop( cell, vm->stack );
    word_name.base   = (ascii*)cell->val._64u;


    if ( word_name.length > ASKF_MAX_NAME_LEN ) {
        _askf_word_failed( (ascii*)": -> word name > 28: ", 21 );
        _askf_word_failed( word_name.base , word_name.length );
        return;
    }

    AskForth_Dictionary* dic = askf_library_find_dic( vm, &dic_name );

    if ( !dic ) {
        _askf_word_failed( (ascii*)": -> Dictionary not found", 25 );

        AskForthError err = {0};
        err.error = ASKF_ERROR_UNKNOWN_DIC;
        err.zone  = ASKF_ERROR_ZONE_INNER;
        err.opt_message = askf_alloc_new_opt_message( dic_name.base, dic_name.length );
        askf_throw_error(err);
        return;
    }

    askf_word_align();
    ascii* new_str = askf_alloc( word_name.length );
    COPY( word_name.base, new_str, (sizeof(ascii) * word_name.length) );
    word_name.base = new_str;

    askf_word_align();
    askf_dic_add_word_threaded( dic, word_name );

    vm->interpret_state  = ASKF_COMPILE;
}

PROC_WORD( COLON_NONAME, askf_word_colon_noname ) {
    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
            (ascii *)":NONAME -> cell width must match architecture word width", 56 );
        return;
    }

    askf_word_align();

    AskForth_Word* new_word = askf_alloc( sizeof( AskForth_Word ) );

    new_word->prev                      = NULL;
    new_word->next                      = NULL;
    new_word->is_immediate              = FALSE;

    new_word->source.type               = ASKF_WORD_THREADED;

    askf_word_align();
    new_word->source.source.threaded_code_start_addr = (u64)askf_alloc( sizeof(u64) );

    new_word->name_len                  = 0;

    ( (AskForth_Library*)vm->lib )->curr_compiling.word = new_word;
    ( (AskForth_Library*)vm->lib )->curr_compiling.here = 
        (u64*)new_word->source.source.threaded_code_start_addr;

    global_c00->val._64u = ( u64 )new_word;
    askf_stack_push( global_c00, vm->stack );

    vm->interpret_state  = ASKF_COMPILE;
}

PROC_WORD( SEMICOLON, askf_word_semicolon ) {
    askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_endword );
    askf_compile_threaded_memory( (u64)ASKF_WORD_REDZONE );
    vm->interpret_state  = ASKF_INTERPRET;
}

PROC_WORD( EXIT, askf_word_exit ) {
    askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_endword );
}

PROC_WORD( IMMEDIATE, askf_word_immediate ) {
    if ( !( (AskForth_Library*)vm->lib )->curr_compiling.word ) {
        _askf_word_failed( (ascii*)"IMMEDIATE -> No last word definition found", 42 );
        return;
    }
    ( (AskForth_Library*)vm->lib )->curr_compiling.word->is_immediate = TRUE;
}

PROC_WORD( IS_IMMEDIATE, askf_word_isimmediate ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"?IMMEDIATE -> Expects ( xt -- flag )", 36 );
        return;
    }
    askf_stack_pop( global_c00, vm->stack );

    AskForth_Word* word = ( AskForth_Word* )global_c00->val._addr_t;
    word->is_immediate ? ( global_c00->val._addr_t = -1 ) : ( global_c00->val._addr_t = 0 ) ;

    askf_stack_push( global_c00, vm->stack );
}

PROC_WORD( INLINE, askf_word_inline ) {
    if ( !( (AskForth_Library*)vm->lib )->curr_compiling.word ) {
        _askf_word_failed( (ascii*)"INLINE -> No last word definition found", 39 );
        return;
    }
    ( (AskForth_Library*)vm->lib )->curr_compiling.word->is_inline = TRUE;
}

PROC_WORD( OPTIMIZE, askf_word_optimize ) {
    if ( !( (AskForth_Library*)vm->lib )->curr_compiling.word ) {
        _askf_word_failed( (ascii*)"OPTIMIZE -> No last word definition found", 41 );
        return;
    }
    if (( (AskForth_Library*)vm->lib )->curr_compiling.word->source.type == ASKF_WORD_NATIVE ) {
        _askf_word_failed( (ascii*)"OPTIMIZE -> Cannot optimize a native word", 41 );
        return;
    }

    askf_optimize_threaded_code(
            (u64*)( (AskForth_Library*)vm->lib )->curr_compiling.word   
            ->source.source.threaded_code_start_addr);

}

PROC_WORD( A, askf_word_add_line_toblock ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"a -> Expects ( block_addr )", 27 );
        return;
    }

    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
            (ascii *)"a -> cell width must match architecture word width", 50 );
        return;
    }

    AskForth_Cell* addr = global_c00;
    askf_stack_pop( addr, vm->stack );

    askf_print( (ascii*)">", 1 );

    u32 read = askf_read_input_blocking_tobuff( vm, (ascii*)addr->val._64u, 
            vm->blocks->block_size );

    // remove the \n from the input
    if ( read )
        ((ascii*)addr->val._64u)[read-1] = ' ';

    addr->val._64u = (u64)read;
    askf_stack_push( addr, vm->stack );
}

PROC_WORD( LOADBLOCK, askf_word_load ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"LOAD -> Expects ( n_block )", 27 );
        return;
    }

    AskForth_Cell* addr = global_c00;
    askf_stack_pop( addr, vm->stack );

    if ( addr->val._64u > vm->blocks->capacity ) {
        _askf_word_failed( (ascii*)"LOAD -> OOB BLOCK", 17 );
        return;
    }
    if ( addr->val._64u == 0 ) {
        _askf_word_failed( (ascii*)"LOAD -> 0 is not a BLOCK", 24 );
        return;
    }

    ascii* block = vm->blocks->start_blocks + vm->blocks->block_size * ( addr->val._64u - 1 );

    askf_istack_push( vm->istack, block, vm->blocks->block_size, 0, addr->val._64u, 0, NULL, 0 );
}

PROC_WORD( DICTIONARY, askf_word_add_dic ) {
    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"ADD-DIC -> cell width must match architecture word width", 56 );
        return;
    }

    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"ADD-DIC -> Expects a name", 25 );
        return;
    }

    AskForth_Cell* len  = global_c00;
    AskForth_Cell* addr = global_c01;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( addr, vm->stack );

    if ( len->val._64u > 28 ) {
        _askf_word_failed( (ascii*)"ADD-DIC -> name too long", 24 );
        return;
    }

    askf_word_align();
    ascii* new_str = askf_alloc( len->val._64u );
    COPY( ((u8*)addr->val._64u), new_str, len->val._64u )

    askf_word_align();
    askf_create_dic( vm, new_str, len->val._64u );
}

PROC_WORD( FORGET_DICT, askf_word_forget_dic ) {
    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"FORGET-DICT -> cell width must match architecture word width", 60 );
        return;
    }

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"FORGET-DICT -> Expects ( dict_xt -- ) ", 38 );
        return;
    }

    AskForth_Cell* addr = global_c01;

    askf_stack_pop( addr, vm->stack );

    AskForth_Dictionary* dict = (AskForth_Dictionary*)addr->val._addr_t;

    if ( dict == NULL ) {
        _askf_word_failed( (ascii*)"FORGET-DICT -> NULL dict_xt", 27 );
        return;
    }

    // clean library
    AskForth_Library* lib = (AskForth_Library*)vm->lib;

    if ( dict->prev && dict->next ) {
        dict->prev->next = dict->next;
        dict->next->prev = dict->prev;
    } else if ( dict->prev && !dict->next ) 
        dict->prev->next = NULL;

    if ( dict->prev && dict->next ) {
        dict->prev->next = dict->next;
        dict->next->prev = dict->prev;
    } else if ( dict->prev && !dict->next )  {
        dict->prev->next = NULL;
        lib->recent_dic = dict->prev;
    } else if ( !dict->prev && dict->next ) {
        dict->next->prev = NULL;
        lib->dictionaries_base = dict->next;
    } else if ( !dict->prev && !dict->next ) {
        return;
    }

    // add to shadow library 
    
    AskForth_ShadowLibrary* slib = (AskForth_ShadowLibrary*)vm->shadow_lib;
    dict->next = NULL;
    dict->prev = NULL;

    if ( slib->dict_base == NULL ) {
        slib->dict_base = dict;
        slib->dict_recent = slib->dict_base;
    } else {
        AskForth_Dictionary* old_shadow_dict = slib->dict_recent;

        old_shadow_dict->next   = dict;
        slib->dict_recent       = dict;
        slib->dict_recent->prev = old_shadow_dict;
        slib->dict_recent->next = NULL;
    }
}

PROC_WORD( REMEMBER_DICT, askf_word_remember_dic ) {
    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"REMEMBER-DICT -> cell width must match architecture word width", 62 );
        return;
    }

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"REMEMBER-DICT -> Expects ( dict_xt -- ) ", 38 );
        return;
    }

    AskForth_Cell* addr = global_c01;

    askf_stack_pop( addr, vm->stack );

    AskForth_Dictionary* dict = (AskForth_Dictionary*)addr->val._addr_t;

    if ( dict == NULL ) {
        _askf_word_failed( (ascii*)"REMEBER-DICT -> NULL dict_xt", 28 );
        return;
    }

    // remove from the shadow library

    AskForth_ShadowLibrary* slib = (AskForth_ShadowLibrary*)vm->shadow_lib;
    AskForth_Dictionary* sbase = slib->dict_base;

    while ( sbase ) {
        if ( (askf_addr_t)sbase == (askf_addr_t)dict )
            break;
        sbase = sbase->next;
    }
    if ( sbase == NULL ) {
        _askf_word_failed( (ascii*)"REMEBER-DICT -> Not a valid dict_xt", 35 );
        return;
    }

    if ( sbase->prev && sbase->next ) {
        sbase->prev->next = sbase->next;
        sbase->next->prev = sbase->prev;
    } else if ( sbase->prev && !sbase->next ) 
        sbase->prev->next = NULL;

    if ( (u64)sbase == (u64)slib->dict_recent ) {
        if ( sbase->prev ) {
            slib->dict_recent = sbase->prev;
        } else {
            slib->dict_base = NULL;
            slib->dict_recent = NULL;
        }
    } else if ( (u64)sbase == (u64)slib->dict_base ) {
        slib->dict_base = NULL;
        slib->dict_recent = NULL;
    }

    // add back to library

    AskForth_Library* lib = (AskForth_Library*)vm->lib;

    dict->next = NULL;
    lib->recent_dic->next = dict;
    dict->prev = lib->recent_dic;
    lib->recent_dic = dict;

    // set to be first dict to search
    // lib->dictionaries_base->prev = dict;
    // dict->next                   = lib->dictionaries_base;
    // dict->prev                   = NULL;
    // lib->dictionaries_base       = dict;


}

PROC_WORD( PROMOTE_DICT, askf_word_promote_dic ) {
    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"PROMOTE-DICT -> cell width must match architecture word width", 61 );
        return;
    }

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"PROMOTE-DICT -> Expects ( dict_xt -- ) ", 39 );
        return;
    }

    AskForth_Cell* addr = global_c01;

    askf_stack_pop( addr, vm->stack );

    AskForth_Dictionary* dict = (AskForth_Dictionary*)addr->val._addr_t;

    if ( dict == NULL ) {
        _askf_word_failed( (ascii*)"PROMOTE-DICT -> NULL dict_xt", 28 );
        return;
    }

    // forget dict
    if ( dict->prev && dict->next ) {
        dict->prev->next = dict->next;
        dict->next->prev = dict->prev;
    } else if ( dict->prev && !dict->next ) 
        dict->prev->next = NULL;

    AskForth_Library* lib = ( AskForth_Library* )vm->lib;

    // set to be first dict to search
    lib->dictionaries_base->prev = dict;
    dict->next                   = lib->dictionaries_base;
    dict->prev                   = NULL;
    lib->dictionaries_base       = dict;
}

PROC_WORD( ABORT, askf_word_abort ) {
    if ( vm->interpret_state == ASKF_COMPILE )
        vm->interpret_state = ASKF_INTERPRET;

    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_EXECUTE );

    vm->input_buffer->index       = 0;
    vm->istack->sources[0].in_max = 0;
    // go back to keyboard input
    vm->istack->index = 1;
    FILL( vm->input_buffer->base, 0, vm->input_buffer->capacity );


    vm->stack->index         = 0;
    vm->rstack->index        = 0;
    vm->cf_stack->index      = 0;
    vm->tframes_stack->index = 0;
}

PROC_WORD( BYE, askf_word_bye ) {
    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_SHUTDOWN_REQUEST );
}

PROC_WORD( ISCOMPTIME, askf_word_iscomptime ) {
    AskForth_Cell* mode  = global_c00;

    mode->val._64u       = vm->interpret_state == ASKF_COMPILE ? -1 : 0 ;

    askf_stack_push( mode, vm->stack );
}

PROC_WORD( ISINTERPTIME, askf_word_isinterptime ) {
    AskForth_Cell* mode  = global_c00;

    mode->val._64u       = vm->interpret_state == ASKF_INTERPRET ? -1 : 0 ;

    askf_stack_push( mode, vm->stack );
}

PROC_WORD( STATE, askf_word_state ) {
    AskForth_Cell* mode  = global_c00;

    mode->val._addr_t = ( askf_addr_t )&vm->interpret_state;

    askf_stack_push( mode, vm->stack );
}

PROC_WORD( IS_WINDOWS, askf_word_is_sys_windows ) {
    AskForth_Cell* mode  = global_c00;

    #if defined( TARGET_WINDOWS )
         mode->val._64u       = -1;
    #else
         mode->val._64u       = 0;
    #endif

    askf_stack_push( mode, vm->stack );
}

PROC_WORD( IS_LINUX, askf_word_is_sys_linux ) {
    AskForth_Cell* mode  = global_c00;

    #if defined( TARGET_LINUX )
         mode->val._64u       = -1;
    #else
         mode->val._64u       = 0;
    #endif

    askf_stack_push( mode, vm->stack );
}

PROC_WORD( WHAT_ARQ_BITS, askf_word_what_arq_bits ) {
    AskForth_Cell* mode  = global_c00;

    #if   defined( ARQBITS64 )
        mode->val._64u       = 64;
    #elif defined( ARQBITS32 )
        mode->val._64u       = 32;
    #elif defined( ARQBITS16 )
        mode->val._64u       = 16;
    #elif defined( ARQBITS8  )
        mode->val._64u       = 8;
    #endif

    askf_stack_push( mode, vm->stack );
}

static boolean _askf_0branch( void ) {
    AskForth_Library* lib       = (AskForth_Library*)vm->lib;

    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"0BRANCH can only be used by compiled code ", 42 );
        return FALSE;
    }

    AskForth_Cell* cell          = global_c00;
    askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_0branch );

    // push into cf_stack the addr of memory on threaded code to later store ( on BRANCH )
    // the offset if flag is 0 on runtime
    cell->val._64u               = ( u64 )lib->curr_compiling.here;
    askf_stack_push( cell, vm->cf_stack );

    lib->curr_compiling.here    = askf_alloc( sizeof( u64 ) );

    return TRUE;
}

static boolean _askf_branch( void ) {
    AskForth_Library* lib       = (AskForth_Library*)vm->lib;

    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"BRANCH can only be used by compiled code ", 41 );
        return FALSE;
    }

    if ( vm->cf_stack->index < 1 ) {
        return FALSE;
    }

    AskForth_Cell* previous_offset_ptr = global_c00;

    askf_stack_pop( previous_offset_ptr, vm->cf_stack );

    *lib->curr_compiling.here   = (u64)vm->dispatch_calls.op_branch;
    lib->curr_compiling.here    = askf_alloc( sizeof(u64) );

    AskForth_Cell* cell          = global_c01;
    cell->val._64u               = ( u64 )lib->curr_compiling.here;
    askf_stack_push( cell, vm->cf_stack );

    lib->curr_compiling.here    = askf_alloc( sizeof(u64) );

    u64 offset = (u64)lib->curr_compiling.here - previous_offset_ptr->val._64u;

    *((u64*)previous_offset_ptr->val._64u) = offset;

    return TRUE;
}

static void askf_word_0branch( void ) {
    if ( !_askf_0branch() ) 
        _askf_word_failed( (ascii*)"0BRANCH can only be used by compiled code ", 42 );
}

static void askf_word_branch( void ) {
    if ( !_askf_branch() ) 
        _askf_word_failed( (ascii*)"BRANCH can only be used by compiled code ", 41 );
}

PROC_WORD( IF, askf_word_if ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"IF -> Can only be used in compiled code", 39 );
        return;
    }

    boolean res = _askf_0branch();

    if ( !res ) 
        _askf_word_failed( (ascii*)"IF -> branch failed", 19 ); 
}

PROC_WORD( ELSE, askf_word_else ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"ELSE -> Can only be used in compiled code", 41 );
        return;
    }

    boolean res = _askf_branch();

    if ( !res ) 
        _askf_word_failed( (ascii*)"ELSE -> branch failed", 21 ); 
}

PROC_WORD( THEN, askf_word_then ) {
    AskForth_Library* lib       = (AskForth_Library*)vm->lib;

    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"THEN -> Must be used in compiled code only", 42 );
        return;
    }

    AskForth_Cell* previous_offset_ptr = global_c00;
    askf_stack_pop( previous_offset_ptr, vm->cf_stack );

    u64 offset = (u64)lib->curr_compiling.here - previous_offset_ptr->val._64u;

    *((u64*)previous_offset_ptr->val._64u) = offset;
}

PROC_WORD( BEGIN, askf_word_begin ) {
    AskForth_Library* lib       = (AskForth_Library*)vm->lib;

    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"BEGIN -> Must be used in compiled code only", 43 );
        return;
    }

    u64 address                 = (u64)lib->curr_compiling.here;

    global_c00->val._64u       = address;
    askf_stack_push( global_c00, vm->cf_stack );
}

PROC_WORD( WHILE, askf_word_while ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"WHILE -> Can only be used in compiled code", 42 );
        return;
    }

    boolean res = _askf_0branch();

    if ( !res ) 
        _askf_word_failed( (ascii*)"WHILE", 5 ); 
}

PROC_WORD( REPEAT, askf_word_repeat ) {
    AskForth_Library* lib = ( AskForth_Library* )vm->lib;

    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"REPEAT -> Can only be used in compiled code", 43 );
        return;
    }

    if ( vm->cf_stack->index < 2 ) {
        _askf_word_failed( (ascii*)"REPEAT -> Missing BEGIN/WHILE", 29 );
        return;
    }

    AskForth_Cell* while_placeholder = global_c00;
    AskForth_Cell* begin_address     = global_c01; 

    askf_stack_pop( while_placeholder, vm->cf_stack );
    askf_stack_pop( begin_address, vm->cf_stack );

    askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_branch );
    u64 offset = begin_address->val._64u - ( u64 )lib->curr_compiling.here;
    askf_compile_threaded_memory( offset );

    offset = ( u64 )lib->curr_compiling.here - while_placeholder->val._64u;
    *( ( u64* )while_placeholder->val._64u )  = offset;
}

PROC_WORD( BRACKET_OPEN, askf_word_bracket_open ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"[ -> Must be called in compile time", 35 );
        return;
    }

    vm->interpret_state = ASKF_INTERPRET;
}

PROC_WORD( BRACKET_CLOSE, askf_word_bracket_close ) {
    if ( vm->interpret_state != ASKF_INTERPRET ) {
        _askf_word_failed( (ascii*)"] -> Must be called in interpret time", 37 );
        return;
    }

    vm->interpret_state = ASKF_COMPILE;
}

PROC_WORD( SINGLE_QUOTE, askf_word_single_quote ) {
    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"'-> cell width must match architecture word width", 49 );
        return;
    }

    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"' -> Expects token", 18 );
        return;
    }

    AskForth_Cell* len  = global_c00;
    AskForth_Cell* addr = global_c01;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( addr, vm->stack );

    AskForthToken token = {0};
    token.base      = ( ascii* )addr->val._64u;
    token.length    = len->val._64u;
    token.line_end  = FALSE;

    AskForth_Word* word =  askf_library_find_word( vm, &token );

    if ( !word ) {
        _askf_word_failed( (ascii*)"' -> Unknown Word", 17 );

        AskForthError err = {0};
        err.zone = ASKF_ERROR_ZONE_OUTER;
        err.error = ASKF_ERROR_UNKNOWN_WORD;
        AskForthErrorMessage* msg = askf_alloc_new_opt_message( token.base, token.length );
        msg->message[msg->length] = '\0';
        err.opt_message = msg;
        askf_throw_error( err );
        return;
    }

    global_c02->val._64u = ( u64 )word;
    askf_stack_push( global_c02, vm->stack );
}

PROC_WORD( EXECUTE, askf_word_execute ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"EXECUTE -> Expects addr", 23 );
        return;
    }

    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"EXECUTE -> cell width must match architecture word width", 56 );
        return;
    }

    AskForth_Cell* addr = global_c00;

    askf_stack_pop( addr, vm->stack );
    AskForth_Word* word = ( AskForth_Word* )addr->val._64u;


    switch ( word->source.type ) {
        case ASKF_WORD_NATIVE:
            word->source.source.native_code();
            break;
        case ASKF_WORD_THREADED:
            askf_stack_push( addr, vm->stack );
            askf_execute_threaded_word();
            break;
        case ASKF_WORD_NATIVE_FOREIGN:
            if ( !askf_trampoline( word->source.source.foreign_sig ) ){
                AskForthError err = {0};
                err.error = ASKF_ERROR_WORD_FOREIGN_FAILED;
                err.zone  = ASKF_ERROR_ZONE_INNER;

                ascii tmp = word->name[word->name_len];
                word->name[word->name_len] = '0';
                AskForthErrorMessage* opt_msg = 
                askf_alloc_new_opt_message( word->name, word->name_len );
                word->name[word->name_len] = tmp;
                err.opt_message = opt_msg;
                askf_throw_error( err );
                return;
            }
            break;
        default:
            break;
    }
}

PROC_WORD( COMPILE_COMMA, askf_word_compile_comma ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"COMPILE, -> Expects addr", 24 );
        return;
    }

    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"COMPILE, -> cell width must match architecture word width", 57 );
        return;
    }

    AskForth_Cell* addr = global_c00;

    askf_stack_pop( addr, vm->stack );

    AskForth_Word* word = ( AskForth_Word* )addr->val._64u;

    switch ( word->source.type ) {
        case ASKF_WORD_NATIVE:
            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_native );
            askf_compile_threaded_memory( (u64)word->source.source.native_code );
            askf_compile_threaded_memory( (u64)word );
            break;
        case ASKF_WORD_THREADED:
            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_threadedword );
            askf_compile_threaded_memory( (u64)word->source.source.threaded_code_start_addr);
            askf_compile_threaded_memory( (u64)word );
            break;
        case ASKF_WORD_NATIVE_FOREIGN:
            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_native_foreign );
            askf_compile_threaded_memory( (u64)word );
            break;
        default:
            break;
    }
}

PROC_WORD( LITERAL, askf_word_literal ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"LITERAL -> Must be used in compile code", 39 );
        return;
    }

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"LITERAL -> Expects ( n - )", 26 );
        return;
    }

    AskForth_Cell* val = global_c00;

    askf_stack_pop( val, vm->stack );

    askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_literal );
    askf_compile_threaded_memory( val->val._64u );
}

PROC_WORD( RLITERAL, askf_word_rliteral ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"RLITERAL -> Must be used in compile code", 40 );
        return;
    }

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"RLITERAL -> Expects ( n - )", 27 );
        return;
    }

    AskForth_Cell* val = global_c00;

    askf_stack_pop( val, vm->stack );

    askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_literal_relocatable );
    askf_compile_threaded_memory( val->val._64u );
}

PROC_WORD( POSTPONE, askf_word_postpone ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"POSTPONE -> Must be used in compile code", 40 );
        return;
    }

    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"POSTPONE -> cell width must match architecture word width", 57 );
        return;
    }

    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"POSTPONE -> Expects token", 25 );
        return;
    }

    AskForth_Cell* len  = global_c00;
    AskForth_Cell* addr = global_c01;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( addr, vm->stack );

    AskForthToken token = {0};
    token.base      = ( ascii* )addr->val._64u;
    token.length    = len->val._64u;
    token.line_end  = FALSE;

    AskForth_Word* word =  askf_library_find_word( vm, &token );

    if ( !word ) {
        _askf_word_failed( (ascii*)"POSTPONE -> Unknown Word", 24 );

        AskForthError err = {0};
        err.zone = ASKF_ERROR_ZONE_OUTER;
        err.error = ASKF_ERROR_UNKNOWN_WORD;
        AskForthErrorMessage* msg = askf_alloc_new_opt_message( token.base, token.length + 1 );
        msg->message[msg->length] = '\0';
        err.opt_message = msg;
        askf_throw_error( err );
        return;
    }

    switch ( word->source.type ) {
        case ASKF_WORD_NATIVE:
            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_native );
            askf_compile_threaded_memory( (u64)word->source.source.native_code );
            askf_compile_threaded_memory( (u64)word );
            break;
        case ASKF_WORD_THREADED:
            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_threadedword );
            askf_compile_threaded_memory( (u64)word->source.source.threaded_code_start_addr);
            askf_compile_threaded_memory( (u64)word );
            break;
        case ASKF_WORD_NATIVE_FOREIGN:
            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_native_foreign );
            askf_compile_threaded_memory( (u64)word );
            break;
    }
}

PROC_WORD( PUSH_RSTACK, askf_word_push_rstack ) {
    AskForth_Cell* val = global_c00;
    
    if ( !askf_stack_pop( val, vm->stack ) ) {
        _askf_word_failed( (ascii*)">R -> Data stack is empty", 25 );
        return;
    }

    askf_stack_push( val, vm->rstack );
}

PROC_WORD( POP_RSTACK, askf_word_pop_rstack ) {
    AskForth_Cell* val = global_c00;
    
    if ( !askf_stack_pop( val, vm->rstack ) ) {
        _askf_word_failed( (ascii*)"R> -> Data stack is empty", 25 );
        return;
    }

    askf_stack_push( val, vm->stack );
}

PROC_WORD( PEEK_RSTACK, askf_word_peek_rstack ) {
    AskForth_Cell* val = global_c00;

    if ( vm->rstack->index < 1 ) {
        _askf_word_failed( (ascii*)"R@ -> rstack is empty", 21 );
        return;
    }

    val->val._64u = vm->rstack->cells.space_64[vm->rstack->index-1];

    askf_stack_push( val, vm->stack );
}

PROC_WORD( SEE, askf_word_see ) {
    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
            (ascii *)"see -> cell width must match architecture word width", 52 );
        return;
    }
    
    askf_word_parse_name();
    askf_word_parse_name();

    if ( vm->stack->index < 4 ) {
        _askf_word_failed( (ascii*)"see -> Expects ': word_name dic_name ' ", 39 );
        return;
    }

    AskForth_Cell* cell = global_c00;

    AskForthToken dic_name = {0};

    askf_stack_pop( cell, vm->stack );
    dic_name.length = cell->val._64u;
    askf_stack_pop( cell, vm->stack );
    dic_name.base   = (ascii*)cell->val._64u;

    AskForthToken word_name;

    askf_stack_pop( cell, vm->stack );
    word_name.length = cell->val._64u;
    askf_stack_pop( cell, vm->stack );
    word_name.base   = (ascii*)cell->val._64u;


    if ( word_name.length > ASKF_MAX_NAME_LEN ) {
        _askf_word_failed( (ascii*)"see -> word name > 28: ", 23 );
        _askf_word_failed( word_name.base , word_name.length );
        return;
    }

    AskForth_Dictionary* dic = askf_library_find_dic( vm, &dic_name );

    if ( !dic ) {
        _askf_word_failed( (ascii*)"see -> Unknown dictionary ", 25 );

        AskForthError err = {0};
        err.error = ASKF_ERROR_UNKNOWN_DIC;
        err.zone  = ASKF_ERROR_ZONE_INNER;
        ascii tmp = dic_name.base[dic_name.length];
        dic_name.base[dic_name.length] = '\0';
        err.opt_message = askf_alloc_new_opt_message( dic_name.base, dic_name.length+1 );
        dic_name.base[dic_name.length] = tmp;

        askf_throw_error(err);
        return;
    }

    AskForth_Word* base = dic->recent_word;
    u64* ip             = NULL;

    while ( base ) {
        if ( base->name_len != word_name.length )
            goto skip_word;

        for ( u64 x = 0; x < word_name.length; x++ )
            if ( base->name[x] != word_name.base[x] )
                goto skip_word;

        if ( base->source.type == ASKF_WORD_NATIVE ) {
            _askf_word_failed( (ascii*)"see -> Cannot see inside a native word", 38 );
            return;
        }
        ip = (u64*)base->source.source.threaded_code_start_addr;
        break;
        
        skip_word:
        base = base->prev;
    }

    if ( !ip ) {
        AskForthError err = {0};
        err.error = ASKF_ERROR_UNKNOWN_WORD;
        err.zone  = ASKF_ERROR_ZONE_INNER;
        err.opt_message = askf_alloc_new_opt_message( word_name.base, word_name.length );
        askf_throw_error(err);
        return;
    }

    #if defined( TARGET_LINUX )
        askf_print_char('\n');
    #endif
    askf_print( (ascii*)": ", 2);
    ascii tmp = word_name.base[word_name.length];
    word_name.base[word_name.length] = '\0';
    askf_print( word_name.base, word_name.length );
    word_name.base[word_name.length] = tmp;
    askf_print( (ascii*)" ", 1);
    tmp = dic_name.base[dic_name.length];
    dic_name.base[dic_name.length] = '\0';
    askf_print( dic_name.base, dic_name.length );
    dic_name.base[dic_name.length] = tmp;
    askf_print_char( '\n' );

    while ( TRUE ) {
        u64 flag = *ip++;

        if ( flag == (u64)vm->dispatch_calls.op_literal ) {
            global_c00->val._64u = *ip;
            askf_print_cell( global_c00 );
            ip++;
        }  
        else if ( flag == (u64)vm->dispatch_calls.op_literal_relocatable ) {
            global_c00->val._64u = *ip;
            askf_print( (ascii*)"RELOCATABLE<", 12);
            askf_print_cell( global_c00 );
            askf_print( (ascii*)">", 1);
            ip++;
        }
        else if ( flag == (u64)vm->dispatch_calls.op_native ) {
            ip++;
            AskForth_Word* word = (AskForth_Word*)*ip;
            askf_print( word->name, word->name_len );
            ip++;
        } 
        else if ( flag == (u64)vm->dispatch_calls.op_threadedword ) {
            ip++;
            AskForth_Word* word = (AskForth_Word*)*ip;
            askf_print( word->name, word->name_len );
            ip++;
        } 
        else if ( flag == (u64)vm->dispatch_calls.opt_noop ) {
            askf_print( (ascii*)"OPT_NOOP", 8 );
            ip++;
        }
        else if ( flag == (u64)vm->dispatch_calls.op_skippable ) {
            u64 bytes_toskip = *ip;
            ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
            askf_print( (ascii*)"_SKIPMEM<",  9 );
            global_c00->val._64u = bytes_toskip;
            askf_print_cell( global_c00 );
            askf_print( (ascii*)" bytes offset>", 14 );
            askf_print_char( '\n' );
        }
        else if ( flag == (u64)vm->dispatch_calls.op_0branch ) {
            ip++; // skip offset
            askf_print_char( '\n' );
            askf_print( (ascii*)"0BRANCH",  7 );
            askf_print_char( '\n' );
        }
        else if ( flag == (u64)vm->dispatch_calls.op_branch ) {
            ip++; // skip offset
            askf_print_char( '\n' );
            askf_print( (ascii*)"BRANCH",  6 );
            askf_print_char( '\n' );
        }
        else if ( flag == (u64)vm->dispatch_calls.opt_type_string ) {
            askf_print( (ascii*)"OPT_TYPE_STRING:",  16 );
        } 
        else if ( flag == (u64)vm->dispatch_calls.op_endword  ) {
            askf_print( (ascii*)"EXIT", 4);
        } else if ( flag == (u64)vm->dispatch_calls.op_dispatch_error ) {
            askf_print( (ascii*)"OP_DISPATCH_ERROR", 17 );
        } else if ( flag == ASKF_WORD_REDZONE ) {
            askf_print_char( '\n' );
            askf_print( (ascii*)"; ", 2);
            break;
        }

        askf_print( (ascii*)" ", 1 );
    }

    if ( base->is_immediate )
        askf_print( (ascii*)"IMMEDIATE ", 10 );

    if ( base->is_inline )
        askf_print( (ascii*)"INLINE", 6 );

    askf_print_char( '\n' );
}

#if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )

PROC_WORD( FOREIGN, askf_word_foreign ) {
    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"FOREIGN -> cell width must match architecture word width", 56 );
        return;
    }
    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"FOREIGN -> Expects token", 24 );
        return;
    }

    AskForth_Cell* len  = global_c00;
    AskForth_Cell* addr = global_c01;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( addr, vm->stack );

    if ( !askf_load_foreign_object( (const ascii*)addr->val._64u, len->val._64u ) ) 
        _askf_word_failed( (ascii*)"FOREIGN -> Could not load foreign object (shared library)", 57 );
}

PROC_WORD( FREEFOREIGN, askf_word_freeforeign ) {
    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"FREEFOREIGN -> cell width must match architecture word width", 60 );
        return;
    }
    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"FREEFOREIGN -> Expects token", 28 );
        return;
    }

    AskForth_Cell* len  = global_c00;
    AskForth_Cell* addr = global_c01;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( addr, vm->stack );

    if ( !askf_free_foreign_object( (const ascii*)addr->val._addr_t, len->val._64u ) ) 
        _askf_word_failed( (ascii*)"FREEFOREIGN -> Foreign object (shared library) not found", 56 );
}

PROC_WORD( DOTFOREIGN, askf_word_dot_foreign ) {
    AskForthForeignObject* base = vm->foreign_manager->objects;

    #if defined( TARGET_LINUX )
        askf_print_char('\n');
    #endif
    askf_print( (ascii*)"Foreign objects (shared libraries): ", 36 );
    while ( base ) {
        askf_print( base->name, base->name_len );
        askf_print( (ascii*)" ", 1 );
        base = base->next;
    }
}




boolean _askf_interp_ffi_params_and_result_to_global_sig( void ) {
    AskForth_InputSource* source = askf_istack_peek( vm->istack );
    AskForthToken tkn = {0};

    boolean got_terminator  = FALSE;

    boolean got_params      = FALSE;
    boolean got_result      = FALSE;

    global_ffi_sig->count = 0;

    while ( TRUE ) {
        tkn = askf_next_token( source );
        if ( tkn.base == NULL && tkn.length == 0 ) 
            break;

        if ( tkn.length == 3 ) { 
            if ( _strequal( tkn.base, (ascii*)"u64", 3 ) ) {
                if ( !got_params ) {
                    global_ffi_sig->args[global_ffi_sig->count++] = ASKF_ARG_U64;
                } else {
                    if ( got_result ) {
                        _askf_word_failed( (ascii*)"FUNCTION: -> result must only contain one return type ", 54 );
                        return FALSE;
                    }
                    global_ffi_sig->ret_type = ASKF_ARG_U64;
                    got_result = TRUE;
                }
            }
            if ( _strequal( tkn.base, (ascii*)"d64", 3 ) )  {
                if ( !got_params ) {
                    global_ffi_sig->args[global_ffi_sig->count++] = ASKF_ARG_I64;
                } else {
                    if ( got_result ) {
                        _askf_word_failed( (ascii*)"FUNCTION: -> result must only contain one return type ", 54 );
                        return FALSE;
                    }
                    global_ffi_sig->ret_type = ASKF_ARG_U64;
                    got_result = TRUE;
                }
            }
        } else if ( tkn.length == 4 ) {
            if ( _strequal( tkn.base, (ascii*)"addr", 4 ) ) {
                if ( !got_params ) {
                    global_ffi_sig->args[global_ffi_sig->count++] = ASKF_ARG_PTR;
                } else {
                    if ( got_result ) {
                        _askf_word_failed( (ascii*)"FUNCTION: -> result must only contain one return type ", 54 );
                        return FALSE;
                    }
                    global_ffi_sig->ret_type = ASKF_ARG_U64;
                    got_result = TRUE;
                }
            } else
            if ( _strequal( tkn.base, (ascii*)"void", 4 ) ) {
                if ( !got_params ) {
                    if ( global_ffi_sig->count > 0 ) {
                        _askf_word_failed( (ascii*)"FUNCTION: -> when using 'void' it must be the only param", 56 );
                        return FALSE;
                    }
                    global_ffi_sig->args[0] = ASKF_ARG_VOID;
                } else {
                    if ( got_result ) {
                        _askf_word_failed( (ascii*)"FUNCTION: -> result must only contain one return type ", 54 );
                        return FALSE;
                    }
                    global_ffi_sig->ret_type = ASKF_ARG_VOID;
                    got_result = TRUE;
                }
            }
        }
        else if ( tkn.length == 2 ) 
        {
            if ( _strequal( tkn.base, (ascii*)"--", 2 ) ) {
                got_params = TRUE;
            }

        }

        if ( tkn.base[tkn.length - 1] == ')' ) {
            got_terminator = TRUE;
            if ( !got_params && global_ffi_sig->count > 0 ) {
                _askf_word_failed( (ascii*)"FUNCTION: -> params found but '--' was not found ", 49 );
                return FALSE;
            } else if ( got_params && !got_result ) {
                _askf_word_failed( (ascii*)"FUNCTION: -> no return type found after '--' ", 45 );
                return FALSE;
            }
            break;
        } 

    }

    if ( !got_terminator ) {
        _askf_word_failed( (ascii*)"FUNCTION: -> Terminator ')' not found on input buffer", 53 );
        return FALSE;
    }

    return TRUE;
}


PROC_WORD( FUNCTION, askf_word_function ) {
    if ( _stack_invalid_for_addresses() ) {
        _askf_word_failed( 
                (ascii *)"FUNCTION: -> cell width must match architecture word width", 58 );
        return;
    }
    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"FUNCTION: -> Expects function name", 34 );
        return;
    }

    AskForth_Cell* len_name  = global_c00;
    AskForth_Cell* addr_name = global_c01;

    askf_stack_pop( len_name, vm->stack );
    askf_stack_pop( addr_name, vm->stack );

    AskForthToken function_name;
    function_name.base   = (ascii*)addr_name->val._addr_t;
    function_name.length = len_name->val._64u;
    function_name.line_end = FALSE;

    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"FUNCTION: -> Expects '(' for stack effects ", 43 );
        return;
    }

    AskForth_Cell* len_paren  = global_c02;
    AskForth_Cell* addr_paren = global_c03;

    askf_stack_pop( len_paren, vm->stack );
    askf_stack_pop( addr_paren, vm->stack );

    if ( ((ascii*)addr_paren->val._addr_t)[0] != '(' && len_paren->val._addr_t != 1 ) {
        _askf_word_failed( (ascii*)"FUNCTION: -> No ' ( params -- result ) ' found", 46 );
        return;
    }

    // create foreign function signature
    global_ffi_sig->count = 0;
    if ( !_askf_interp_ffi_params_and_result_to_global_sig() )
        return;


    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"FUNCTION: -> Expects dictionary name", 36 );
        return;
    }

    AskForth_Cell* len_dic  = global_c00;
    AskForth_Cell* addr_dic = global_c01;

    askf_stack_pop( len_dic, vm->stack );
    askf_stack_pop( addr_dic, vm->stack );

    AskForthToken dic_name;
    dic_name.base     = (ascii*)addr_dic->val._addr_t;
    dic_name.length   = len_dic->val._64u;
    dic_name.line_end = FALSE;


    if ( !askf_bind_function( function_name.base, function_name.length, global_ffi_sig ) ) 
        _askf_word_failed( (ascii*)"FUNCTION: -> Could not bind to the C function", 45 );

    ascii* new_str = askf_alloc( sizeof(ascii) * function_name.length );
    COPY( function_name.base, new_str, function_name.length );
    function_name.base  = new_str;
    askf_dic_add_word_foreign_native( dic_name, global_ffi_sig, function_name );
}
#endif

PROC_WORD( EVALUATE, askf_word_evaluate ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"EVALUATE -> Expects ( addr u -- ) ", 34 );
        return;
    }

    askf_stack_pop( global_c00, vm->stack );
    askf_stack_pop( global_c01, vm->stack );

    askf_istack_push( vm->istack, (ascii*)global_c01->val._addr_t, global_c00->val._64u+1, -1, 0, 0, NULL, 0 );
}

PROC_WORD( STARTER, askf_word_starter ) {
    askf_word_single_quote();
    if ( vm->outer_state != ASKF_VM_OUTER_STATE_EXECUTE )
        return;

    askf_stack_pop( global_c00, vm->stack );
    askf_new_starterword( global_c00->val._addr_t );
}

PROC_WORD( PROGRAM, askf_word_program ) {
    askf_word_parse_name();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"PROGRAM -> Path must be included", 32 );
        return;
    }

    AskForth_Cell* len   = global_c00;
    AskForth_Cell* path  = global_c01;

    askf_stack_pop( len, vm->stack );
    askf_stack_pop( path, vm->stack );

    if ( !askf_save_image( vm, (ascii*)path->val._addr_t, len->val._addr_t ) ) {
        _askf_word_failed( (ascii*)"PROGRAM -> Failed to save new AskForth Image", 44 );
        return;
    }
}

#if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
static u64 _askf_custom_fgets( ascii* buff, u64 cap, FILE* stream, int* is_eof ) {
    if ( cap == 0 || buff == NULL || stream == NULL )  {
        *is_eof = 0;
        return 0;
    }

    u64 bytes_read = 0;
    *is_eof        = 0;

    while ( bytes_read < cap ) {
        int ch = fgetc( stream );

        if ( ch == EOF ) {
            *is_eof = 1;
            return bytes_read;
        }

        if ( ch == '\n' ) {
            buff[bytes_read++] = (ascii)ch;
            break;
        }

        if ( ch == '\r' ) {
            // Peek ahead to handle Windows CRLF properly
            int next_ch = fgetc( stream );

            if ( next_ch == '\n' ) {
                if ( bytes_read < cap )
                    buff[bytes_read++] = (ascii)ch;
            } 
            else if ( next_ch != EOF ) 
                ungetc( next_ch, stream );

            break;
        }


        buff[bytes_read++] = (ascii)ch;
    }

    return bytes_read;
}


PROC_WORD( INCLUDE, askf_word_include ) {
        askf_word_parse_name();

        if ( vm->stack->index < 2 ) {
            _askf_word_failed( (ascii*)"INCLUDE -> Path must be included", 32 );
            return;
        }

        AskForth_Cell* len   = global_c00;
        AskForth_Cell* path  = global_c01;

        askf_stack_pop( len, vm->stack );
        askf_stack_pop( path, vm->stack );

        ascii* alloced_name = malloc( len->val._addr_t );

        ascii tmp = ((ascii*)path->val._64u)[len->val._64u];

        ((ascii*)path->val._64u)[len->val._64u] = '\0';
        FILE *f = fopen( (char*)path->val._64u, "r");
        ((ascii*)path->val._64u)[len->val._64u] = tmp;
        COPY( path->val._64u, alloced_name, len->val._addr_t );

        if ( !f ) {
            _askf_word_failed( (ascii*)"INCLUDE -> Could not open file", 30 );
            _askf_word_failed( (ascii*)path->val._64u, len->val._64u );
            return;
        }

        int is_eof;

        ascii* base = malloc( KB(4) );
        u64    base_idx = 0;
        u64    baselen  = KB(4);

        int fileid = fileno(f);
        AskForth_InputSource* source = NULL; 
        u64 read = _askf_custom_fgets( base, baselen - 1, f, &is_eof );

        base_idx = read;
        base[base_idx] = '\0';
        askf_istack_push( vm->istack, base, baselen, fileid, 0, (askf_addr_t)f, alloced_name, len->val._64u );
        source = askf_istack_peek( vm->istack );
        source->in_max = base_idx;
}

PROC_WORD( INCLUDED, askf_word_included ) {
        if ( vm->stack->index < 2 ) {
            _askf_word_failed( (ascii*)"INCLUDED -> Expects ( c-addr u -- )", 35 );
            return;
        }

        AskForth_Cell* len   = global_c00;
        AskForth_Cell* path  = global_c01;

        askf_stack_pop( len, vm->stack );
        askf_stack_pop( path, vm->stack );

        ascii* alloced_name = malloc( len->val._addr_t );

        ascii tmp = ((ascii*)path->val._64u)[len->val._64u];

        ((ascii*)path->val._64u)[len->val._64u] = '\0';
        FILE *f = fopen( (char*)path->val._64u, "r");
        ((ascii*)path->val._64u)[len->val._64u] = tmp;
        COPY( path->val._64u, alloced_name, len->val._addr_t );

        if ( !f ) {
            _askf_word_failed( (ascii*)"INCLUDE -> Could not open file", 30 );
            _askf_word_failed( (ascii*)path->val._64u, len->val._64u );
            return;
        }

        int is_eof;

        ascii* base = malloc( KB(4) );
        u64    base_idx = 0;
        u64    baselen  = KB(4);

        int fileid = fileno(f);
        AskForth_InputSource* source = NULL; 
        u64 read = _askf_custom_fgets( base, baselen - 1, f, &is_eof );

        base_idx = read;
        base[base_idx] = '\0';
        askf_istack_push( vm->istack, base, baselen, fileid, 0, (askf_addr_t)f, alloced_name, len->val._64u );
        source = askf_istack_peek( vm->istack );
        source->in_max = base_idx;
}
#endif

PROC_WORD( REFILL, askf_word_refill ) {
    AskForth_InputSource* source = askf_istack_peek( vm->istack );

    // input source EVALUATE return false
    if ( source->source_id == -1 ) {
        global_c00->val._addr_t = FORTH_FALSE;
        askf_stack_push( global_c00, vm->stack );
        return;
    }

    // input source from user input device
    if ( source->blk == 0 && source->source_id == 0 ) {
        askf_read_input_blocking( vm );
        global_c00->val._addr_t = FORTH_TRUE;
        askf_stack_push( global_c00, vm->stack );

    // input source from text file
    } else if ( source->blk == 0 && source->source_id > 0 ) {
        #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
            int is_eof;
            u64 read = _askf_custom_fgets(source->base , source->byte_cap-1, (FILE*)source->file, &is_eof);

            source->in = 0;
            source->in_max = read;
            if ( is_eof && read == 0 ) {
                free( source->base );
                free( source->file_name );
                fclose( (FILE*)source->file );
                askf_istack_pop( vm->istack );
                askf_stack_push( global_c00, vm->stack );
                global_c00->val._addr_t = FORTH_FALSE;
            } else if ( read > 0 ) {
                source->base[read] = '\0';
                global_c00->val._addr_t = FORTH_TRUE;
                askf_stack_push( global_c00, vm->stack );
            }
        #endif

    // input source from block
    } else if ( source->blk != 0 ) {
        ascii* block = vm->blocks->start_blocks + vm->blocks->block_size * ( source->blk );

        source->base = block;
        source->blk += 1;
        source->in = 0;
    }

}

void _askf_print_failed_add_word( AskForthToken* tkn ) {
    askf_print( (ascii*)"Failed adding '", 15 );
    askf_print( tkn->base, tkn->length );
    askf_print( (ascii*)"' word to 'Core' Dictionary\n", 29 );
}

void askf_initialize_core( void ) {
    vm                  = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack );

    askf_word_align();
    global_c00 = askf_alloc( sizeof(AskForth_Cell) );
    askf_word_align();
    global_c01 = askf_alloc( sizeof(AskForth_Cell) );
    askf_word_align();
    global_c02 = askf_alloc( sizeof(AskForth_Cell) );
    askf_word_align();
    global_c03 = askf_alloc( sizeof(AskForth_Cell) );

    askf_word_align();
    global_ffi_sig = askf_alloc( sizeof(AskForthForeignFuncSig) );
    global_ffi_sig->count = 0;

    COPY( &cell, global_c00, sizeof(AskForth_Cell) );
    COPY( &cell, global_c01, sizeof(AskForth_Cell) );
    COPY( &cell, global_c02, sizeof(AskForth_Cell) );
    COPY( &cell, global_c03, sizeof(AskForth_Cell) );

    // trickery to set jump labels set (happens on first run)
    _askf_execute_threaded_frames( vm->istack->index );
}

enum { MAX_WID = __COUNTER__-1 };

#define IS_IMMEDIATE 1
#define NOT_IMMEDIATE 0

// could be a function but i dont want to pass more arguments xd
// only works inside the 'askf_add_core_words' local scope
#define NEW_NATIVE_WORD( str,is_immediate, addr )    \
    scratch_word_name.base            = (ascii*)(str);                                          \
    counter = 0;                                                                                \
    while ( *(str) != 0 ) {                                                                       \
        counter++;  \
        (str)++;                                                                       \
    }                                                                                           \
    scratch_word_name.length          = counter;                                                \
                                                                                                \
    res =                                                                                       \
        askf_dic_add_word_native( core_dic_name, (is_immediate), addr, scratch_word_name );   \
                                                                                                \
    if ( !res )                                                                                 \
        _askf_print_failed_add_word( &scratch_word_name );                                      \

// we are keeping track of these strings so we can relocate them after an askforth image boot up
ascii* str_names_table[] = {
    [WID(ALIGNED)]              = FSTR("ALIGNED"),
    [WID(ALIGN)]                = FSTR("ALIGN"),
    [WID(DOT)]                  = FSTR("."),
    [WID(SOURCE)]               = FSTR("SOURCE"),
    [WID(SOURCEID)]             = FSTR("SOURCE-ID"),
    [WID(SOURCEIN)]             = FSTR(">IN"),
    [WID(BLK)]                  = FSTR("blk"),
    [WID(DEPTH)]                = FSTR("depth"),
    [WID(BASE)]                 = FSTR("BASE"),
    [WID(DECIMAL)]              = FSTR("DECIMAL"),
    [WID(HEXADECIMAL)]          = FSTR("HEX"),
    [WID(OCTAL)]                = FSTR("OCTAL"),
    [WID(DOT_STACK)]            = FSTR(".s"),
    [WID(DUP)]                  = FSTR("dup"),
    [WID(MAYBE_DUP)]            = FSTR("?dup"),
    [WID(2DUP)]                 = FSTR("2dup"),
    [WID(SWAP)]                 = FSTR("swap"),
    [WID(2SWAP)]                = FSTR("2swap"),
    [WID(NIP)]                  = FSTR("nip"),
    [WID(TUCK)]                 = FSTR("tuck"),
    [WID(DROP)]                 = FSTR("drop"),
    [WID(2DROP)]                = FSTR("2drop"),
    [WID(OVER)]                 = FSTR("over"),
    [WID(2OVER)]                = FSTR("2over"),
    [WID(NEGATE)]               = FSTR("negate"),
    [WID(LIB)]                  = FSTR("LIB"),
    [WID(FROM)]                 = FSTR("[FROM]"),
    [WID(FIND)]                 = FSTR("[FIND]"),
    [WID(SFIND)]                = FSTR("FIND"),
    [WID(PARSE_NAME)]           = FSTR("PARSE-NAME"),
    [WID(FORGET)]               = FSTR("[FORGET]"),
    [WID(XTFORGET)]             = FSTR("FORGET"),
    [WID(WORDS)]                = FSTR("words:"),
    [WID(ADD)]                  = FSTR("+"),
    [WID(MINUS)]                = FSTR("-"),
    [WID(MULTIPLY)]             = FSTR("*"),
    [WID(SLASHMOD)]             = FSTR("/mod"),
    [WID(EQUALS)]               = FSTR("="),
    [WID(EQUALS_ZERO)]          = FSTR("0="),
    [WID(LESS_THAN)]            = FSTR("<"),
    [WID(LESS_THAN_OR_EQUAL)]   = FSTR("<="),
    [WID(LESS_THAN_ZERO)]       = FSTR("0<"),
    [WID(MORE_THAN)]            = FSTR(">"),
    [WID(MORE_THAN_OR_EQUAL)]   = FSTR(">="),
    [WID(MORE_THAN_ZERO)]       = FSTR("0>"),
    [WID(NOT_EQUAL)]            = FSTR("<>"),
    [WID(NOT_EQUAL_ZERO)]       = FSTR("0<>"),
    [WID(SIGNED)]               = FSTR("SIGNED"),
    [WID(UNSIGNED)]             = FSTR("UNSIGNED"),
    [WID(BITS)]                 = FSTR("BITS"),
    [WID(WHATBITS)]             = FSTR("?BITS"),
    [WID(TYPE)]                 = FSTR("TYPE"),
    [WID(STORE)]                = FSTR("!"),
    [WID(BYTE_STORE)]           = FSTR("c!"),
    [WID(LOAD)]                 = FSTR("@"),
    [WID(BYTE_LOAD)]            = FSTR("c@"),
    [WID(HERE)]                 = FSTR("HERE"),
    [WID(ALLOT)]                = FSTR("ALLOT"),
    [WID(UNUSED)]               = FSTR("UNUSED"),
    [WID(PRINT_STRING)]         = FSTR(".\""),
    [WID(STORE_STRING)]         = FSTR("s\""),
    [WID(ERROR_WITH_STRING)]    = FSTR("error\""),
    [WID(CR)]                   = FSTR("cr"),
    [WID(EMIT)]                 = FSTR("EMIT"),
    [WID(LSHIFT)]               = FSTR("lshift"),
    [WID(RSHIFT)]               = FSTR("rshift"),
    [WID(XOR)]                  = FSTR("xor"),
    [WID(AND)]                  = FSTR("and"),
    [WID(ROT)]                  = FSTR("rot"),
    [WID(OR)]                   = FSTR("or"),
    [WID(INVERT)]               = FSTR("invert"),
    [WID(0SP)]                  = FSTR("0SP"),
    [WID(TRUE)]                 = FSTR("TRUE"),
    [WID(FALSE)]                = FSTR("FALSE"),
    [WID(CELLS)]                = FSTR("cells"),
    [WID(CELLS_ADD)]            = FSTR("cell+"),
    [WID(CHARS)]                = FSTR("chars"),
    [WID(CHARS_ADD)]            = FSTR("char+"),
    [WID(FILL)]                 = FSTR("FILL"),
    [WID(COPY)]                 = FSTR("COPY"),
    [WID(COMMENT_PAREN)]        = FSTR("("),
    [WID(COMMENT_SLASH)]        = FSTR("\\"),
    [WID(FLUSH)]                = FSTR("FLUSH"),
    [WID(LIST)]                 = FSTR("LIST"),
    [WID(LOADBLOCK)]            = FSTR("LOAD"),
    [WID(BLOCK)]                = FSTR("BLOCK"),
    [WID(BLOCK_SIZE)]           = FSTR("BLOCK-SIZE"),
    [WID(LINE)]                 = FSTR("LINE"),
    [WID(A)]                    = FSTR("a"),
    [WID(MAX_LINES)]            = FSTR("MAX-LINES"),

    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        [WID(INCLUDED)]         = FSTR("INCLUDED"),
        [WID(INCLUDE)]          = FSTR("INCLUDE"),
    #endif
    [WID(CREATE_WORD)]          = FSTR("CREATE-WORD"),
    [WID(COLON)]                = FSTR(":"),
    [WID(COLON_NONAME)]         = FSTR(":NONAME"),
    [WID(SEMICOLON)]            = FSTR(";"),
    [WID(IMMEDIATE)]            = FSTR("IMMEDIATE"),
    [WID(IS_IMMEDIATE)]         = FSTR("?IMMEDIATE"),
    [WID(OPTIMIZE)]             = FSTR("OPTIMIZE"),
    [WID(INLINE)]               = FSTR("INLINE"),
    [WID(DICTIONARY)]           = FSTR("DICTIONARY"),
    [WID(FORGET_DICT)]          = FSTR("FORGET-DICT"),
    [WID(REMEMBER_DICT)]        = FSTR("REMEMBER-DICT"),
    [WID(PROMOTE_DICT)]         = FSTR("PROMOTE-DICT"),
    [WID(ABORT)]                = FSTR("ABORT"),
    [WID(EXIT)]                 = FSTR("EXIT"),
    [WID(BYE)]                  = FSTR("bye"),
    [WID(ISCOMPTIME)]           = FSTR("?COMPTIME"),
    [WID(ISINTERPTIME)]         = FSTR("?INTERPTIME"),
    [WID(STATE)]                = FSTR("STATE"),
    [WID(IS_WINDOWS)]           = FSTR("?SYSTEM-WINDOWS"),
    [WID(IS_LINUX)]             = FSTR("?SYSTEM-LINUX"),
    [WID(WHAT_ARQ_BITS)]        = FSTR("?ARQ-BITS"),
    [WID(REFILL)]               = FSTR("REFILL"),
    [WID(IF)]                   = FSTR("IF"),
    [WID(ELSE)]                 = FSTR("ELSE"),
    [WID(THEN)]                 = FSTR("THEN"),
    [WID(BEGIN)]                = FSTR("BEGIN"),
    [WID(WHILE)]                = FSTR("WHILE"),
    [WID(REPEAT)]               = FSTR("REPEAT"),
    [WID(BRACKET_OPEN)]         = FSTR("["),
    [WID(BRACKET_CLOSE)]        = FSTR("]"),
    [WID(SINGLE_QUOTE)]         = FSTR("'"),
    [WID(LITERAL)]              = FSTR("LITERAL"),
    [WID(RLITERAL)]             = FSTR("RLITERAL"),
    [WID(POSTPONE)]             = FSTR("POSTPONE"),
    [WID(EVALUATE)]             = FSTR("EVALUATE"),
    [WID(EXECUTE)]              = FSTR("EXECUTE"),
    [WID(COMPILE_COMMA)]        = FSTR("COMPILE,"),
    [WID(PUSH_RSTACK)]          = FSTR(">R"),
    [WID(POP_RSTACK)]           = FSTR("R>"),
    [WID(PEEK_RSTACK)]          = FSTR("R@"),
    [WID(SEE)]                  = FSTR("see"),
    [WID(STARTER)]              = FSTR("STARTER"),
    [WID(PROGRAM)]              = FSTR("PROGRAM"),

    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        [WID(FOREIGN)]          = FSTR("FOREIGN"),
        [WID(DOTFOREIGN)]       = FSTR(".FOREIGN"),
        [WID(FREEFOREIGN)]      = FSTR("FREEFOREIGN"),
        [WID(FUNCTION)]         = FSTR("FUNCTION:"),
    #endif
};

#define GET_WID_STR(WID)  (str_names_table[(WID)])

ascii* askf_get_wid_word_address( askf_addr_t wid ) {
    if ( wid > MAX_WID ) 
        return NULL;

    return GET_WID_STR(wid);
}

askf_addr_t askf_get_word_id( ascii* name, askf_addr_t len ) {
    if ( name == NULL )
        return -1;

    for ( askf_addr_t x = 0; x < MAX_WID; x++ ) {
        if ( _strequal( name, str_names_table[x], len) )
            return x;
    }

    return -1;
}

void askf_add_core_words( void ) {
    AskForthToken core_dic_name = {0};
    core_dic_name.base          = (ascii*)"core";
    core_dic_name.length        = 4;

    AskForthToken scratch_word_name = {0};

    boolean res = FALSE;
    // for counting strlen
    u8 counter  = 0;

    NEW_NATIVE_WORD( GET_WID_STR(WID(SOURCE)),     NOT_IMMEDIATE, askf_word_input_source       );

    NEW_NATIVE_WORD( GET_WID_STR(WID(SOURCEID)),   NOT_IMMEDIATE, askf_word_input_source_id    );
    NEW_NATIVE_WORD( GET_WID_STR(WID(SOURCEIN)),   NOT_IMMEDIATE, askf_word_input_source_in    );
    NEW_NATIVE_WORD( GET_WID_STR(WID(BLK)),        NOT_IMMEDIATE, askf_word_input_source_blk   );
    NEW_NATIVE_WORD( GET_WID_STR(WID(REFILL)),     NOT_IMMEDIATE, askf_word_refill             );
    NEW_NATIVE_WORD( GET_WID_STR(WID(ALIGNED)),    NOT_IMMEDIATE, askf_word_aligned            );
    NEW_NATIVE_WORD( GET_WID_STR(WID(ALIGN)),      NOT_IMMEDIATE, askf_word_align              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(DOT)),        NOT_IMMEDIATE, askf_word_dot                );
    NEW_NATIVE_WORD( GET_WID_STR(WID(DEPTH)),      NOT_IMMEDIATE, askf_word_stack_depth        );
    NEW_NATIVE_WORD( GET_WID_STR(WID(BASE)),        NOT_IMMEDIATE, askf_word_base               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(DECIMAL)),     NOT_IMMEDIATE, askf_word_decimal            );
    NEW_NATIVE_WORD( GET_WID_STR(WID(HEXADECIMAL)), NOT_IMMEDIATE, askf_word_hexadecimal        );
    NEW_NATIVE_WORD( GET_WID_STR(WID(OCTAL)),       NOT_IMMEDIATE, askf_word_octal              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(DOT_STACK)),   NOT_IMMEDIATE, askf_word_dot_stack          );
    NEW_NATIVE_WORD( GET_WID_STR(WID(DUP)),         NOT_IMMEDIATE, askf_word_dup                );
    NEW_NATIVE_WORD( GET_WID_STR(WID(MAYBE_DUP)),   NOT_IMMEDIATE, askf_word_maybe_dup          );
    NEW_NATIVE_WORD( GET_WID_STR(WID(2DUP)),        NOT_IMMEDIATE, askf_word_2dup               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(SWAP)),        NOT_IMMEDIATE, askf_word_swap               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(2SWAP)),       NOT_IMMEDIATE, askf_word_2swap              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(NIP)),         NOT_IMMEDIATE, askf_word_nip                );
    NEW_NATIVE_WORD( GET_WID_STR(WID(TUCK)),        NOT_IMMEDIATE, askf_word_tuck               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(DROP)),        NOT_IMMEDIATE, askf_word_drop               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(2DROP)),       NOT_IMMEDIATE, askf_word_2drop              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(OVER)),        NOT_IMMEDIATE, askf_word_over               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(2OVER)),       NOT_IMMEDIATE, askf_word_2over              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(NEGATE)),      NOT_IMMEDIATE, askf_word_negate             );
    NEW_NATIVE_WORD( GET_WID_STR(WID(LIB)),         NOT_IMMEDIATE, askf_word_lib                );
    NEW_NATIVE_WORD( GET_WID_STR(WID(FROM)),       IS_IMMEDIATE, askf_word_from               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(FIND)),       IS_IMMEDIATE, askf_word_find               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(SFIND)),        NOT_IMMEDIATE, askf_word_sfind              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(FORGET)),     IS_IMMEDIATE, askf_word_forget             );
    NEW_NATIVE_WORD( GET_WID_STR(WID(XTFORGET)),      NOT_IMMEDIATE, askf_word_xtforget           );
    NEW_NATIVE_WORD( GET_WID_STR(WID(PARSE_NAME)),  NOT_IMMEDIATE, askf_word_parse_name         );
    NEW_NATIVE_WORD( GET_WID_STR(WID(WORDS)),      NOT_IMMEDIATE, askf_word_words              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(ADD)),           NOT_IMMEDIATE, askf_word_add                );
    NEW_NATIVE_WORD( GET_WID_STR(WID(MINUS)),           NOT_IMMEDIATE, askf_word_minus              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(MULTIPLY)),           NOT_IMMEDIATE, askf_word_multiply           );
    NEW_NATIVE_WORD( GET_WID_STR(WID(SLASHMOD)),        NOT_IMMEDIATE, askf_word_slashmod           );
    NEW_NATIVE_WORD( GET_WID_STR(WID(EQUALS)),           NOT_IMMEDIATE, askf_word_equals             );
    NEW_NATIVE_WORD( GET_WID_STR(WID(EQUALS_ZERO)),          NOT_IMMEDIATE, askf_word_equals_zero        );
    NEW_NATIVE_WORD( GET_WID_STR(WID(LESS_THAN)),           NOT_IMMEDIATE, askf_word_less_than          );
    NEW_NATIVE_WORD( GET_WID_STR(WID(LESS_THAN_OR_EQUAL)),          NOT_IMMEDIATE, askf_word_less_than_or_equal );
    NEW_NATIVE_WORD( GET_WID_STR(WID(LESS_THAN_ZERO)),          NOT_IMMEDIATE, askf_word_less_than_zero     );
    NEW_NATIVE_WORD( GET_WID_STR(WID(MORE_THAN)),           NOT_IMMEDIATE, askf_word_more_than          );
    NEW_NATIVE_WORD( GET_WID_STR(WID(MORE_THAN_OR_EQUAL)),          NOT_IMMEDIATE, askf_word_more_than_or_equal );
    NEW_NATIVE_WORD( GET_WID_STR(WID(MORE_THAN_ZERO)),          NOT_IMMEDIATE, askf_word_more_than_zero     );
    NEW_NATIVE_WORD( GET_WID_STR(WID(NOT_EQUAL)),          NOT_IMMEDIATE, askf_word_not_equal          );
    NEW_NATIVE_WORD( GET_WID_STR(WID(NOT_EQUAL_ZERO)),         NOT_IMMEDIATE, askf_word_not_equal_zero     );
    NEW_NATIVE_WORD( GET_WID_STR(WID(SIGNED)),      NOT_IMMEDIATE, askf_word_make_stack_signed  );
    NEW_NATIVE_WORD( GET_WID_STR(WID(UNSIGNED)),    NOT_IMMEDIATE, askf_word_make_stack_unsigned );
    NEW_NATIVE_WORD( GET_WID_STR(WID(BITS)),        NOT_IMMEDIATE, askf_word_bits               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(WHATBITS)),       NOT_IMMEDIATE, askf_word_whatbits           );
    NEW_NATIVE_WORD( GET_WID_STR(WID(TYPE)),        NOT_IMMEDIATE, askf_word_type               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(STORE)),           NOT_IMMEDIATE, askf_word_store              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(BYTE_STORE)),          NOT_IMMEDIATE, askf_word_byte_store         );
    NEW_NATIVE_WORD( GET_WID_STR(WID(LOAD)),           NOT_IMMEDIATE, askf_word_load_ptr           );
    NEW_NATIVE_WORD( GET_WID_STR(WID(BYTE_LOAD)),          NOT_IMMEDIATE, askf_word_load_byte_ptr      );
    NEW_NATIVE_WORD( GET_WID_STR(WID(HERE)),        NOT_IMMEDIATE, askf_word_here               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(ALLOT)),       NOT_IMMEDIATE, askf_word_allot              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(UNUSED)),      NOT_IMMEDIATE, askf_word_unused             );
    NEW_NATIVE_WORD( GET_WID_STR(WID(PRINT_STRING)),          IS_IMMEDIATE, askf_word_print_string       );
    NEW_NATIVE_WORD( GET_WID_STR(WID(STORE_STRING)),          IS_IMMEDIATE, askf_word_store_string       );
    NEW_NATIVE_WORD( GET_WID_STR(WID(ERROR_WITH_STRING)),      IS_IMMEDIATE, askf_word_error_with_string  );
    NEW_NATIVE_WORD( GET_WID_STR(WID(CR)),          NOT_IMMEDIATE, askf_word_cr                 );
    NEW_NATIVE_WORD( GET_WID_STR(WID(EMIT)),        NOT_IMMEDIATE, askf_word_emit               );

    NEW_NATIVE_WORD( GET_WID_STR(WID(LSHIFT)),      NOT_IMMEDIATE, askf_word_lshift             );
    NEW_NATIVE_WORD( GET_WID_STR(WID(RSHIFT)),      NOT_IMMEDIATE, askf_word_rshift             );
    NEW_NATIVE_WORD( GET_WID_STR(WID(XOR)),         NOT_IMMEDIATE, askf_word_xor                );
    NEW_NATIVE_WORD( GET_WID_STR(WID(AND)),         NOT_IMMEDIATE, askf_word_and                );
    NEW_NATIVE_WORD( GET_WID_STR(WID(ROT)),         NOT_IMMEDIATE, askf_word_rot                );
    NEW_NATIVE_WORD( GET_WID_STR(WID(OR)),          NOT_IMMEDIATE, askf_word_or                 );
    NEW_NATIVE_WORD( GET_WID_STR(WID(INVERT)),      NOT_IMMEDIATE, askf_word_invert             );
    NEW_NATIVE_WORD( GET_WID_STR(WID(0SP)),         NOT_IMMEDIATE, askf_word_clearstack         );
    NEW_NATIVE_WORD( GET_WID_STR(WID(TRUE)),        NOT_IMMEDIATE, askf_word_true               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(FALSE)),       NOT_IMMEDIATE, askf_word_false              );

    NEW_NATIVE_WORD( GET_WID_STR(WID(CELLS)),       NOT_IMMEDIATE, askf_word_cells              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(CELLS_ADD)),       NOT_IMMEDIATE, askf_word_cell_add           );
    NEW_NATIVE_WORD( GET_WID_STR(WID(CHARS)),       NOT_IMMEDIATE, askf_word_chars              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(CHARS_ADD)),       NOT_IMMEDIATE, askf_word_char_add           );
    NEW_NATIVE_WORD( GET_WID_STR(WID(FILL)),        NOT_IMMEDIATE, askf_word_fill               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(COPY)),        NOT_IMMEDIATE, askf_word_copy               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(COMMENT_PAREN)),            IS_IMMEDIATE, askf_word_comment_parenteshis );
    NEW_NATIVE_WORD( GET_WID_STR(WID(COMMENT_SLASH)),           IS_IMMEDIATE, askf_word_comment_slash      );

    // block words
    // TODO: remove native block words for forth equivalent implementation
    NEW_NATIVE_WORD( GET_WID_STR(WID(FLUSH)),       NOT_IMMEDIATE, askf_word_flush              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(LIST)),        NOT_IMMEDIATE, askf_word_list               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(LOADBLOCK)),        NOT_IMMEDIATE, askf_word_load               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(BLOCK)),       NOT_IMMEDIATE, askf_word_block              );
    NEW_NATIVE_WORD( GET_WID_STR(WID(BLOCK_SIZE)),  NOT_IMMEDIATE, askf_word_block_size         );
    NEW_NATIVE_WORD( GET_WID_STR(WID(LINE)),        NOT_IMMEDIATE, askf_word_line               );
    NEW_NATIVE_WORD( GET_WID_STR(WID(A)),           NOT_IMMEDIATE, askf_word_add_line_toblock   );
    NEW_NATIVE_WORD( GET_WID_STR(WID(MAX_LINES)),   NOT_IMMEDIATE, askf_word_max_lines          );
    // TODO: remove native include words for forth equivalent implementation
    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        NEW_NATIVE_WORD( GET_WID_STR(WID(INCLUDED)),        NOT_IMMEDIATE, askf_word_included   );
        NEW_NATIVE_WORD( GET_WID_STR(WID(INCLUDE)),         NOT_IMMEDIATE, askf_word_include    );
    #endif

    NEW_NATIVE_WORD( GET_WID_STR(WID(CREATE_WORD)),     NOT_IMMEDIATE, askf_word_create_word    );
    NEW_NATIVE_WORD( GET_WID_STR(WID(COLON)),               NOT_IMMEDIATE, askf_word_colon          );
    NEW_NATIVE_WORD( GET_WID_STR(WID(COLON_NONAME)),         NOT_IMMEDIATE, askf_word_colon_noname   );
    NEW_NATIVE_WORD( GET_WID_STR(WID(SEMICOLON)),                IS_IMMEDIATE, askf_word_semicolon      );
    NEW_NATIVE_WORD( GET_WID_STR(WID(IMMEDIATE)),        IS_IMMEDIATE, askf_word_immediate      );
    NEW_NATIVE_WORD( GET_WID_STR(WID(IS_IMMEDIATE)),      NOT_IMMEDIATE, askf_word_isimmediate    );
    NEW_NATIVE_WORD( GET_WID_STR(WID(OPTIMIZE)),         IS_IMMEDIATE, askf_word_optimize       );
    NEW_NATIVE_WORD( GET_WID_STR(WID(INLINE)),           IS_IMMEDIATE, askf_word_inline         );
    NEW_NATIVE_WORD( GET_WID_STR(WID(DICTIONARY)),      NOT_IMMEDIATE, askf_word_add_dic        );
    NEW_NATIVE_WORD( GET_WID_STR(WID(FORGET_DICT)),     NOT_IMMEDIATE, askf_word_forget_dic     );
    NEW_NATIVE_WORD( GET_WID_STR(WID(REMEMBER_DICT)),   NOT_IMMEDIATE, askf_word_remember_dic   );
    NEW_NATIVE_WORD( GET_WID_STR(WID(PROMOTE_DICT)),    NOT_IMMEDIATE, askf_word_promote_dic    );
    NEW_NATIVE_WORD( GET_WID_STR(WID(ABORT)),           NOT_IMMEDIATE, askf_word_abort          );
    NEW_NATIVE_WORD( GET_WID_STR(WID(EXIT)),             IS_IMMEDIATE, askf_word_exit           );
    NEW_NATIVE_WORD( GET_WID_STR(WID(BYE)),             NOT_IMMEDIATE, askf_word_bye            );
    NEW_NATIVE_WORD( GET_WID_STR(WID(ISCOMPTIME)),       NOT_IMMEDIATE, askf_word_iscomptime     );
    NEW_NATIVE_WORD( GET_WID_STR(WID(ISINTERPTIME)),     NOT_IMMEDIATE, askf_word_isinterptime   );
    NEW_NATIVE_WORD( GET_WID_STR(WID(STATE)),           NOT_IMMEDIATE, askf_word_state          );
    NEW_NATIVE_WORD( GET_WID_STR(WID(IS_WINDOWS)), NOT_IMMEDIATE, askf_word_is_sys_windows );
    NEW_NATIVE_WORD( GET_WID_STR(WID(IS_LINUX)),   NOT_IMMEDIATE, askf_word_is_sys_linux   );
    NEW_NATIVE_WORD( GET_WID_STR(WID(WHAT_ARQ_BITS)),       NOT_IMMEDIATE, askf_word_what_arq_bits  );
    
    // INFO: totally valid words to include on the core dic BUT im not sure if i want so
    UNUSED( askf_word_0branch );
    UNUSED( askf_word_branch  );
    
    NEW_NATIVE_WORD( GET_WID_STR(WID(IF)),          IS_IMMEDIATE, askf_word_if      );
    NEW_NATIVE_WORD( GET_WID_STR(WID(ELSE)),        IS_IMMEDIATE, askf_word_else    );
    NEW_NATIVE_WORD( GET_WID_STR(WID(THEN)),        IS_IMMEDIATE, askf_word_then    );
    NEW_NATIVE_WORD( GET_WID_STR(WID(BEGIN)),       IS_IMMEDIATE, askf_word_begin   );
    NEW_NATIVE_WORD( GET_WID_STR(WID(WHILE)),       IS_IMMEDIATE, askf_word_while   );
    NEW_NATIVE_WORD( GET_WID_STR(WID(REPEAT)),      IS_IMMEDIATE, askf_word_repeat  );

    NEW_NATIVE_WORD( GET_WID_STR(WID(BRACKET_OPEN)),           IS_IMMEDIATE, askf_word_bracket_open    );
    NEW_NATIVE_WORD( GET_WID_STR(WID(BRACKET_CLOSE)),           IS_IMMEDIATE, askf_word_bracket_close   );
    NEW_NATIVE_WORD( GET_WID_STR(WID(SINGLE_QUOTE)),           IS_IMMEDIATE, askf_word_single_quote    );
    NEW_NATIVE_WORD( GET_WID_STR(WID(LITERAL)),     IS_IMMEDIATE, askf_word_literal         );
    NEW_NATIVE_WORD( GET_WID_STR(WID(RLITERAL)),     IS_IMMEDIATE, askf_word_rliteral         );
    NEW_NATIVE_WORD( GET_WID_STR(WID(POSTPONE)),    IS_IMMEDIATE, askf_word_postpone        );
    NEW_NATIVE_WORD( GET_WID_STR(WID(EVALUATE)),   NOT_IMMEDIATE, askf_word_evaluate        );
    NEW_NATIVE_WORD( GET_WID_STR(WID(EXECUTE)),    NOT_IMMEDIATE, askf_word_execute         );
    NEW_NATIVE_WORD( GET_WID_STR(WID(COMPILE_COMMA)),   NOT_IMMEDIATE, askf_word_compile_comma   );
    NEW_NATIVE_WORD( GET_WID_STR(WID(PUSH_RSTACK)),         NOT_IMMEDIATE, askf_word_push_rstack     );
    NEW_NATIVE_WORD( GET_WID_STR(WID(POP_RSTACK)),         NOT_IMMEDIATE, askf_word_pop_rstack      );
    NEW_NATIVE_WORD( GET_WID_STR(WID(PEEK_RSTACK)),         NOT_IMMEDIATE, askf_word_peek_rstack     );
    NEW_NATIVE_WORD( GET_WID_STR(WID(SEE)),         IS_IMMEDIATE, askf_word_see             );

    NEW_NATIVE_WORD( GET_WID_STR(WID(STARTER)),     IS_IMMEDIATE, askf_word_starter         );
    NEW_NATIVE_WORD( GET_WID_STR(WID(PROGRAM)),     IS_IMMEDIATE, askf_word_program         );

    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        NEW_NATIVE_WORD( GET_WID_STR(WID(FOREIGN)),      IS_IMMEDIATE, askf_word_foreign        );
        NEW_NATIVE_WORD( GET_WID_STR(WID(DOTFOREIGN)),    NOT_IMMEDIATE, askf_word_dot_foreign    );
        NEW_NATIVE_WORD( GET_WID_STR(WID(FREEFOREIGN)), NOT_IMMEDIATE, askf_word_freeforeign    );
        NEW_NATIVE_WORD( GET_WID_STR(WID(FUNCTION)),    IS_IMMEDIATE, askf_word_function       );
    #endif
}

AskForth_NativeWordRelocation* askf_get_relocation_array( void ) {
    AskForth_NativeWordRelocation* arr = malloc(sizeof(AskForth_NativeWordRelocation) * (MAX_WID+1) );

    if ( arr == NULL )
        return arr;

    arr[WID(ALIGNED)].native_address              = (askf_addr_t)askf_word_aligned;
    arr[WID(ALIGN)].native_address                = (askf_addr_t)askf_word_align;
    arr[WID(DOT)].native_address                  = (askf_addr_t)askf_word_dot;
    arr[WID(SOURCE)].native_address               = (askf_addr_t)askf_word_input_source;
    arr[WID(SOURCEID)].native_address             = (askf_addr_t)askf_word_input_source_id;
    arr[WID(SOURCEIN)].native_address             = (askf_addr_t)askf_word_input_source_in;
    arr[WID(BLK)].native_address                  = (askf_addr_t)askf_word_input_source_blk;
    arr[WID(DEPTH)].native_address                = (askf_addr_t)askf_word_stack_depth;
    arr[WID(BASE)].native_address                 = (askf_addr_t)askf_word_base;
    arr[WID(DECIMAL)].native_address              = (askf_addr_t)askf_word_decimal;
    arr[WID(HEXADECIMAL)].native_address          = (askf_addr_t)askf_word_hexadecimal;
    arr[WID(OCTAL)].native_address                = (askf_addr_t)askf_word_octal;
    arr[WID(DOT_STACK)].native_address            = (askf_addr_t)askf_word_dot_stack;
    arr[WID(DUP)].native_address                  = (askf_addr_t)askf_word_dup;
    arr[WID(MAYBE_DUP)].native_address            = (askf_addr_t)askf_word_maybe_dup;
    arr[WID(2DUP)].native_address                 = (askf_addr_t)askf_word_2dup;
    arr[WID(SWAP)].native_address                 = (askf_addr_t)askf_word_swap;
    arr[WID(2SWAP)].native_address                = (askf_addr_t)askf_word_2swap;
    arr[WID(NIP)].native_address                  = (askf_addr_t)askf_word_nip;
    arr[WID(TUCK)].native_address                 = (askf_addr_t)askf_word_tuck;
    arr[WID(DROP)].native_address                 = (askf_addr_t)askf_word_drop;
    arr[WID(2DROP)].native_address                = (askf_addr_t)askf_word_2drop;
    arr[WID(OVER)].native_address                 = (askf_addr_t)askf_word_over;
    arr[WID(2OVER)].native_address                = (askf_addr_t)askf_word_2over;
    arr[WID(NEGATE)].native_address               = (askf_addr_t)askf_word_negate;
    arr[WID(LIB)].native_address                  = (askf_addr_t)askf_word_lib;
    arr[WID(FROM)].native_address                 = (askf_addr_t)askf_word_from;
    arr[WID(FIND)].native_address                 = (askf_addr_t)askf_word_find;
    arr[WID(SFIND)].native_address                = (askf_addr_t)askf_word_sfind;
    arr[WID(PARSE_NAME)].native_address           = (askf_addr_t)askf_word_parse_name;
    arr[WID(FORGET)].native_address               = (askf_addr_t)askf_word_forget;
    arr[WID(XTFORGET)].native_address             = (askf_addr_t)askf_word_xtforget;
    arr[WID(WORDS)].native_address                = (askf_addr_t)askf_word_words;
    arr[WID(ADD)].native_address                  = (askf_addr_t)askf_word_add;
    arr[WID(MINUS)].native_address                = (askf_addr_t)askf_word_minus;
    arr[WID(MULTIPLY)].native_address             = (askf_addr_t)askf_word_multiply;
    arr[WID(SLASHMOD)].native_address             = (askf_addr_t)askf_word_slashmod;
    arr[WID(EQUALS)].native_address               = (askf_addr_t)askf_word_equals;
    arr[WID(EQUALS_ZERO)].native_address          = (askf_addr_t)askf_word_equals_zero;
    arr[WID(LESS_THAN)].native_address            = (askf_addr_t)askf_word_less_than;
    arr[WID(LESS_THAN_OR_EQUAL)].native_address   = (askf_addr_t)askf_word_less_than_or_equal;
    arr[WID(LESS_THAN_ZERO)].native_address       = (askf_addr_t)askf_word_less_than_zero;
    arr[WID(MORE_THAN)].native_address            = (askf_addr_t)askf_word_more_than;
    arr[WID(MORE_THAN_OR_EQUAL)].native_address   = (askf_addr_t)askf_word_more_than_or_equal;
    arr[WID(MORE_THAN_ZERO)].native_address       = (askf_addr_t)askf_word_more_than_zero;
    arr[WID(NOT_EQUAL)].native_address            = (askf_addr_t)askf_word_not_equal;
    arr[WID(NOT_EQUAL_ZERO)].native_address       = (askf_addr_t)askf_word_not_equal_zero;
    arr[WID(SIGNED)].native_address               = (askf_addr_t)askf_word_make_stack_signed;
    arr[WID(UNSIGNED)].native_address             = (askf_addr_t)askf_word_make_stack_unsigned;
    arr[WID(BITS)].native_address                 = (askf_addr_t)askf_word_bits;
    arr[WID(WHATBITS)].native_address             = (askf_addr_t)askf_word_whatbits;
    arr[WID(TYPE)].native_address                 = (askf_addr_t)askf_word_type;
    arr[WID(STORE)].native_address                = (askf_addr_t)askf_word_store;
    arr[WID(BYTE_STORE)].native_address           = (askf_addr_t)askf_word_byte_store;
    arr[WID(LOAD)].native_address                 = (askf_addr_t)askf_word_load_ptr;
    arr[WID(BYTE_LOAD)].native_address            = (askf_addr_t)askf_word_load_byte_ptr;
    arr[WID(HERE)].native_address                 = (askf_addr_t)askf_word_here;
    arr[WID(ALLOT)].native_address                = (askf_addr_t)askf_word_allot;
    arr[WID(UNUSED)].native_address               = (askf_addr_t)askf_word_unused;
    arr[WID(PRINT_STRING)].native_address         = (askf_addr_t)askf_word_print_string;
    arr[WID(STORE_STRING)].native_address         = (askf_addr_t)askf_word_store_string;
    arr[WID(ERROR_WITH_STRING)].native_address    = (askf_addr_t)askf_word_error_with_string;
    arr[WID(CR)].native_address                   = (askf_addr_t)askf_word_cr;
    arr[WID(EMIT)].native_address                 = (askf_addr_t)askf_word_emit;
    arr[WID(LSHIFT)].native_address               = (askf_addr_t)askf_word_lshift;
    arr[WID(RSHIFT)].native_address               = (askf_addr_t)askf_word_rshift;
    arr[WID(XOR)].native_address                  = (askf_addr_t)askf_word_xor;
    arr[WID(AND)].native_address                  = (askf_addr_t)askf_word_and;
    arr[WID(ROT)].native_address                  = (askf_addr_t)askf_word_rot;
    arr[WID(OR)].native_address                   = (askf_addr_t)askf_word_or;
    arr[WID(INVERT)].native_address               = (askf_addr_t)askf_word_invert;
    arr[WID(0SP)].native_address                  = (askf_addr_t)askf_word_clearstack;
    arr[WID(TRUE)].native_address                 = (askf_addr_t)askf_word_true;
    arr[WID(FALSE)].native_address                = (askf_addr_t)askf_word_false;
    arr[WID(CELLS)].native_address                = (askf_addr_t)askf_word_cells;
    arr[WID(CELLS_ADD)].native_address            = (askf_addr_t)askf_word_cell_add;
    arr[WID(CHARS)].native_address                = (askf_addr_t)askf_word_chars;
    arr[WID(CHARS_ADD)].native_address            = (askf_addr_t)askf_word_char_add;
    arr[WID(FILL)].native_address                 = (askf_addr_t)askf_word_fill;
    arr[WID(COPY)].native_address                 = (askf_addr_t)askf_word_copy;
    arr[WID(COMMENT_PAREN)].native_address        = (askf_addr_t)askf_word_comment_parenteshis;
    arr[WID(COMMENT_SLASH)].native_address        = (askf_addr_t)askf_word_comment_slash;
    arr[WID(FLUSH)].native_address                = (askf_addr_t)askf_word_flush;
    arr[WID(LIST)].native_address                 = (askf_addr_t)askf_word_list;
    arr[WID(LOADBLOCK)].native_address            = (askf_addr_t)askf_word_load;
    arr[WID(BLOCK)].native_address                = (askf_addr_t)askf_word_block;
    arr[WID(BLOCK_SIZE)].native_address           = (askf_addr_t)askf_word_block_size;
    arr[WID(LINE)].native_address                 = (askf_addr_t)askf_word_line;
    arr[WID(A)].native_address                    = (askf_addr_t)askf_word_add_line_toblock;
    arr[WID(MAX_LINES)].native_address            = (askf_addr_t)askf_word_max_lines;

    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        arr[WID(INCLUDED)].native_address         = (askf_addr_t)askf_word_included;
        arr[WID(INCLUDE)].native_address          = (askf_addr_t)askf_word_include;
    #endif
    arr[WID(CREATE_WORD)].native_address          = (askf_addr_t)askf_word_create_word;
    arr[WID(COLON)].native_address                = (askf_addr_t)askf_word_colon;
    arr[WID(COLON_NONAME)].native_address         = (askf_addr_t)askf_word_colon_noname;
    arr[WID(SEMICOLON)].native_address            = (askf_addr_t)askf_word_semicolon;
    arr[WID(IMMEDIATE)].native_address            = (askf_addr_t)askf_word_immediate;
    arr[WID(IS_IMMEDIATE)].native_address         = (askf_addr_t)askf_word_isimmediate;
    arr[WID(OPTIMIZE)].native_address             = (askf_addr_t)askf_word_optimize;
    arr[WID(INLINE)].native_address               = (askf_addr_t)askf_word_inline;
    arr[WID(DICTIONARY)].native_address           = (askf_addr_t)askf_word_add_dic;
    arr[WID(FORGET_DICT)].native_address          = (askf_addr_t)askf_word_forget_dic;
    arr[WID(REMEMBER_DICT)].native_address        = (askf_addr_t)askf_word_remember_dic;
    arr[WID(PROMOTE_DICT)].native_address         = (askf_addr_t)askf_word_promote_dic;
    arr[WID(ABORT)].native_address                = (askf_addr_t)askf_word_abort;
    arr[WID(EXIT)].native_address                 = (askf_addr_t)askf_word_exit;
    arr[WID(BYE)].native_address                  = (askf_addr_t)askf_word_bye;
    arr[WID(ISCOMPTIME)].native_address           = (askf_addr_t)askf_word_iscomptime;
    arr[WID(ISINTERPTIME)].native_address         = (askf_addr_t)askf_word_isinterptime;
    arr[WID(STATE)].native_address                = (askf_addr_t)askf_word_state;
    arr[WID(IS_WINDOWS)].native_address           = (askf_addr_t)askf_word_is_sys_windows;
    arr[WID(IS_LINUX)].native_address             = (askf_addr_t)askf_word_is_sys_linux;
    arr[WID(WHAT_ARQ_BITS)].native_address        = (askf_addr_t)askf_word_what_arq_bits;
    arr[WID(LITERAL)].native_address              = (askf_addr_t)askf_word_literal;
    arr[WID(REFILL)].native_address               = (askf_addr_t)askf_word_refill;
    arr[WID(IF)].native_address                   = (askf_addr_t)askf_word_if;
    arr[WID(ELSE)].native_address                 = (askf_addr_t)askf_word_else;
    arr[WID(THEN)].native_address                 = (askf_addr_t)askf_word_then;
    arr[WID(BEGIN)].native_address                = (askf_addr_t)askf_word_begin;
    arr[WID(WHILE)].native_address                = (askf_addr_t)askf_word_while;
    arr[WID(REPEAT)].native_address               = (askf_addr_t)askf_word_repeat;
    arr[WID(BRACKET_OPEN)].native_address         = (askf_addr_t)askf_word_bracket_open;
    arr[WID(BRACKET_CLOSE)].native_address        = (askf_addr_t)askf_word_bracket_close;
    arr[WID(SINGLE_QUOTE)].native_address         = (askf_addr_t)askf_word_single_quote;
    arr[WID(LITERAL)].native_address              = (askf_addr_t)askf_word_literal;
    arr[WID(RLITERAL)].native_address             = (askf_addr_t)askf_word_rliteral;
    arr[WID(POSTPONE)].native_address             = (askf_addr_t)askf_word_postpone;
    arr[WID(EVALUATE)].native_address             = (askf_addr_t)askf_word_evaluate;
    arr[WID(EXECUTE)].native_address              = (askf_addr_t)askf_word_execute;
    arr[WID(COMPILE_COMMA)].native_address        = (askf_addr_t)askf_word_compile_comma;
    arr[WID(PUSH_RSTACK)].native_address          = (askf_addr_t)askf_word_push_rstack;
    arr[WID(POP_RSTACK)].native_address           = (askf_addr_t)askf_word_pop_rstack;
    arr[WID(PEEK_RSTACK)].native_address          = (askf_addr_t)askf_word_peek_rstack;
    arr[WID(SEE)].native_address                  = (askf_addr_t)askf_word_see;
    arr[WID(STARTER)].native_address              = (askf_addr_t)askf_word_starter;
    arr[WID(PROGRAM)].native_address              = (askf_addr_t)askf_word_program;

    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        arr[WID(FOREIGN)].native_address          = (askf_addr_t)askf_word_foreign;
        arr[WID(DOTFOREIGN)].native_address       = (askf_addr_t)askf_word_dot_foreign;
        arr[WID(FREEFOREIGN)].native_address      = (askf_addr_t)askf_word_freeforeign;
        arr[WID(FUNCTION)].native_address         = (askf_addr_t)askf_word_function;
    #endif

    return arr;
}
void askf_free_relocation_array( AskForth_NativeWordRelocation* array ) {
    free( array );
}
