#include "askforth_words.h"
#include "../library/library.h"

#include "../input/input.h"
#include "../stack/stack.h"
#include "../memory/blocks.h"

#ifdef TARGET_LINUX
    #include <stdio.h>
#endif

static void _askf_word_failed( ascii* msg, u64 len ) {
    AskForthError err = {0};
    err.error = ASKF_ERROR_WORD_FAILED;
    err.zone  = ASKF_ERROR_ZONE_INNER;

    AskForthErrorMessage* opt_msg = 
    askf_alloc_new_opt_message( msg, len );
    err.opt_message = opt_msg;
    askf_throw_error( err );
}

static void askf_word_dot( void ) {
    AskForthVm* vm = askf_get_global_vm();

    AskForth_Cell cell = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    u32 res = askf_stack_pop( &cell, vm->stack );

    // TODO: throw error stack underflow
    if ( !res ) {
        _askf_word_failed( (ascii*)". -> Stack Empty", 16 );
        return;
    }

    askf_print_cell(&cell);
    askf_print( (ascii*)" ", 1 );
}

static void askf_word_stack_depth( void ) {
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

static void askf_word_dot_stack ( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u8 bits = vm->stack->cell_scale;
    cell.val._8u = bits;
    askf_print_cell( &cell );
    askf_print( (ascii*)" BITS ", 6 );

    u8 depth            = vm->stack->index;
    cell.val._8u        = depth;

    if ( vm->stack->is_signed )
        askf_print( (ascii*)"S ", 2 );
    else
        askf_print( (ascii*)"U ", 2 );

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

static void askf_word_dup ( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &cell, vm->stack );

    if ( !res ) {
        _askf_word_failed( ( ascii* )"dup -> Stack Empty" , 18);
        return;
    }
    askf_stack_push( &cell, vm->stack );
    askf_stack_push( &cell, vm->stack );
}

static void askf_word_2dup ( void ) {
    AskForthVm* vm      = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( ( ascii* )"2dup -> Expects ( a b - )" , 25 );
        return;
    }

    AskForth_Cell a = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell b = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &b, vm->stack );
    askf_stack_pop( &a, vm->stack );

    askf_stack_push( &a, vm->stack );
    askf_stack_push( &b, vm->stack );
    askf_stack_push( &a, vm->stack );
    askf_stack_push( &b, vm->stack );
}

static void askf_word_swap( void ) {
    AskForthVm* vm          = askf_get_global_vm();
    AskForth_Cell cell_ts   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_ss   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res1 = askf_stack_pop( &cell_ts, vm->stack );

    if ( !res1 ) {
        _askf_word_failed( ( ascii* )"swap -> Stack Empty" , 18);
    }

    u32 res2 = askf_stack_pop( &cell_ss, vm->stack );

    if ( !res2 ) {
        _askf_word_failed( ( ascii* )"swap -> Stack misses 2nd value" , 30);
    }

    askf_stack_push( &cell_ts, vm->stack );
    askf_stack_push( &cell_ss, vm->stack );
}

static void askf_word_2swap( void ) {
    AskForthVm* vm          = askf_get_global_vm();

    if ( vm->stack->index < 4 ) {
        _askf_word_failed( ( ascii* )"2swap -> Expects ( a b c d - )" , 30 );
        return;
    }

    AskForth_Cell cell_a   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_b   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_c   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_d   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &cell_d, vm->stack );
    askf_stack_pop( &cell_c, vm->stack );
    askf_stack_pop( &cell_b, vm->stack );
    askf_stack_pop( &cell_a, vm->stack );

    askf_stack_push( &cell_c, vm->stack );
    askf_stack_push( &cell_d, vm->stack );
    askf_stack_push( &cell_a, vm->stack );
    askf_stack_push( &cell_b, vm->stack );
}

static void askf_word_rot ( void ) {
    AskForthVm* vm      = askf_get_global_vm();

    if ( vm->stack->index < 3 ) {
        _askf_word_failed( ( ascii* )"rot -> Expects ( a b c - )" , 27 );
        return;
    }

    AskForth_Cell a  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell b  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell c  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &c, vm->stack );
    askf_stack_pop( &b, vm->stack );
    askf_stack_pop( &a, vm->stack );

    askf_stack_push( &b, vm->stack );
    askf_stack_push( &c, vm->stack );
    askf_stack_push( &a, vm->stack );
}

static void askf_word_nip ( void ) {
    AskForthVm* vm      = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( ( ascii* )"nip -> Expects ( a b - )" , 24 );
        return;
    }

    AskForth_Cell a  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell b  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &b, vm->stack );
    askf_stack_pop( &a, vm->stack );

    askf_stack_push( &b, vm->stack );
}

static void askf_word_tuck ( void ) {
    AskForthVm* vm      = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( ( ascii* )"nip -> Expects ( a b - )" , 24 );
        return;
    }

    AskForth_Cell a  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell b  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &b, vm->stack );
    askf_stack_pop( &a, vm->stack );

    askf_stack_push( &b, vm->stack );
    askf_stack_push( &a, vm->stack );
    askf_stack_push( &b, vm->stack );
}

static void askf_word_drop( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &cell, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"drop -> Empty Stack", 19 );
    }
}

static void askf_word_2drop( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"2drop -> Expects ( a b - )", 26 );
        return;
    }

    askf_stack_pop( &cell, vm->stack );
    askf_stack_pop( &cell, vm->stack );
}

static void askf_word_over( void ) {
    AskForthVm* vm          = askf_get_global_vm();
    AskForth_Cell cell_ts   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_ss   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res1 = askf_stack_pop( &cell_ts, vm->stack );

    if ( !res1 ) {
        _askf_word_failed( (ascii*)"over -> Empty Stack", 19 );
    }

    u32 res2 = askf_stack_pop( &cell_ss, vm->stack );

    if ( !res2 ) {
        _askf_word_failed( (ascii*)"over -> 2nd value expected on stack", 35 );
    }

    askf_stack_push( &cell_ss, vm->stack );
    askf_stack_push( &cell_ts, vm->stack );
    askf_stack_push( &cell_ss, vm->stack );
}

static void askf_word_2over( void ) {
    AskForthVm* vm          = askf_get_global_vm();
    if ( vm->stack->index < 4 ) {
        _askf_word_failed( (ascii*)"2over -> Expects ( a b c d - )", 30 );
        return;
    }
    
    AskForth_Cell cell_a   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_b   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_c   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_d   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &cell_a, vm->stack );
    askf_stack_pop( &cell_b, vm->stack );
    askf_stack_pop( &cell_c, vm->stack );
    askf_stack_pop( &cell_d, vm->stack );


    askf_stack_push( &cell_d, vm->stack );
    askf_stack_push( &cell_c, vm->stack );
    askf_stack_push( &cell_b, vm->stack );
    askf_stack_push( &cell_a, vm->stack );
    askf_stack_push( &cell_d, vm->stack );
    askf_stack_push( &cell_c, vm->stack );
}

