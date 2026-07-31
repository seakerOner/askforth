#include "askforth_words.h"
#include "../library/library.h"

#include "../input/input.h"
#include "../stack/stack.h"

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

static void askf_word_drop( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &cell, vm->stack );

    if ( !res ) {
        _askf_word_failed( (ascii*)"drop -> Empty Stack", 19 );
    }
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

static void askf_word_negate( void ) {
    AskForthVm* vm      = askf_get_global_vm();
    AskForth_Cell cell  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    u32 res = askf_stack_pop( &cell, vm->stack );
    if ( !res ) {
        _askf_word_failed( (ascii*)"negate -> Empty Stack", 21 );
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
    AskForthVm* vm = askf_get_global_vm();
    if ( vm->tokenizer->ctx.idx + 1 > vm->tokenizer->capacity ) {
        _askf_word_failed( (ascii*)"PARSE-WORD -> No token found", 28 );
        return;
    }
    vm->tokenizer->ctx.idx += 1;
    u64 idx = vm->tokenizer->ctx.idx;

    AskForth_Cell cell      = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    // TODO: decide if this is what i want from parse_word
    AskForthToken* scratch  = askf_alloc( sizeof( ascii ) * vm->tokenizer->tokens[idx].length );

    COPY( vm->tokenizer->tokens[idx].base, scratch, vm->tokenizer->tokens[idx].length );

    cell.val._64u = (u64)scratch;
    askf_stack_push( &cell, vm->stack );
    cell.val._64u = vm->tokenizer->tokens[idx].length;
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
    }

    AskForth_Cell len   = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    AskForth_Cell addr  = askf_new_cell_payload( vm->stack, vm->stack->is_signed );

    askf_stack_pop( &len, vm->stack );
    askf_stack_pop( &addr, vm->stack );

    askf_print( ( ascii* )addr.val._64u, len.val._32u );
    askf_print( ( ascii* )" ", 1 );
}

void _askf_print_failed_add_word( AskForthToken* tkn ) {
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

    // SWAP
    scratch_word_name.base            = (ascii*)"swap";
    scratch_word_name.length          = 4;

    boolean added_swap = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_swap, scratch_word_name );

    if ( !added_swap )
        _askf_print_failed_add_word( &scratch_word_name );

    // DROP
    scratch_word_name.base            = (ascii*)"drop";
    scratch_word_name.length          = 4;

    boolean added_drop = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_drop, scratch_word_name );

    if ( !added_drop )
        _askf_print_failed_add_word( &scratch_word_name );

    // OVER
    scratch_word_name.base            = (ascii*)"over";
    scratch_word_name.length          = 4;

    boolean added_over = 
        askf_dic_add_word_native( core_dic_name, FALSE, askf_word_over, scratch_word_name );

    if ( !added_over )
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

}
