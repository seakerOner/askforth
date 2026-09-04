#include "askforth_words.h"
#include "../library/library.h"

#include "../input/input.h"
#include "../stack/stack.h"
#include "../memory/blocks.h"
#include "../optimizer/optimizer.h"
#include "../fallback_loop/fallback.h"

#if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
    #include <stdio.h>
#endif

AskForthVm* vm            = NULL;

// global preallocated cells for native words to use 
AskForth_Cell* global_c00 = NULL;
AskForth_Cell* global_c01 = NULL;
AskForth_Cell* global_c02 = NULL;
AskForth_Cell* global_c03 = NULL;

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
    u32 res = askf_stack_pop( global_c00, vm->stack );

    // TODO: throw error stack underflow
    if ( !res ) {
        _askf_word_failed( (ascii*)". -> Stack Empty", 16 );
        return;
    }

    askf_print_cell( global_c00 );
    askf_print( (ascii*)" ", 1 );
}

static void askf_word_stack_depth( void ) {
    global_c00->val._8u = vm->stack->index;
    askf_stack_push( global_c00, vm->stack );
}

static void askf_word_dot_stack ( void ) {
    global_c00->val._8u = (u8)vm->stack->cell_scale;
    askf_print_cell( global_c00 );
    askf_print( (ascii*)" BITS ", 6 );

    global_c00->val._8u = vm->stack->index;

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

static void askf_word_dup ( void ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );

    if ( !res ) {
        _askf_word_failed( ( ascii* )"dup -> Stack Empty" , 18);
        return;
    }

    askf_stack_push( global_c00, vm->stack );
    askf_stack_push( global_c00, vm->stack );
}

static void askf_word_2dup ( void ) {
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

static void askf_word_swap( void ) {
    AskForth_Cell* cell_ts   = global_c00;
    AskForth_Cell* cell_ss   = global_c01;

    u32 res1 = askf_stack_pop( cell_ts, vm->stack );

    if ( !res1 ) {
        _askf_word_failed( ( ascii* )"swap -> Stack Empty" , 18);
    }

    u32 res2 = askf_stack_pop( cell_ss, vm->stack );

    if ( !res2 ) {
        _askf_word_failed( ( ascii* )"swap -> Stack misses 2nd value" , 30);
    }

    askf_stack_push( cell_ts, vm->stack );
    askf_stack_push( cell_ss, vm->stack );
}

static void askf_word_2swap( void ) {
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

static void askf_word_rot ( void ) {
    if ( vm->stack->index < 3 ) {
        _askf_word_failed( ( ascii* )"rot -> Expects ( a b c - )" , 27 );
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

static void askf_word_nip ( void ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( ( ascii* )"nip -> Expects ( a b - )" , 24 );
        return;
    }

    AskForth_Cell* a  = global_c00;
    AskForth_Cell* b  = global_c01;

    askf_stack_pop( b, vm->stack );
    volatile u32 dummy = askf_stack_pop( a, vm->stack );

    askf_stack_push( b, vm->stack );
}

static void askf_word_tuck ( void ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( ( ascii* )"nip -> Expects ( a b - )" , 24 );
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

static void askf_word_drop( void ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"drop -> Empty Stack", 19 );
    }
}

static void askf_word_2drop( void ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"2drop -> Expects ( a b - )", 26 );
        return;
    }

    volatile u32 dummy;

    dummy = askf_stack_pop( global_c00, vm->stack );
    dummy = askf_stack_pop( global_c00, vm->stack );
}

static void askf_word_over( void ) {
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

static void askf_word_2over( void ) {
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

static void askf_word_negate( void ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );
    if ( !res ) {
        _askf_word_failed( (ascii*)"negate -> Empty Stack", 21 );
        return;
    }

    global_c00->val._addr_t = 0 - global_c00->val._addr_t;
    askf_stack_push( global_c00, vm->stack );
}

static void askf_word_lib( void ) {
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

    if ( ( vm->stack->cell_scale / 8 ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
                (ascii *)"PARSE-NAME -> cell width must match architecture word width", 59 );
        return;
    }

    tokenizer->ctx.idx += 1;
    u64 idx = tokenizer->ctx.idx;

    AskForth_Cell* cell      = global_c00;

    // TODO: decide if this is what i want from parse_word
    u64 len =  sizeof( ascii ) * tokenizer->tokens[idx].length ;

    AskForthToken* scratch  = NULL;
    if ( vm->interpret_state == ASKF_COMPILE && 
            ((AskForth_Library*)vm->lib)->curr_compiling.here ) {
        askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_skippable );
        scratch  = askf_alloc( len );
        askf_compile_threaded_memory( len );
    }  else if ( vm->interpret_state == ASKF_COMPILE 
            && !((AskForth_Library*)vm->lib)->curr_compiling.here ) {
        // this should not happen at any point if we are in compilation
    } else {
        scratch  = askf_alloc( len );
    }


    COPY( tokenizer->tokens[idx].base, scratch, tokenizer->tokens[idx].length );

    cell->val._addr_t = (askf_addr_t)scratch;
    askf_stack_push( cell, vm->stack );
    cell->val._addr_t = tokenizer->tokens[idx].length;
    askf_stack_push( cell, vm->stack );
}