static void askf_word_negate( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &cell, vm->stack );
    if ( !res ) {
        _askf_word_failed( (ascii*)"negate -> Empty Stack", 21 );
        return;
    }

    cell.val._64s = 0 - cell.val._64u;
    askf_stack_push( &cell, vm->stack );
}

static void askf_word_lib( void ) {
    AskForthVm* vm              = askf_get_global_vm();
    AskForth_Library* lib       = (AskForth_Library*)vm->lib;

    AskForth_Dictionary* base   = lib->dictionaries_base;
    askf_print( (ascii*)"Dictionaries: ", 14 );

    while ( base ) {
        askf_print( base->name, base->name_len );
        askf_print( (ascii*)", ", 2 );

        base = base->next;
    }
}

static void askf_word_parse_word( void ) {
    AskForthVm* vm               = askf_get_global_vm();
    AskForthTokenizer* tokenizer = NULL;

    switch ( vm->parse_type ) {
        case ASKF_MAIN_PARSER:
            tokenizer = vm->tokenizer;
            break;
        case ASKF_X_PARSER:
            tokenizer = vm->tokenizer_x;
            break;
        default:
            return;
            break;
    }

    if ( tokenizer->ctx.idx + 1 >= tokenizer->index ) {
        _askf_word_failed( (ascii*)"PARSE-WORD -> No token found", 28 );
        return;
    }

    tokenizer->ctx.idx += 1;
    u64 idx = tokenizer->ctx.idx;

    AskForth_Cell cell      = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    // TODO: decide if this is what i want from parse_word
    AskForthToken* scratch  = askf_alloc( sizeof( ascii ) * tokenizer->tokens[idx].length );

    COPY( tokenizer->tokens[idx].base, scratch, tokenizer->tokens[idx].length );

    cell.val._64u = (u64)scratch;
    askf_stack_push( &cell, vm->stack );
    cell.val._64u = tokenizer->tokens[idx].length;
    askf_stack_push( &cell, vm->stack );
}

static void askf_word_words( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    askf_word_parse_word();

    if ( vm->outer_state != ASKF_VM_OUTER_STATE_EXECUTE ) {
        return;
    }

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"words -> Expects (addr len - )", 30 );
        return;
    }

    AskForth_Cell len    = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell addr   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &len, vm->stack );
    askf_stack_pop( &addr, vm->stack );
    
    AskForthToken tkn           = {0};
    tkn.base                    = (ascii*)addr.val._64u;
    tkn.length                  = len.val._64u;
    AskForth_Dictionary* dic    = askf_library_find_dic( vm, &tkn );

    if ( !dic ) {
        _askf_word_failed( (ascii*)"words -> dictionary not found" , 29 );
        return;
    }

    AskForth_Word* base = dic->words_base;

    if ( !base ) {
        _askf_word_failed( (ascii*)"words -> empty dictionary" , 25 );
        return;
    }

    askf_print( dic->name, dic->name_len );
    askf_print( (ascii*)" dictionary words: \n", 20 );

    while ( base ) {
        askf_print( base->name, base->name_len );
        askf_print( (ascii*)" ", 1 );
        base = base->next;
    }
}

static void askf_word_add( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"+ -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    top_stack.val._64u = below_top__stack.val._64u + top_stack.val._64u;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_minus( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"- -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    top_stack.val._64u = below_top__stack.val._64u - top_stack.val._64u;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_multiply( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"* -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    top_stack.val._64u = below_top__stack.val._64u * top_stack.val._64u;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_slashmod( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"/mod -> Expects 2 values on stack" , 33 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    u64 slash   = below_top__stack.val._64u / top_stack.val._64u;
    u64 mod     = below_top__stack.val._64u % top_stack.val._64u;

    top_stack.val._64u = slash;
    askf_stack_push( &top_stack, vm->stack );
    top_stack.val._64u = mod;
    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_equals( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"= -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    u64 val = below_top__stack.val._64u == top_stack.val._64u;

    top_stack.val._64u = val;
    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_equals_zero( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0= -> Expects 1 values on stack" , 31 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    u64 val = 0 == top_stack.val._64u;

    top_stack.val._64u = val;
    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_less_than( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"< -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    u64 val = 0;
    if ( top_stack.is_signed ) 
        val = below_top__stack.val._64s < top_stack.val._64s;
    else
        val = below_top__stack.val._64u < top_stack.val._64u;

    top_stack.val._64u = val;
    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_less_than_or_equal( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"< -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    u64 val = 0;
    if ( top_stack.is_signed ) 
        val = below_top__stack.val._64s <= top_stack.val._64s;
    else
        val = below_top__stack.val._64u <= top_stack.val._64u;

    top_stack.val._64u = val;
    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_less_than_zero( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0< -> Expects 1 values on stack" , 31 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    u64 val = 0;
    if ( top_stack.is_signed )
        val = 0 < top_stack.val._64s;
    else
        val = 0 < top_stack.val._64u;

    top_stack.val._64u = val;
    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_more_than( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"> -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    u64 val = 0;
    
    if ( top_stack.is_signed )
        val = below_top__stack.val._64s > top_stack.val._64s;
    else
        val = below_top__stack.val._64u > top_stack.val._64u;

    top_stack.val._64u = val;
    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_more_than_or_equal( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"> -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    u64 val = 0;
    
    if ( top_stack.is_signed )
        val = below_top__stack.val._64s >= top_stack.val._64s;
    else
        val = below_top__stack.val._64u >= top_stack.val._64u;

    top_stack.val._64u = val;
    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_more_than_zero( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0> -> Expects 1 values on stack" , 31 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    u64 val = 0;

    if ( top_stack.is_signed )
         val = 0 > top_stack.val._64s;
    else
         val = 0 > top_stack.val._64u;

    top_stack.val._64u = val;
    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_not_equal( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"<> -> Expects 2 values on stack" , 31 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    u64 val = below_top__stack.val._64u != top_stack.val._64u;

    top_stack.val._64u = val;
    askf_stack_push( &top_stack, vm->stack );
}

static void  askf_word_not_equal_zero( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0<> -> Expects 1 values on stack" , 32 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    u64 val = 0 != top_stack.val._64u;

    top_stack.val._64u = val;
    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_make_stack_signed( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    vm->stack->is_signed = TRUE;
}

static void askf_word_make_stack_unsigned( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    vm->stack->is_signed = FALSE;
}

static void askf_word_bits( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell cell   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &cell, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)" BITS -> Empty Stack", 21 );
        return;
    }

    AskForth_CellSize new_cell_size = {0};

    switch ( cell.val._64u ) {
        case ASKF_BITS8:
        case ASKF_BITS16:
        case ASKF_BITS32:
        case ASKF_BITS64:
            new_cell_size = cell.val._64u;
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

static void askf_word_type( void ){
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"TYPE -> Expects ( addr len - )", 30);
        return;
    }

    AskForth_Cell len   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell addr  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &len, vm->stack );
    askf_stack_pop( &addr, vm->stack );

    askf_print( ( ascii* )addr.val._64u, len.val._32u );
    askf_print( ( ascii* )" ", 1 );
}

static void askf_word_store( void ){
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"! -> Expects ( val addr - )", 27);
        return;
    }
    AskForth_Cell val   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell addr  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &addr, vm->stack );
    askf_stack_pop( &val, vm->stack );

    u64* ptr = ( u64* )addr.val._64u;
    *ptr = val.val._64u;
}

static void askf_word_byte_store( void ){
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"! -> Expects ( val addr - )", 27);
        return;
    }
    AskForth_Cell val   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell addr  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &addr, vm->stack );
    askf_stack_pop( &val, vm->stack );

    u8* ptr = ( u8* )addr.val._64u;
    *ptr = val.val._8u;
}

static void askf_word_load_ptr( void ){
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"@ -> Expects ( addr - )", 27);
        return;
    }
    AskForth_Cell addr  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &addr, vm->stack );

    u64* ptr = ( u64* )addr.val._64u;

    addr.val._64u = *ptr;

    askf_stack_push( &addr, vm->stack );
}

static void askf_word_load_byte_ptr( void ){
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"c@ -> Expects ( addr - )", 27);
        return;
    }
    AskForth_Cell addr  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &addr, vm->stack );

    u8* ptr = ( u8* )addr.val._64u;

    addr.val._8u = *ptr;

    askf_stack_push( &addr, vm->stack );
}

static void askf_word_here( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell addr  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    *addr.cell_scale = ASKF_BITS64;

    addr.val._64u = ( u64 )( (( u8* )vm->ram->start_ptr ) + vm->ram->byte_index );

    askf_stack_push( &addr, vm->stack );
}

static void askf_word_allot( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell val  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"ALLOT -> Expects ( bytes - )", 26 );
        return;
    }

    vm->ram->byte_index += val.val._64u;
}

static void askf_word_cells( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell val  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"cells -> Expects ( n - )", 24 );
        return;
    }

    val.val._64u *= sizeof( val.val._64u );

    askf_stack_push( &val, vm->stack );
}

static void askf_word_cell_add( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell val  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"cell+ -> Expects ( n - )", 24 );
        return;
    }

    val.val._64u += sizeof( val.val._64u );

    askf_stack_push( &val, vm->stack );
}

static void askf_word_chars( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell val  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"chars -> Expects ( n - )", 24 );
        return;
    }

    val.val._64u *= sizeof( val.val._8u );

    askf_stack_push( &val, vm->stack );
}

static void askf_word_char_add( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell val  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"char+ -> Expects ( n - )", 24 );
        return;
    }

    val.val._64u += sizeof( val.val._8u );

    askf_stack_push( &val, vm->stack );
}

static void askf_word_print_string( void ) {
    AskForthVm* vm = askf_get_global_vm();

    AskForthTokenizer* tokenizer = NULL;

    switch ( vm->parse_type ) {
        case ASKF_MAIN_PARSER:
            tokenizer = vm->tokenizer;
            break;
        case ASKF_X_PARSER:
            tokenizer = vm->tokenizer_x;
            break;
        default:
            return;
    }

    u64 ctx_idx = tokenizer->ctx.idx;

    ctx_idx++;
    ascii*  string_base     = tokenizer->tokens[ctx_idx].base;
    u64     len             = 0;
    boolean got_terminator  = FALSE;

    while ( ctx_idx < tokenizer->index ) {
        AskForthToken* tkn = &tokenizer->tokens[ctx_idx];
        if ( tkn->base[tkn->length - 1] == '"' ) {
            got_terminator = TRUE;
            len = (u64)( tkn->base + tkn->length ) - (u64)string_base;
            tkn->base[tkn->length - 1] = '\0';
            len -= 1;
            break;
        } 

        ctx_idx++;
    }

    tokenizer->ctx.idx = ctx_idx;

    if ( !got_terminator ) {
        _askf_word_failed( (ascii*)".\" -> Terminator not found on input buffer", 42 );
        return;
    }

    askf_print( string_base, len );
    askf_print( (ascii*)" ", 1 );
}

static void askf_word_store_string( void ) {
    AskForthVm* vm = askf_get_global_vm();

    AskForthTokenizer* tokenizer = NULL;

    switch ( vm->parse_type ) {
        case ASKF_MAIN_PARSER:
            tokenizer = vm->tokenizer;
            break;
        case ASKF_X_PARSER:
            tokenizer = vm->tokenizer_x;
            break;
        default:
            return;
    }

    u64 ctx_idx = tokenizer->ctx.idx;

    ctx_idx++;
    ascii*  string_base     = tokenizer->tokens[ctx_idx].base;
    u64     len             = 0;
    boolean got_terminator  = FALSE;

    while ( ctx_idx < tokenizer->index ) {
        AskForthToken* tkn = &tokenizer->tokens[ctx_idx];
        if ( tkn->base[tkn->length - 1] == '"' ) {
            got_terminator = TRUE;
            len = (u64)( tkn->base + tkn->length ) - (u64)string_base;
            tkn->base[tkn->length - 1] = '\0';
            len -= 1;
            break;
        } 

        ctx_idx++;
    }

    tokenizer->ctx.idx = ctx_idx;

    if ( !got_terminator ) {
        _askf_word_failed( (ascii*)".\" -> Terminator not found on input buffer", 42 );
        return;
    }

    ascii* new_base = askf_alloc( sizeof(ascii) * len );

    COPY( string_base, new_base, len );

    AskForth_Cell cell_addr = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell cell_len  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    cell_addr.val._64u = (u64) new_base;
    cell_len.val._64u  = len;

    askf_stack_push( &cell_addr, vm->stack );
    askf_stack_push( &cell_len, vm->stack );
}