static void askf_word_words( void ) {
    askf_word_parse_word();

    if ( vm->outer_state != ASKF_VM_OUTER_STATE_EXECUTE ) {
        return;
    }

    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"words -> Expects (addr len - )", 30 );
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

static void askf_word_minus( void ) {
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

static void askf_word_multiply( void ) {
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

static void askf_word_slashmod( void ) {
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

static void askf_word_equals( void ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"= -> Expects 2 values on stack" , 30 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;
    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    u64 val = below_top__stack->val._64u == top_stack->val._64u;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

static void askf_word_equals_zero( void ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0= -> Expects 1 values on stack" , 31 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    askf_stack_pop( top_stack, vm->stack );

    u64 val = 0 == top_stack->val._64u;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

static void askf_word_less_than( void ) {
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
        val = below_top__stack->val._64s < top_stack->val._64s;
    else
        val = below_top__stack->val._64u < top_stack->val._64u;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

static void askf_word_less_than_or_equal( void ) {
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
        val = below_top__stack->val._64s <= top_stack->val._64s;
    else
        val = below_top__stack->val._64u <= top_stack->val._64u;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

static void askf_word_less_than_zero( void ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0< -> Expects 1 values on stack" , 31 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    askf_stack_pop( top_stack, vm->stack );

    u64 val = 0;
    if ( vm->stack->is_signed )
        val = 0 < top_stack->val._64s;
    else
        val = 0 < top_stack->val._64u;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

static void askf_word_more_than( void ) {
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
        val = below_top__stack->val._64s > top_stack->val._64s;
    else
        val = below_top__stack->val._64u > top_stack->val._64u;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

static void askf_word_more_than_or_equal( void ) {
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
        val = below_top__stack->val._64s >= top_stack->val._64s;
    else
        val = below_top__stack->val._64u >= top_stack->val._64u;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

static void askf_word_more_than_zero( void ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0> -> Expects 1 values on stack" , 31 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    askf_stack_pop( top_stack, vm->stack );

    u64 val = 0;

    if ( vm->stack->is_signed )
         val = 0 > top_stack->val._64s;
    else
         val = 0 > top_stack->val._64u;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

static void askf_word_not_equal( void ) {
    if ( vm->stack->index < 2 ) {
        _askf_word_failed( (ascii*)"<> -> Expects 2 values on stack" , 31 );
        return;
    }

    AskForth_Cell* top_stack        = global_c00;
    AskForth_Cell* below_top__stack = global_c01;

    askf_stack_pop( top_stack, vm->stack );
    askf_stack_pop( below_top__stack, vm->stack );

    u64 val = below_top__stack->val._64u != top_stack->val._64u;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

static void  askf_word_not_equal_zero( void ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"0<> -> Expects 1 values on stack" , 32 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    askf_stack_pop( global_c00, vm->stack );

    u64 val = 0 != top_stack->val._64u;

    top_stack->val._64u = val;
    askf_stack_push( top_stack, vm->stack );
}

static void askf_word_make_stack_signed( void ) {
    vm->stack->is_signed = TRUE;
}

static void askf_word_make_stack_unsigned( void ) {
    vm->stack->is_signed = FALSE;
}

static void askf_word_bits( void ) {
    u32 res = askf_stack_pop( global_c00, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)" BITS -> Empty Stack", 21 );
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

static void askf_word_type( void ){
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

static void askf_word_store( void ){
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

static void askf_word_byte_store( void ){
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

static void askf_word_load_ptr( void ){
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"@ -> Expects ( addr - )", 27);
        return;
    }
    AskForth_Cell* addr  = global_c00;

    if ( ( vm->stack->cell_scale / 8  ) 
            != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
                (ascii *)"@ -> cell width must match architecture word width", 50 );
        return;
    }       

    askf_stack_pop( addr, vm->stack );

    askf_addr_t* ptr = ( askf_addr_t* )addr->val._addr_t;

    addr->val._addr_t = *ptr;

    askf_stack_push( addr, vm->stack );
}

static void askf_word_load_byte_ptr( void ){
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"c@ -> Expects ( addr - )", 27);
        return;
    }
    AskForth_Cell* addr  = global_c00;

    askf_stack_pop( addr, vm->stack );

    if ( ( vm->stack->cell_scale / 8  ) 
            != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
                (ascii *)"c@ -> cell width must match architecture word width", 51 );
        return;
    }

    u8* ptr = ( u8* )addr->val._addr_t;

    addr->val._addr_t = 0;
    addr->val._8u = *ptr;

    askf_stack_push( addr, vm->stack );
}

static void askf_word_here( void ) {
    AskForth_Cell* addr  = global_c00;

    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
                (ascii *)"HERE -> cell width must match architecture word width", 53 );
        return;
    }

    addr->val._addr_t = ( askf_addr_t )( (( u8* )vm->ram->start_ptr ) + vm->ram->byte_index );

    askf_stack_push( addr, vm->stack );
}

static void askf_word_allot( void ) {
    AskForth_Cell* val  = global_c00;

    u32 res = askf_stack_pop( val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"ALLOT -> Expects ( bytes - )", 26 );
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

static void askf_word_cells( void ) {
    AskForth_Cell* val  = global_c00;

    u32 res = askf_stack_pop( val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"cells -> Expects ( n - )", 24 );
        return;
    }

    val->val._64u *= sizeof( askf_addr_t );

    askf_stack_push( val, vm->stack );
}

static void askf_word_cell_add( void ) {
    AskForth_Cell* val  = global_c00;

    u32 res = askf_stack_pop( val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"cell+ -> Expects ( n - )", 24 );
        return;
    }

    val->val._64u += sizeof( askf_addr_t );

    askf_stack_push( val, vm->stack );
}

static void askf_word_chars( void ) {
    AskForth_Cell* val  = global_c00;

    u32 res = askf_stack_pop( val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"chars -> Expects ( n - )", 24 );
        return;
    }

    val->val._64u *= sizeof( u8 );

    askf_stack_push( val, vm->stack );
}

static void askf_word_char_add( void ) {
    AskForth_Cell* val  = global_c00;

    u32 res = askf_stack_pop( val, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"char+ -> Expects ( n - )", 24 );
        return;
    }

    val->val._64u += sizeof( u8 );

    askf_stack_push( val, vm->stack );
}

static void askf_word_print_string( void ) {
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
    askf_addr_t len         = 0;
    boolean got_terminator  = FALSE;

    while ( ctx_idx < tokenizer->index ) {
        AskForthToken* tkn = &tokenizer->tokens[ctx_idx];
        if ( tkn->base[tkn->length - 1] == '"' ) {
            got_terminator = TRUE;
            len = (askf_addr_t)( tkn->base + tkn->length ) - (askf_addr_t)string_base;
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

    switch ( vm->interpret_state ) {
        case ASKF_INTERPRET: {
            AskForth_Cell* cell = global_c00;
            cell->val._addr_t = (askf_addr_t)string_base;
            askf_stack_push( cell, vm->stack );
            cell->val._addr_t = len;
            askf_stack_push( cell, vm->stack );

            if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
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
            askf_compile_threaded_memory( len+1 );


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

static void askf_word_store_string( void ) {
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

    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)".\" -> cell width must match architecture word width", 51 );
        return;
    }

    u64 ctx_idx = tokenizer->ctx.idx;

    ctx_idx++;
    ascii*  string_base     = tokenizer->tokens[ctx_idx].base;
    askf_addr_t len         = 0;
    boolean got_terminator  = FALSE;

    while ( ctx_idx < tokenizer->index ) {
        AskForthToken* tkn = &tokenizer->tokens[ctx_idx];
        if ( tkn->base[tkn->length - 1] == '"' ) {
            got_terminator = TRUE;
            len = (askf_addr_t)( tkn->base + tkn->length ) - (askf_addr_t)string_base;
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

    if ( vm->interpret_state == ASKF_COMPILE )
        askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_skippable );

    ascii* new_base = askf_alloc( sizeof(ascii) * len+1 );
    
    if ( vm->interpret_state == ASKF_COMPILE )
        askf_compile_threaded_memory( len+1 );

    COPY( string_base, new_base, len );
    new_base[len+1] = '\0';

    AskForth_Cell* cell_addr = global_c00;
    AskForth_Cell* cell_len  = global_c01;

    cell_addr->val._addr_t = (askf_addr_t) new_base;
    cell_len->val._addr_t  = len;

    askf_stack_push( cell_addr, vm->stack );
    askf_stack_push( cell_len, vm->stack );
}

static void askf_word_comment_parenteshis( void ) {
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

    // if ( tokenizer->comment_state == ASKF_COMMENT_STATE_NONE )
    //      ctx_idx++;

    while ( ctx_idx < tokenizer->index ) {
        AskForthToken* tkn = &tokenizer->tokens[ctx_idx];
        if ( tkn->base[tkn->length - 1] == ')' ) {
            tokenizer->ctx.idx       = ctx_idx + 1;
            tokenizer->comment_state = ASKF_COMMENT_STATE_NONE;
            return;
        } 

        ctx_idx++;
    }

    tokenizer->ctx.idx          = tokenizer->index;
    tokenizer->comment_state    = ASKF_COMMENT_STATE_PAREN;
}

static void askf_word_comment_slash( void ) {
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

    // if ( tokenizer->comment_state == ASKF_COMMENT_STATE_NONE )
    //      ctx_idx++;

    while ( ctx_idx < tokenizer->index ) {
        AskForthToken* tkn = &tokenizer->tokens[ctx_idx];
        if ( tkn->line_end == TRUE )  {
            tokenizer->ctx.idx          = ctx_idx + 1;
            tokenizer->comment_state    = ASKF_COMMENT_STATE_NONE;
            return;
        }

        ctx_idx++;
    }

    tokenizer->ctx.idx          = tokenizer->index;
    tokenizer->comment_state    = ASKF_COMMENT_STATE_SLASH;
}

void askf_continue_comment_paren( void ) {
    askf_word_comment_parenteshis();
}
void askf_continue_comment_slash( void ) {
    askf_word_comment_slash();
}

static void askf_word_cr( void ) { 
    askf_print( (ascii*)"\n", 1 );
}

static void askf_word_emit( void ) { 
    AskForth_Cell* ascii_char = global_c00;
    u32 res = askf_stack_pop( ascii_char, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"EMIT -> Expects ( ascii_char - )", 32 );
        return;
    }

    askf_print( &ascii_char->val._8u, 1 );
}

static void askf_word_lshift( void ) {
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

static void askf_word_rshift( void ) {
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

static void askf_word_xor( void ) {
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

static void askf_word_and( void ) {
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

static void askf_word_or( void ) {
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

static void askf_word_invert( void ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"invert -> Expects ( n - )" , 25 );
        return;
    }

    AskForth_Cell* top_stack = global_c00;
    askf_stack_pop( top_stack, vm->stack );

    top_stack->val._64u = ~top_stack->val._64u;

    askf_stack_push( top_stack, vm->stack );
}

static void askf_word_clearstack( void ) {
    vm->stack->index = 0;
}

static void askf_word_true( void ) {
    global_c00->val._64u = -1;

    askf_stack_push( global_c00, vm->stack );
}

static void askf_word_false( void ) {
    global_c00->val._64u = 0;

    askf_stack_push( global_c00, vm->stack );
}

static void askf_word_fill( void ) {
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

static void askf_word_copy( void ) {
    if ( vm->stack->index < 3 ) {
        _askf_word_failed( (ascii*)"MOVE -> Expects ( old_addr new_addr n_bytes - )", 47 );
    }

    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
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

static void askf_word_flush( void ) { 
    askf_blocks_update();

    if ( askf_blocks_update() != 0 ) {
        _askf_word_failed( (ascii*)"FLUSH -> Failed to update BLOCKS", 32 );
    };
}

static void askf_word_list( void ) { 
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"LIST -> Expects ( n - )", 23);
        return;
    }
    AskForth_Cell* block_id = global_c00;
    askf_stack_pop( block_id, vm->stack );

    ascii* start_block = (ascii*)
        ( vm->blocks->start_blocks + ( vm->blocks->block_size * block_id->val._64u ));

    u64 max_lines      = 16;
    u64 max_line_chars = vm->blocks->block_size / max_lines;
    AskForth_Cell* cell = global_c01;

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

static void askf_word_block( void ) { 
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"BLOCK -> Expects ( n - )", 24);
        return;
    }
    AskForth_Cell* cell = global_c00;
    askf_stack_pop( cell, vm->stack );

    if ( cell->val._64u > vm->blocks->capacity ) {
        _askf_word_failed( (ascii*)"BLOCK -> OOB BLOCK", 18 );
        return;
    }

    ascii* block_start = vm->blocks->start_blocks + ( vm->blocks->block_size * cell->val._64u );

    cell->val._64u = ( u64 )block_start;

    askf_stack_push( cell, vm->stack );
}

static void askf_word_block_size( void ) { 
    global_c00->val._64u = vm->blocks->block_size;
    askf_stack_push( global_c00, vm->stack );
}

static void askf_word_line( void ) { 
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

static void askf_word_max_lines( void ) { 
    global_c00->val._64u        = 16;

    askf_stack_push( global_c00, vm->stack );

}

static void askf_word_colon( void ) { 
    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)": -> cell width must match architecture word width", 50 );
        return;
    }
    
    askf_word_parse_word();
    askf_word_parse_word();

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

    askf_dic_add_word_threaded( dic, word_name );

    vm->interpret_state  = ASKF_COMPILE;
}

static void askf_word_semicolon( void ) { 
    vm->interpret_state  = ASKF_INTERPRET;

    askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_endword );
}

static void askf_word_immediate( void ) { 
    if ( !( (AskForth_Library*)vm->lib )->curr_compiling.word ) {
        _askf_word_failed( (ascii*)"IMMEDIATE -> No last word definition found", 42 );
        return;
    }
    ( (AskForth_Library*)vm->lib )->curr_compiling.word->is_immediate = TRUE;
}
static void askf_word_inline( void ) {
    if ( !( (AskForth_Library*)vm->lib )->curr_compiling.word ) {
        _askf_word_failed( (ascii*)"INLINE -> No last word definition found", 39 );
        return;
    }
    ( (AskForth_Library*)vm->lib )->curr_compiling.word->is_inline = TRUE;
}

static void askf_word_optimize( void ) {
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

static void askf_word_add_line_toblock( void ) {
    if ( vm->stack->index < 1 ) {
        _askf_word_failed( (ascii*)"a -> Expects ( block_addr )", 27 );
        return;
    }

    AskForth_Cell* addr = global_c00;
    askf_stack_pop( addr, vm->stack );

    u32 read = askf_read_input_blocking_tobuff( vm, (ascii*)addr->val._64u, 
            vm->blocks->block_size );

    // remove the \n from the input
    if ( read )
        ((ascii*)addr->val._64u)[read-1] = ' ';

    addr->val._64u = (u64)read;
    askf_stack_push( addr, vm->stack );
}

static void askf_word_load( void ) {
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

    ascii* block = vm->blocks->start_blocks + vm->blocks->block_size * addr->val._64u;

    // TODO: choose how to execute
    COPY( block, vm->input_buffer_x->base, vm->blocks->block_size );
    vm->input_buffer_x->index = vm->blocks->block_size;

    askf_exec( vm, ASKF_X_PARSER );
}

static void askf_word_add_dic( void ) { 
    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"ADD-DIC -> cell width must match architecture word width", 56 );
        return;
    }

    askf_word_parse_word();

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

    askf_create_dic( vm, (ascii*)addr->val._64u, len->val._64u );
}

static void askf_word_abort( void ) { 
    if ( vm->interpret_state == ASKF_COMPILE )
        vm->interpret_state = ASKF_INTERPRET;

    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_BLOCKING_INPUT );

    askf_reset_input_buffer( vm, vm->parse_type );

    switch ( vm->parse_type ) {
        case ASKF_MAIN_PARSER:
            askf_tokenizer_reset( vm->tokenizer );
            break;
        case ASKF_X_PARSER:
            askf_tokenizer_reset( vm->tokenizer_x );
            break;
    }
}

static void askf_word_bye( void ) { 
    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_SHUTDOWN_REQUEST );
}

static void askf_word_iscomptime( void ) { 
    AskForth_Cell* mode  = global_c00;

    mode->val._64u       = vm->interpret_state == ASKF_COMPILE ? -1 : 0 ;

    askf_stack_push( mode, vm->stack );
}

static void askf_word_isinterptime( void ) { 
    AskForth_Cell* mode  = global_c00;

    mode->val._64u       = vm->interpret_state == ASKF_INTERPRET ? -1 : 0 ;

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

static void askf_word_if( void ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"IF -> Can only be used in compiled code", 39 );
        return;
    }

    boolean res = _askf_0branch();

    if ( !res ) 
        _askf_word_failed( (ascii*)"IF -> branch failed", 19 ); 
}

static void askf_word_else( void ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"ELSE -> Can only be used in compiled code", 41 );
        return;
    }

    boolean res = _askf_branch();

    if ( !res ) 
        _askf_word_failed( (ascii*)"ELSE -> branch failed", 21 ); 
}