static void askf_word_comment_parenteshis( void ) {
    AskForthVm* vm = askf_get_global_vm();

    AskForthTokenizer* tokenizer = NULL;

    switch ( vm->parse_type ) {
        case ASKF_MAIN_PARSER:
            tokenizer = vm->tokenizer;
            break;
        case ASKF_X_PARSER:
            tokenizer = vm->tokenizer_x;
            break;
        default:
            return;
    }

    u64 ctx_idx = tokenizer->ctx.idx;

    ctx_idx++;
    boolean got_terminator  = FALSE;

    while ( ctx_idx < tokenizer->index ) {
        AskForthToken* tkn = &tokenizer->tokens[ctx_idx];
        if ( tkn->base[tkn->length - 1] == ')' ) {
            got_terminator = TRUE;
            break;
        } 

        ctx_idx++;
    }

    tokenizer->ctx.idx = ctx_idx;

    if ( !got_terminator ) {
        _askf_word_failed( (ascii*)"( -> Terminator not found on input buffer ')'", 45 );
        return;
    }
}

static void askf_word_comment_slash( void ) {
    AskForthVm* vm = askf_get_global_vm();

    AskForthTokenizer* tokenizer = NULL;

    switch ( vm->parse_type ) {
        case ASKF_MAIN_PARSER:
            tokenizer = vm->tokenizer;
            break;
        case ASKF_X_PARSER:
            tokenizer = vm->tokenizer_x;
            break;
        default:
            return;
    }

    u64 ctx_idx = tokenizer->ctx.idx;

    ctx_idx++;
    boolean got_terminator  = FALSE;

    while ( ctx_idx < tokenizer->index ) {
        AskForthToken* tkn = &tokenizer->tokens[ctx_idx];
        if ( tkn->line_end == TRUE ) {
            got_terminator = TRUE;
            break;
        } 

        ctx_idx++;
    }

    tokenizer->ctx.idx = ctx_idx;

    if ( !got_terminator ) {
        _askf_word_failed( (ascii*)"\\ -> Terminator not found on input buffer '\\n'", 46 );
        return;
    }
}

static void askf_word_cr( void ) { 
    askf_print( (ascii*)"\n", 1 );
}

static void askf_word_emit( void ) { 
    AskForthVm* vm = askf_get_global_vm();

    AskForth_Cell ascii_char = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    u32 res = askf_stack_pop( &ascii_char, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"EMIT -> Expects ( ascii_char - )", 32 );
        return;
    }

    askf_print( &ascii_char.val._8u, 1 );
}

static void askf_word_lshift( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"lshift -> Expects 2 values on stack" , 35 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    top_stack.val._64u = below_top__stack.val._64u << top_stack.val._64u;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_rshift( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"rshift -> Expects 2 values on stack" , 35 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    top_stack.val._64u = below_top__stack.val._64u >> top_stack.val._64u;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_xor( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"xor -> Expects 2 values on stack" , 32 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    top_stack.val._64u = below_top__stack.val._64u ^ top_stack.val._64u;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_and( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"and -> Expects 2 values on stack" , 32 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    top_stack.val._64u = below_top__stack.val._64u & top_stack.val._64u;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_or( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"or -> Expects ( a b - )" , 23 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    AskForth_Cell below_top__stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &below_top__stack, vm->stack );

    top_stack.val._64u = below_top__stack.val._64u | top_stack.val._64u;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_invert( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"invert -> Expects ( n - )" , 25 );
        return;
    }

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &top_stack, vm->stack );

    top_stack.val._64u = ~top_stack.val._64u;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_clearstack( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    vm->stack->index = 0;
}

static void askf_word_true( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    top_stack.val._64u = 1;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_false( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell top_stack = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    top_stack.val._64u = 0;

    askf_stack_push( &top_stack, vm->stack );
}

static void askf_word_fill( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 3 ) {
        _askf_word_failed( (ascii*)"FILL -> Expects ( addr n char - )", 33 );
    }

    AskForth_Cell _char = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell bytes = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell addr  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &_char, vm->stack );
    askf_stack_pop( &bytes, vm->stack );
    askf_stack_pop( &addr , vm->stack );

    FILL( ((u8*)addr.val._64u), _char.val._64u , bytes.val._64u );
}

static void askf_word_copy( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 3 ) {
        _askf_word_failed( (ascii*)"MOVE -> Expects ( old_addr new_addr n_bytes - )", 47 );
    }

    AskForth_Cell bytes     = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell new_addr  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell old_addr  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &bytes, vm->stack );
    askf_stack_pop( &new_addr, vm->stack );
    askf_stack_pop( &old_addr, vm->stack );

    COPY( ((u8*)old_addr.val._64u) , ((u8*)new_addr.val._64u), bytes.val._64u );
}

static void askf_word_flush( void ) { 
    askf_blocks_update();

    if ( askf_blocks_update() != 0 ) {
        _askf_word_failed( (ascii*)"FLUSH -> Failed to update BLOCKS", 32 );
    };
}

static void askf_word_list( void ) { 
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"LIST -> Expects ( n - )", 23);
        return;
    }
    AskForth_Cell block_id = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &block_id, vm->stack );

    ascii* start_block = (ascii*)
        ( vm->blocks->start_blocks + ( vm->blocks->block_size * block_id.val._64u ));

    u64 max_lines      = 16;
    u64 max_line_chars = vm->blocks->block_size / max_lines;
    AskForth_Cell cell = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    cell.is_signed     = FALSE;

    for ( u64 x = 0; x < max_lines; x++) {
        if ( x < 10 )
            askf_print( (ascii*)"  ", 2);
        else if ( x < 100 )
            askf_print( (ascii*)" ", 1);

        cell.val._64u = x;
        askf_print_cell( &cell );
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

static void askf_word_block( void ) { 
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"BLOCK -> Expects ( n - )", 24);
        return;
    }
    AskForth_Cell cell = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &cell, vm->stack );

    if ( cell.val._64u > vm->blocks->capacity ) {
        _askf_word_failed( (ascii*)"BLOCK -> OOB BLOCK", 18 );
        return;
    }
    ascii* block_start = vm->blocks->start_blocks + ( vm->blocks->block_size * cell.val._64u );

    cell.val._64u = ( u64 )block_start;

    askf_stack_push( &cell, vm->stack );
}

static void askf_word_block_size( void ) { 
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell cell   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    cell.val._64u        = vm->blocks->block_size;

    askf_stack_push( &cell, vm->stack );
}

static void askf_word_line( void ) { 
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"LINE -> Expects ( block_addr n - )", 32 );
        return;
    }

    AskForth_Cell line       = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell blk_addr   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &line, vm->stack );
    askf_stack_pop( &blk_addr, vm->stack );

    if ( line.val._64u > 24 ) {
        _askf_word_failed( (ascii*)"LINE -> line > 16", 17);
        return;
    }

    u64 max_line_len     = vm->blocks->block_size / 16;

    blk_addr.val._64u = blk_addr.val._64u + ( max_line_len * line.val._64u );

    askf_stack_push( &blk_addr, vm->stack );
}

static void askf_word_max_lines( void ) { 
    AskForthVm* vm       = askf_get_global_vm();

    AskForth_Cell cell   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    cell.val._64u        = 16;

    askf_stack_push( &cell, vm->stack );

}

static void askf_word_colon( void ) { 
    AskForthVm* vm       = askf_get_global_vm();
    
    askf_word_parse_word();
    askf_word_parse_word();

    if ( vm->stack->index < 4 ) {
        _askf_word_failed( (ascii*)": -> Expects ': word_name dic_name ' ", 37 );
        return;
    }

    AskForth_Cell cell = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    AskForthToken dic_name = {0};

    askf_stack_pop( &cell, vm->stack );
    dic_name.length = cell.val._64u;
    askf_stack_pop( &cell, vm->stack );
    dic_name.base   = (ascii*)cell.val._64u;

    AskForthToken word_name = {0};

    askf_stack_pop( &cell, vm->stack );
    word_name.length = cell.val._64u;
    askf_stack_pop( &cell, vm->stack );
    word_name.base   = (ascii*)cell.val._64u;


    if ( word_name.length > ASKF_MAX_NAME_LEN ) {
        _askf_word_failed( (ascii*)": -> word name > 28: ", 21 );
        _askf_word_failed( word_name.base , word_name.length );
        return;
    }

    AskForth_Dictionary* dic = askf_library_find_dic( vm, &dic_name );

    if ( !dic ) {
        _askf_word_failed( (ascii*)": -> Dictionary not found: ", 27 );
        _askf_word_failed(  dic_name.base , dic_name.length );
        return;
    }

    askf_dic_add_word_threaded( dic, word_name );

    vm->interpret_state  = ASKF_COMPILE;
}

static void askf_word_semicolon( void ) { 
    AskForthVm* vm       = askf_get_global_vm();
    
    vm->interpret_state  = ASKF_INTERPRET;

    u64* comp_addr = 
        (u64*)( (AskForth_Library*)vm->lib )
            ->curr_compiling.here;

    // signal end of word
    *comp_addr = 0x0;

    ( (AskForth_Library*)vm->lib )
            ->curr_compiling.here = (u64*)askf_alloc( sizeof(u64) );

    ( (AskForth_Library*)vm->lib )->curr_compiling.dic  = NULL;
    ( (AskForth_Library*)vm->lib )->curr_compiling.word = NULL;
}

static void askf_word_immediate( void ) { 
    AskForthVm* vm       = askf_get_global_vm();
    if ( !( (AskForth_Library*)vm->lib )->curr_compiling.word ) {
        _askf_word_failed( (ascii*)"IMMEDIATE -> Must be called inside a word definition", 52);
        return;
    }
    ( (AskForth_Library*)vm->lib )->curr_compiling.word->is_immediate = TRUE;
}

static void askf_word_add_line_toblock( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"a -> Expects ( block_addr )", 27 );
        return;
    }

    AskForth_Cell addr = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &addr, vm->stack );

    u32 read = askf_read_input_blocking_tobuff( vm, (ascii*)addr.val._64u, 
            vm->blocks->block_size );

    // remove the \n from the input
    if ( read )
        ((ascii*)addr.val._64u)[read-1] = ' ';

    addr.val._64u = (u64)read;
    askf_stack_push( &addr, vm->stack );
}

static void askf_word_load( void ) {
    AskForthVm* vm       = askf_get_global_vm();

    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"LOAD -> Expects ( n_block )", 27 );
        return;
    }

    AskForth_Cell addr = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &addr, vm->stack );

    if ( addr.val._64u > vm->blocks->capacity ) {
        _askf_word_failed( (ascii*)"LOAD -> OOB BLOCK", 17 );
        return;
    }

    ascii* block = vm->blocks->start_blocks + vm->blocks->block_size * addr.val._64u;

    // TODO: choose how to execute
    
    // COPY( block, vm->input_buffer->base, vm->blocks->block_size );
}

#ifdef TARGET_LINUX
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
            break;
        }

        if ( ch == '\r' ) {
            // Peek ahead to handle Windows CRLF properly
            int next_ch = fgetc( stream );
            if ( next_ch != '\n' && next_ch != EOF ) 
                ungetc( next_ch, stream );

            break;
        }


        buff[bytes_read++] = (ascii)ch;
    }

    return bytes_read;
}

    static void askf_word_include( void ) { 
        AskForthVm* vm       = askf_get_global_vm();

        askf_word_parse_word();

        AskForth_Cell len   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
        AskForth_Cell path  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

        askf_stack_pop( &len, vm->stack );
        askf_stack_pop( &path, vm->stack );

        FILE *f = fopen( (char*)path.val._64u, "r");

        if ( !f ) {
            _askf_word_failed( (ascii*)"INCLUDE -> Could not open file", 30 );
            _askf_word_failed( (ascii*)path.val._64u, len.val._64u );
            return;
        }

        u64 read = 0;
        int is_eof;

        while ( (read =  
                    _askf_custom_fgets( 
                        vm->input_buffer_x->base, vm->input_buffer_x->capacity , f, &is_eof )) 
                && vm->outer_state == ASKF_VM_OUTER_STATE_EXECUTE )  
            if ( read ) {
                vm->input_buffer_x->index += read;
                 // askf_print( (ascii*)"read:\n", 6 );
                 // askf_print( vm->input_buffer_x->base, vm->input_buffer_x->index );
                vm->input_buffer_x->base[vm->input_buffer_x->index] = '\0';
                askf_exec( vm, ASKF_X_PARSER );
            }


        fclose( f );
    }
#endif

void _askf_print_failed_add_word( AskForthToken* tkn ) {
    askf_print( (ascii*)"Failed adding '", 15 );
    askf_print( tkn->base, tkn->length );
    askf_print( (ascii*)"' word to 'Core' Dictionary\n", 29 );
}