static void askf_word_then( void ) {
    AskForth_Library* lib       = (AskForth_Library*)vm->lib;

    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"THEN -> Must be used in compiled code only", 42 );
        return;
    }

    AskForth_Cell* previous_offset_ptr = global_c00;
    askf_stack_pop( previous_offset_ptr, vm->cf_stack );

    u64 offset = (u64)lib->curr_compiling.here - previous_offset_ptr->val._64u;

    *((u64*)previous_offset_ptr->val._64u) = offset;

    lib->curr_compiling.here = askf_alloc( sizeof(u64) );
}

static void askf_word_begin( void ) {
    AskForth_Library* lib       = (AskForth_Library*)vm->lib;

    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"BEGIN -> Must be used in compiled code only", 43 );
        return;
    }

    u64 address                 = (u64)lib->curr_compiling.here;

    global_c00->val._64u       = address;
    askf_stack_push( global_c00, vm->cf_stack );
}

static void askf_word_while( void ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"WHILE -> Can only be used in compiled code", 42 );
        return;
    }

    boolean res = _askf_0branch();

    if ( !res ) 
        _askf_word_failed( (ascii*)"WHILE", 5 ); 
}

static void askf_word_repeat( void ) {
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

static void askf_word_bracket_open( void ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"[ -> Must be called in compile time", 35 );
        return;
    }

    vm->interpret_state = ASKF_INTERPRET;
}