void askf_add_core_words( void ) {
    AskForthToken core_dic_name = {0};
    core_dic_name.base          = (ascii*)"core";
    core_dic_name.length        = 4;

    // DOT 
    AskForthToken scratch_word_name = {0};
    scratch_word_name.base          = (ascii*)".";
    scratch_word_name.length        = 1;

    boolean added_dot = 
        askf_dic_add_word_native( core_dic_name, FALSE ,askf_word_dot, scratch_word_name );

    if ( !added_dot )
        _askf_print_failed_add_word( &scratch_word_name );

    // DEPTH
    scratch_word_name.base            = (ascii*)"depth";
    scratch_word_name.length          = 5;

    boolean added_stack_depth = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_stack_depth, scratch_word_name );

    if ( !added_stack_depth )
        _askf_print_failed_add_word( &scratch_word_name );

    scratch_word_name.base            = (ascii*)".s";
    scratch_word_name.length          = 2;

    // DOT STACK 
    boolean added_dot_stack = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_dot_stack, scratch_word_name );

    if ( !added_dot_stack )
        _askf_print_failed_add_word( &scratch_word_name );

    // DUP
    scratch_word_name.base            = (ascii*)"dup";
    scratch_word_name.length          = 3;

    boolean added_dup = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_dup, scratch_word_name );

    if ( !added_dup )
        _askf_print_failed_add_word( &scratch_word_name );

    // 2DUP
    scratch_word_name.base            = (ascii*)"2dup";
    scratch_word_name.length          = 4;

    boolean added_2dup = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_2dup, scratch_word_name );

    if ( !added_2dup )
        _askf_print_failed_add_word( &scratch_word_name );


    // SWAP
    scratch_word_name.base            = (ascii*)"swap";
    scratch_word_name.length          = 4;

    boolean added_swap = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_swap, scratch_word_name );

    if ( !added_swap )
        _askf_print_failed_add_word( &scratch_word_name );

    // 2SWAP
    scratch_word_name.base            = (ascii*)"2swap";
    scratch_word_name.length          = 5;

    boolean added_2swap = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_2swap, scratch_word_name );

    if ( !added_2swap )
        _askf_print_failed_add_word( &scratch_word_name );


    // nip
    scratch_word_name.base            = (ascii*)"nip";
    scratch_word_name.length          = 3;

    boolean added_nip = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_nip, scratch_word_name );

    if ( !added_nip )
        _askf_print_failed_add_word( &scratch_word_name );

    // tuck
    scratch_word_name.base            = (ascii*)"tuck";
    scratch_word_name.length          = 4;

    boolean added_tuck = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_tuck, scratch_word_name );

    if ( !added_tuck )
        _askf_print_failed_add_word( &scratch_word_name );

    // DROP
    scratch_word_name.base            = (ascii*)"drop";
    scratch_word_name.length          = 4;

    boolean added_drop = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_drop, scratch_word_name );

    if ( !added_drop )
        _askf_print_failed_add_word( &scratch_word_name );

    // 2DROP
    scratch_word_name.base            = (ascii*)"2drop";
    scratch_word_name.length          = 5;

    boolean added_2drop = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_2drop, scratch_word_name );

    if ( !added_2drop )
        _askf_print_failed_add_word( &scratch_word_name );


    // OVER
    scratch_word_name.base            = (ascii*)"over";
    scratch_word_name.length          = 4;

    boolean added_over = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_over, scratch_word_name );

    if ( !added_over )
        _askf_print_failed_add_word( &scratch_word_name );

    // 2OVER
    scratch_word_name.base            = (ascii*)"2over";
    scratch_word_name.length          = 5;

    boolean added_2over = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_2over, scratch_word_name );

    if ( !added_2over )
        _askf_print_failed_add_word( &scratch_word_name );


    // negate
    scratch_word_name.base            = (ascii*)"negate";
    scratch_word_name.length          = 6;

    boolean added_negate = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_negate, scratch_word_name );

    if ( !added_negate )
        _askf_print_failed_add_word( &scratch_word_name );

    // LIB 
    scratch_word_name.base            = (ascii*)"LIB";
    scratch_word_name.length          = 3;

    boolean added_lib = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_lib, scratch_word_name );

    if ( !added_lib )
        _askf_print_failed_add_word( &scratch_word_name );

    // PARSE-NAME
    scratch_word_name.base            = (ascii*)"PARSE-NAME";
    scratch_word_name.length          = 10;

    boolean added_parse_name = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_parse_word, scratch_word_name );

    if ( !added_parse_name )
        _askf_print_failed_add_word( &scratch_word_name );

    // words
    scratch_word_name.base            = (ascii*)"words:";
    scratch_word_name.length          = 6;

    boolean added_words = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_words, scratch_word_name );

    if ( !added_words )
        _askf_print_failed_add_word( &scratch_word_name );

    // +
    scratch_word_name.base            = (ascii*)"+";
    scratch_word_name.length          = 1;

    boolean added_add = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_add, scratch_word_name );

    if ( !added_add )
        _askf_print_failed_add_word( &scratch_word_name );

    // -
    scratch_word_name.base            = (ascii*)"-";
    scratch_word_name.length          = 1;

    boolean added_minus = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_minus, scratch_word_name );

    if ( !added_minus )
        _askf_print_failed_add_word( &scratch_word_name );

    // *
    scratch_word_name.base            = (ascii*)"*";
    scratch_word_name.length          = 1;

    boolean added_multiply = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_multiply, scratch_word_name );

    if ( !added_multiply )
        _askf_print_failed_add_word( &scratch_word_name );

    // /mod
    scratch_word_name.base            = (ascii*)"/mod";
    scratch_word_name.length          = 4;

    boolean added_slashmod = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_slashmod, scratch_word_name );

    if ( !added_slashmod )
        _askf_print_failed_add_word( &scratch_word_name );

    // =
    scratch_word_name.base            = (ascii*)"=";
    scratch_word_name.length          = 1;

    boolean added_equals = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_equals, scratch_word_name );

    if ( !added_equals )
        _askf_print_failed_add_word( &scratch_word_name );

    // 0=
    scratch_word_name.base            = (ascii*)"0=";
    scratch_word_name.length          = 2;

    boolean added_equals_zero = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_equals_zero, scratch_word_name );

    if ( !added_equals_zero )
        _askf_print_failed_add_word( &scratch_word_name );


    // <
    scratch_word_name.base            = (ascii*)"<";
    scratch_word_name.length          = 1;

    boolean added_less_than = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_less_than, scratch_word_name );

    if ( !added_less_than )
        _askf_print_failed_add_word( &scratch_word_name );

    // <=
    scratch_word_name.base            = (ascii*)"<=";
    scratch_word_name.length          = 2;

    boolean added_less_than_or_equal = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_less_than_or_equal, scratch_word_name );

    if ( !added_less_than_or_equal )
        _askf_print_failed_add_word( &scratch_word_name );


    // 0<
    scratch_word_name.base            = (ascii*)"0<";
    scratch_word_name.length          = 2;

    boolean added_less_than_zero = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_less_than_zero, scratch_word_name );

    if ( !added_less_than_zero )
        _askf_print_failed_add_word( &scratch_word_name );


    // >
    scratch_word_name.base            = (ascii*)">";
    scratch_word_name.length          = 1;

    boolean added_more_than = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_more_than, scratch_word_name );

    if ( !added_more_than)
        _askf_print_failed_add_word( &scratch_word_name );

    // >=
    scratch_word_name.base            = (ascii*)">=";
    scratch_word_name.length          = 2;

    boolean added_more_than_or_equal = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_more_than_or_equal, scratch_word_name );

    if ( !added_more_than_or_equal )
        _askf_print_failed_add_word( &scratch_word_name );


    // 0>
    scratch_word_name.base            = (ascii*)"0>";
    scratch_word_name.length          = 2;

    boolean added_more_than_zero = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_more_than_zero, scratch_word_name );

    if ( !added_more_than_zero )
        _askf_print_failed_add_word( &scratch_word_name );


    // <>
    scratch_word_name.base            = (ascii*)"<>";
    scratch_word_name.length          = 2;

    boolean added_not_equal = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_not_equal, scratch_word_name );

    if ( !added_not_equal )
        _askf_print_failed_add_word( &scratch_word_name );

    // 0<>
    scratch_word_name.base            = (ascii*)"0<>";
    scratch_word_name.length          = 3;

    boolean added_not_equal_zero = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_not_equal_zero, scratch_word_name );

    if ( !added_not_equal_zero )
        _askf_print_failed_add_word( &scratch_word_name );


    // SIGNED
    scratch_word_name.base            = (ascii*)"SIGNED";
    scratch_word_name.length          = 6;

    boolean added_stacksigned = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_make_stack_signed , scratch_word_name );

    if ( !added_stacksigned )
        _askf_print_failed_add_word( &scratch_word_name );

    // UNSIGNED
    scratch_word_name.base            = (ascii*)"UNSIGNED";
    scratch_word_name.length          = 8;

    boolean added_stackunsigned = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_make_stack_unsigned , scratch_word_name );

    if ( !added_stackunsigned )
        _askf_print_failed_add_word( &scratch_word_name );

    // BITS
    scratch_word_name.base            = (ascii*)"BITS";
    scratch_word_name.length          = 4;

    boolean added_bits = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_bits , scratch_word_name );

    if ( !added_bits )
        _askf_print_failed_add_word( &scratch_word_name );

    // TYPE
    scratch_word_name.base            = (ascii*)"TYPE";
    scratch_word_name.length          = 4;

    boolean added_type = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_type , scratch_word_name );

    if ( !added_type )
        _askf_print_failed_add_word( &scratch_word_name );

    // !
    scratch_word_name.base            = (ascii*)"!";
    scratch_word_name.length          = 1;

    boolean added_store = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_store , scratch_word_name );

    if ( !added_store )
        _askf_print_failed_add_word( &scratch_word_name );

    // c!
    scratch_word_name.base            = (ascii*)"c!";
    scratch_word_name.length          = 2;

    boolean added_byte_store = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_byte_store , scratch_word_name );

    if ( !added_byte_store )
        _askf_print_failed_add_word( &scratch_word_name );


    // @
    scratch_word_name.base            = (ascii*)"@";
    scratch_word_name.length          = 1;

    boolean added_load_ptr = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_load_ptr , scratch_word_name );

    if ( !added_load_ptr )
        _askf_print_failed_add_word( &scratch_word_name );

    // c@
    scratch_word_name.base            = (ascii*)"c@";
    scratch_word_name.length          = 2;

    boolean added_load_byte_ptr = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_load_byte_ptr , scratch_word_name );

    if ( !added_load_byte_ptr )
        _askf_print_failed_add_word( &scratch_word_name );


    // HERE
    scratch_word_name.base            = (ascii*)"HERE";
    scratch_word_name.length          = 4;

    boolean added_here = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_here , scratch_word_name );

    if ( !added_here )
        _askf_print_failed_add_word( &scratch_word_name );

    // ALLOT
    scratch_word_name.base            = (ascii*)"ALLOT";
    scratch_word_name.length          = 5;

    boolean added_allot = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_allot , scratch_word_name );

    if ( !added_allot )
        _askf_print_failed_add_word( &scratch_word_name );

    // ."
    scratch_word_name.base            = (ascii*)".\"";
    scratch_word_name.length          = 2;

    boolean added_print_string = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_print_string , scratch_word_name );

    if ( !added_print_string )
        _askf_print_failed_add_word( &scratch_word_name );

    // s"
    scratch_word_name.base            = (ascii*)"s\"";
    scratch_word_name.length          = 2;

    boolean added_store_string = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_store_string , scratch_word_name );

    if ( !added_store_string )
        _askf_print_failed_add_word( &scratch_word_name );

    // cr
    scratch_word_name.base            = (ascii*)"cr";
    scratch_word_name.length          = 2;

    boolean added_cr = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_cr, scratch_word_name );

    if ( !added_cr )
        _askf_print_failed_add_word( &scratch_word_name );

    // EMIT
    scratch_word_name.base            = (ascii*)"EMIT";
    scratch_word_name.length          = 4;

    boolean added_emit = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_emit, scratch_word_name );

    if ( !added_emit )
        _askf_print_failed_add_word( &scratch_word_name );

    // lshift
    scratch_word_name.base            = (ascii*)"lshift";
    scratch_word_name.length          = 6;

    boolean added_lshift = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_lshift, scratch_word_name );

    if ( !added_lshift )
        _askf_print_failed_add_word( &scratch_word_name );

    // rshift
    scratch_word_name.base            = (ascii*)"rshift";
    scratch_word_name.length          = 6;

    boolean added_rshift = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_rshift, scratch_word_name );

    if ( !added_rshift )
        _askf_print_failed_add_word( &scratch_word_name );

    // xor
    scratch_word_name.base            = (ascii*)"xor";
    scratch_word_name.length          = 3;

    boolean added_xor = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_xor, scratch_word_name );

    if ( !added_xor )
        _askf_print_failed_add_word( &scratch_word_name );

    // and
    scratch_word_name.base            = (ascii*)"and";
    scratch_word_name.length          = 3;

    boolean added_and = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_and, scratch_word_name );

    if ( !added_and )
        _askf_print_failed_add_word( &scratch_word_name );

    // rot
    scratch_word_name.base            = (ascii*)"rot";
    scratch_word_name.length          = 3;

    boolean added_rot = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_rot, scratch_word_name );

    if ( !added_rot )
        _askf_print_failed_add_word( &scratch_word_name );

    // or
    scratch_word_name.base            = (ascii*)"or";
    scratch_word_name.length          = 2;

    boolean added_or = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_or, scratch_word_name );

    if ( !added_or )
        _askf_print_failed_add_word( &scratch_word_name );

    // invert
    scratch_word_name.base            = (ascii*)"invert";
    scratch_word_name.length          = 6;

    boolean added_invert = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_invert, scratch_word_name );

    if ( !added_invert )
        _askf_print_failed_add_word( &scratch_word_name );


    // 0SP
    scratch_word_name.base            = (ascii*)"0SP";
    scratch_word_name.length          = 3;

    boolean added_clearstack = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_clearstack, scratch_word_name );

    if ( !added_clearstack )
        _askf_print_failed_add_word( &scratch_word_name );

    // TRUE
    scratch_word_name.base            = (ascii*)"TRUE";
    scratch_word_name.length          = 4;

    boolean added_true = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_true, scratch_word_name );

    if ( !added_true )
        _askf_print_failed_add_word( &scratch_word_name );

    // FALSE
    scratch_word_name.base            = (ascii*)"FALSE";
    scratch_word_name.length          = 5;

    boolean added_false = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_false, scratch_word_name );

    if ( !added_false )
        _askf_print_failed_add_word( &scratch_word_name );

    // cells
    scratch_word_name.base            = (ascii*)"cells";
    scratch_word_name.length          = 5;

    boolean added_cells = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_cells, scratch_word_name );

    if ( !added_cells )
        _askf_print_failed_add_word( &scratch_word_name );

    // cell+
    scratch_word_name.base            = (ascii*)"cell+";
    scratch_word_name.length          = 5;

    boolean added_cell_add = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_cell_add, scratch_word_name );

    if ( !added_cell_add )
        _askf_print_failed_add_word( &scratch_word_name );

    // chars
    scratch_word_name.base            = (ascii*)"chars";
    scratch_word_name.length          = 5;

    boolean added_chars = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_chars, scratch_word_name );

    if ( !added_chars )
        _askf_print_failed_add_word( &scratch_word_name );

    // char+
    scratch_word_name.base            = (ascii*)"char+";
    scratch_word_name.length          = 5;

    boolean added_char_add = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_char_add, scratch_word_name );

    if ( !added_char_add )
        _askf_print_failed_add_word( &scratch_word_name );

    // FILL
    scratch_word_name.base            = (ascii*)"FILL";
    scratch_word_name.length          = 4;

    boolean added_fill = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_fill, scratch_word_name );

    if ( !added_fill )
        _askf_print_failed_add_word( &scratch_word_name );

    // COPY
    scratch_word_name.base            = (ascii*)"COPY";
    scratch_word_name.length          = 4;

    boolean added_copy = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_copy, scratch_word_name );

    if ( !added_copy )
        _askf_print_failed_add_word( &scratch_word_name );

    // (
    scratch_word_name.base            = (ascii*)"(";
    scratch_word_name.length          = 1;

    boolean added_comment_paren = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_comment_parenteshis, scratch_word_name );

    if ( !added_comment_paren )
        _askf_print_failed_add_word( &scratch_word_name );

    // slash comment 
    scratch_word_name.base            = (ascii*)"\\";
    scratch_word_name.length          = 1;

    boolean added_comment_slash = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_comment_slash, scratch_word_name );

    if ( !added_comment_slash )
        _askf_print_failed_add_word( &scratch_word_name );

    // FLUSH
    scratch_word_name.base            = (ascii*)"FLUSH";
    scratch_word_name.length          = 5;

    boolean added_flush = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_flush, scratch_word_name );

    if ( !added_flush )
        _askf_print_failed_add_word( &scratch_word_name );

    // LIST
    scratch_word_name.base            = (ascii*)"LIST";
    scratch_word_name.length          = 4;

    boolean added_list = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_list, scratch_word_name );

    if ( !added_list )
        _askf_print_failed_add_word( &scratch_word_name );

    // BLOCK
    scratch_word_name.base            = (ascii*)"BLOCK";
    scratch_word_name.length          = 5;

    boolean added_block = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_block, scratch_word_name );

    if ( !added_block )
        _askf_print_failed_add_word( &scratch_word_name );

    // BLOCK-SIZE
    scratch_word_name.base            = (ascii*)"BLOCK-SIZE";
    scratch_word_name.length          = 10;

    boolean added_block_size = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_block_size, scratch_word_name );

    if ( !added_block_size )
        _askf_print_failed_add_word( &scratch_word_name );

    // LINE
    scratch_word_name.base            = (ascii*)"LINE";
    scratch_word_name.length          = 4;

    boolean added_line = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_line, scratch_word_name );

    if ( !added_line )
        _askf_print_failed_add_word( &scratch_word_name );

    // a
    scratch_word_name.base            = (ascii*)"a";
    scratch_word_name.length          = 1;

    boolean added_atoblock = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_add_line_toblock, scratch_word_name );

    if ( !added_atoblock )
        _askf_print_failed_add_word( &scratch_word_name );


    // MAX-LINES
    scratch_word_name.base            = (ascii*)"MAX-LINES";
    scratch_word_name.length          = 9;

    boolean added_max_lines = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_max_lines, scratch_word_name );

    if ( !added_max_lines )
        _askf_print_failed_add_word( &scratch_word_name );

    // :
    scratch_word_name.base            = (ascii*)":";
    scratch_word_name.length          = 1;

    boolean added_colon = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_colon, scratch_word_name );

    if ( !added_colon )
        _askf_print_failed_add_word( &scratch_word_name );

    // ;
    scratch_word_name.base            = (ascii*)";";
    scratch_word_name.length          = 1;

    boolean added_semicolon = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_semicolon, scratch_word_name );

    if ( !added_semicolon )
        _askf_print_failed_add_word( &scratch_word_name );

    // IMMEDIATE
    scratch_word_name.base            = (ascii*)"IMMEDIATE";
    scratch_word_name.length          = 9;

    boolean added_immediate = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_immediate, scratch_word_name );

    if ( !added_immediate )
        _askf_print_failed_add_word( &scratch_word_name );

    #ifdef TARGET_LINUX
        // INCLUDE
        scratch_word_name.base            = (ascii*)"INCLUDE";
        scratch_word_name.length          = 7;

        boolean added_include = 
            askf_dic_add_word_native( core_dic_name, TRUE, askf_word_include, scratch_word_name );

        if ( !added_include )
            _askf_print_failed_add_word( &scratch_word_name );

    #endif

}