static void askf_word_bracket_close( void ) {
    if ( vm->interpret_state != ASKF_INTERPRET ) {
        _askf_word_failed( (ascii*)"] -> Must be called in interpret time", 37 );
        return;
    }

    vm->interpret_state = ASKF_COMPILE;
}

static void askf_word_single_quote( void ) {
    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"'-> cell width must match architecture word width", 49 );
        return;
    }

    askf_word_parse_word();

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

static void askf_word_execute( void ) {
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
            askf_execute_threaded_word();
            break;
        default:
            break;
    }
}

static void askf_word_compile_comma( void )  {
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
        default:
            break;
    }
}

static void askf_word_literal( void ) {
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

static void askf_word_postpone( void ) {
    if ( vm->interpret_state != ASKF_COMPILE ) {
        _askf_word_failed( (ascii*)"POSTPONE -> Must be used in compile code", 40 );
        return;
    }

    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"POSTPONE -> cell width must match architecture word width", 57 );
        return;
    }

    askf_word_parse_word();

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

    // idk not working like other compilations
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
    }
}

void askf_word_push_rstack( void ) {
    AskForth_Cell* val = global_c00;
    
    if ( !askf_stack_pop( val, vm->stack ) ) {
        _askf_word_failed( (ascii*)">R -> Data stack is empty", 25 );
        return;
    }

    askf_stack_push( val, vm->rstack );
}

void askf_word_pop_rstack( void ) {
    AskForth_Cell* val = global_c00;
    
    if ( !askf_stack_pop( val, vm->rstack ) ) {
        _askf_word_failed( (ascii*)"R> -> Data stack is empty", 25 );
        return;
    }

    askf_stack_push( val, vm->stack );
}

void askf_word_peek_rstack( void ) {
    AskForth_Cell* val = global_c00;

    if ( vm->rstack->index < 1 ) {
        _askf_word_failed( (ascii*)"R@ -> rstack is empty", 21 );
        return;
    }

    val->val._64u = vm->rstack->cells.space_64[vm->rstack->index-1];

    askf_stack_push( val, vm->stack );
}

void askf_word_see( void ) {
    if ( ( vm->stack->cell_scale / 8  ) != sizeof( askf_addr_t ) ) {
        _askf_word_failed( 
            (ascii *)"see -> cell width must match architecture word width", 52 );
        return;
    }
    
    askf_word_parse_word();
    askf_word_parse_word();

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
        } 
        else if ( flag == (u64)vm->dispatch_calls.op_native ) {
            ip++;
            AskForth_Word* word = (AskForth_Word*)*ip;
            askf_print( word->name, word->name_len );
        } 
        else if ( flag == (u64)vm->dispatch_calls.op_threadedword ) {
            ip++;
            AskForth_Word* word = (AskForth_Word*)*ip;
            askf_print( word->name, word->name_len );
        } 
        else if ( flag == (u64)vm->dispatch_calls.opt_noop ) {
            askf_print( (ascii*)"OPT_NOOP", 8 );
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
            askf_print_char( '\n' );
            askf_print( (ascii*)"0BRANCH",  7 );
            askf_print_char( '\n' );
        }
        else if ( flag == (u64)vm->dispatch_calls.op_branch ) {
            askf_print_char( '\n' );
            askf_print( (ascii*)"BRANCH",  6 );
            askf_print_char( '\n' );
        }
        else if ( flag == (u64)vm->dispatch_calls.opt_type_string ) {
            askf_print( (ascii*)"OPT_TYPE_STRING:",  16 );
        } 
        else if ( flag == (u64)vm->dispatch_calls.op_endword  ) {
            askf_print_char( '\n' );
            askf_print( (ascii*)"; ", 2);
            break;
        }

        askf_print( (ascii*)" ", 1 );
        ip++;
    }

    if ( base->is_immediate )
        askf_print( (ascii*)"IMMEDIATE ", 10 );

    if ( base->is_inline )
        askf_print( (ascii*)"INLINE", 6 );

    askf_print_char( '\n' );
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
            break;
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

    static void askf_word_include( void ) { 
        askf_word_parse_word();

        if ( vm->stack->index < 2 ) {
            _askf_word_failed( (ascii*)"INCLUDE -> Path must be included", 32 );
            return;
        }

        AskForth_Cell* len   = global_c00;
        AskForth_Cell* path  = global_c01;


        askf_stack_pop( len, vm->stack );
        askf_stack_pop( path, vm->stack );

        FILE *f = fopen( (char*)path->val._64u, "r");

        if ( !f ) {
            _askf_word_failed( (ascii*)"INCLUDE -> Could not open file", 30 );
            _askf_word_failed( (ascii*)path->val._64u, len->val._64u );
            return;
        }

        int is_eof;

        while ( vm->outer_state == ASKF_VM_OUTER_STATE_EXECUTE )   { 
            u64 read = _askf_custom_fgets( vm->input_buffer_x->base, 
                    vm->input_buffer_x->capacity - 1, f, &is_eof );

            if ( read > 0 ) {
                vm->input_buffer_x->index = read;
                vm->input_buffer_x->base[vm->input_buffer_x->index] = '\0';

                askf_exec( vm, ASKF_X_PARSER );

               if ( vm->outer_state == ASKF_VM_OUTER_STATE_FAILED_CRITICAL ||
                    vm->outer_state == ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL ) {
                   return;
               }
            }

            if ( is_eof )
                break;
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
    vm                  = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack );

    global_c00 = askf_alloc( sizeof(AskForth_Cell) );
    global_c01 = askf_alloc( sizeof(AskForth_Cell) );
    global_c02 = askf_alloc( sizeof(AskForth_Cell) );
    global_c03 = askf_alloc( sizeof(AskForth_Cell) );

    COPY( &cell, global_c00, sizeof(AskForth_Cell) );
    COPY( &cell, global_c01, sizeof(AskForth_Cell) );
    COPY( &cell, global_c02, sizeof(AskForth_Cell) );
    COPY( &cell, global_c03, sizeof(AskForth_Cell) );

    // trickery to set jump labels set (happens on first run)
    _askf_execute_threaded_frames();

    
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
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_print_string , scratch_word_name );

    if ( !added_print_string )
        _askf_print_failed_add_word( &scratch_word_name );

    // s"
    scratch_word_name.base            = (ascii*)"s\"";
    scratch_word_name.length          = 2;

    boolean added_store_string = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_store_string , scratch_word_name );

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
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_comment_parenteshis, scratch_word_name );

    if ( !added_comment_paren )
        _askf_print_failed_add_word( &scratch_word_name );

    // slash comment 
    scratch_word_name.base            = (ascii*)"\\";
    scratch_word_name.length          = 1;

    boolean added_comment_slash = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_comment_slash, scratch_word_name );

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

    // OPTIMIZE
    scratch_word_name.base            = (ascii*)"OPTIMIZE";
    scratch_word_name.length          = 8;

    boolean added_optimize = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_optimize, scratch_word_name );

    if ( !added_optimize )
        _askf_print_failed_add_word( &scratch_word_name );

    // INLINE
    scratch_word_name.base            = (ascii*)"INLINE";
    scratch_word_name.length          = 6;

    boolean added_inline = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_inline, scratch_word_name );

    if ( !added_inline )
        _askf_print_failed_add_word( &scratch_word_name );



    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        // INCLUDE
        scratch_word_name.base            = (ascii*)"INCLUDE";
        scratch_word_name.length          = 7;

        boolean added_include = 
            askf_dic_add_word_native( core_dic_name, FALSE, askf_word_include, scratch_word_name );

        if ( !added_include )
            _askf_print_failed_add_word( &scratch_word_name );

    #endif

    // LOAD
    scratch_word_name.base            = (ascii*)"LOAD";
    scratch_word_name.length          = 4;

    boolean added_load = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_load, scratch_word_name );

    if ( !added_load )
        _askf_print_failed_add_word( &scratch_word_name );

    // ADD-DIC
    scratch_word_name.base            = (ascii*)"ADD-DIC";
    scratch_word_name.length          = 7;

    boolean added_add_dic = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_add_dic, scratch_word_name );

    if ( !added_add_dic )
        _askf_print_failed_add_word( &scratch_word_name );

    // ABORT
    scratch_word_name.base            = (ascii*)"ABORT";
    scratch_word_name.length          = 5;

    boolean added_abort = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_abort, scratch_word_name );

    if ( !added_abort )
        _askf_print_failed_add_word( &scratch_word_name );

    // EXIT
    // special word treated by the VM
    scratch_word_name.base            = (ascii*)"EXIT";
    scratch_word_name.length          = 4;

    boolean added_exit = 
        askf_dic_add_word_native( core_dic_name, FALSE, NULL, scratch_word_name );

    if ( !added_exit )
        _askf_print_failed_add_word( &scratch_word_name );

    // bye
    scratch_word_name.base            = (ascii*)"bye";
    scratch_word_name.length          = 3;

    boolean added_bye = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_bye, scratch_word_name );

    if ( !added_bye )
        _askf_print_failed_add_word( &scratch_word_name );

    // COMPTIME?
    scratch_word_name.base            = (ascii*)"COMPTIME?";
    scratch_word_name.length          = 9;

    boolean added_iscomptime = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_iscomptime, scratch_word_name );

    if ( !added_iscomptime )
        _askf_print_failed_add_word( &scratch_word_name );

    // INTERPTIME?
    scratch_word_name.base            = (ascii*)"INTERPTIME?";
    scratch_word_name.length          = 11;

    boolean added_isinterp = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_isinterptime, scratch_word_name );

    if ( !added_isinterp )
        _askf_print_failed_add_word( &scratch_word_name );


    // INFO: totally valid words to include on the core dic BUT im not sure if i want so
    UNUSED( askf_word_0branch );
    UNUSED( askf_word_branch  );
    
    // // 0BRANCH
    // scratch_word_name.base            = (ascii*)"0BRANCH";
    // scratch_word_name.length          = 7;
    //
    // boolean added_0branch = 
    //     askf_dic_add_word_native( core_dic_name, FALSE, askf_word_0branch, scratch_word_name );
    //
    // if ( !added_0branch )
    //     _askf_print_failed_add_word( &scratch_word_name );
    //
    // // BRANCH
    // scratch_word_name.base            = (ascii*)"BRANCH";
    // scratch_word_name.length          = 6;
    //
    // boolean added_branch = 
    //     askf_dic_add_word_native( core_dic_name, FALSE, askf_word_branch, scratch_word_name );
    //
    // if ( !added_branch )
    //     _askf_print_failed_add_word( &scratch_word_name );

    // IF
    scratch_word_name.base            = (ascii*)"IF";
    scratch_word_name.length          = 2;

    boolean added_if = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_if, scratch_word_name );

    if ( !added_if )
        _askf_print_failed_add_word( &scratch_word_name );

    // ELSE
    scratch_word_name.base            = (ascii*)"ELSE";
    scratch_word_name.length          = 4;

    boolean added_else = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_else, scratch_word_name );

    if ( !added_else )
        _askf_print_failed_add_word( &scratch_word_name );

    // THEN
    scratch_word_name.base            = (ascii*)"THEN";
    scratch_word_name.length          = 4;

    boolean added_then = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_then, scratch_word_name );

    if ( !added_then )
        _askf_print_failed_add_word( &scratch_word_name );

    // BEGIN
    scratch_word_name.base            = (ascii*)"BEGIN";
    scratch_word_name.length          = 5;

    boolean added_begin = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_begin, scratch_word_name );

    if ( !added_begin )
        _askf_print_failed_add_word( &scratch_word_name );

    // WHILE
    scratch_word_name.base            = (ascii*)"WHILE";
    scratch_word_name.length          = 5;

    boolean added_while = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_while, scratch_word_name );

    if ( !added_while )
        _askf_print_failed_add_word( &scratch_word_name );

    // REPEAT
    scratch_word_name.base            = (ascii*)"REPEAT";
    scratch_word_name.length          = 6;

    boolean added_repeat = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_repeat, scratch_word_name );

    if ( !added_repeat )
        _askf_print_failed_add_word( &scratch_word_name );


    // [
    scratch_word_name.base            = (ascii*)"[";
    scratch_word_name.length          = 1;

    boolean added_open_bracket = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_bracket_open, scratch_word_name );

    if ( !added_open_bracket )
        _askf_print_failed_add_word( &scratch_word_name );

    // ]
    scratch_word_name.base            = (ascii*)"]";
    scratch_word_name.length          = 1;

    boolean added_close_bracket = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_bracket_close, scratch_word_name );

    if ( !added_close_bracket )
        _askf_print_failed_add_word( &scratch_word_name );

    // '
    scratch_word_name.base            = (ascii*)"'";
    scratch_word_name.length          = 1;

    boolean added_single_quote = 
        askf_dic_add_word_native( core_dic_name, TRUE, askf_word_single_quote, scratch_word_name );

    if ( !added_single_quote )
        _askf_print_failed_add_word( &scratch_word_name );

    // LITERAL
    scratch_word_name.base            = (ascii*)"LITERAL";
    scratch_word_name.length          = 7;

    boolean added_literal = 
        askf_dic_add_word_native( 
                core_dic_name, TRUE, askf_word_literal, scratch_word_name );

    if ( !added_literal )
        _askf_print_failed_add_word( &scratch_word_name );

    // POSTPONE
    scratch_word_name.base            = (ascii*)"POSTPONE";
    scratch_word_name.length          = 8;

    boolean added_postpone = 
        askf_dic_add_word_native( 
                core_dic_name, TRUE, askf_word_postpone, scratch_word_name );

    if ( !added_postpone )
        _askf_print_failed_add_word( &scratch_word_name );

    // EXECUTE
    scratch_word_name.base            = (ascii*)"EXECUTE";
    scratch_word_name.length          = 7;

    boolean added_execute = 
        askf_dic_add_word_native( 
                core_dic_name, FALSE, askf_word_execute, scratch_word_name );

    if ( !added_execute )
        _askf_print_failed_add_word( &scratch_word_name );

    // COMPILE,
    scratch_word_name.base            = (ascii*)"COMPILE,";
    scratch_word_name.length          = 8;

    boolean added_compile_comma = 
        askf_dic_add_word_native( 
                core_dic_name, FALSE, askf_word_compile_comma, scratch_word_name );

    if ( !added_compile_comma )
        _askf_print_failed_add_word( &scratch_word_name );

    // >R
    scratch_word_name.base            = (ascii*)">R";
    scratch_word_name.length          = 2;

    boolean added_push_rstack = 
        askf_dic_add_word_native( 
                core_dic_name, FALSE, askf_word_push_rstack, scratch_word_name );

    if ( !added_push_rstack )
        _askf_print_failed_add_word( &scratch_word_name );

    // R>
    scratch_word_name.base            = (ascii*)"R>";
    scratch_word_name.length          = 2;

    boolean added_pop_rstack = 
        askf_dic_add_word_native( 
                core_dic_name, FALSE, askf_word_pop_rstack, scratch_word_name );

    if ( !added_pop_rstack )
        _askf_print_failed_add_word( &scratch_word_name );

    // R@
    scratch_word_name.base            = (ascii*)"R@";
    scratch_word_name.length          = 2;

    boolean added_peek_rstack = 
        askf_dic_add_word_native( 
                core_dic_name, FALSE, askf_word_peek_rstack, scratch_word_name );

    if ( !added_peek_rstack )
        _askf_print_failed_add_word( &scratch_word_name );

    // see
    scratch_word_name.base            = (ascii*)"see";
    scratch_word_name.length          = 3;

    boolean added_see = 
        askf_dic_add_word_native( 
                core_dic_name, FALSE, askf_word_see, scratch_word_name );

    if ( !added_see )
        _askf_print_failed_add_word( &scratch_word_name );

}
