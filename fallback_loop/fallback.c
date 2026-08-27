#include "fallback.h"
#include "../input/tokenizer.h"
#include "../input/input.h"

#include "../library/library.h"

static void _askf_fallback_cmd_help( AskForthVm* vm ) {
    askf_print( (ascii*)"Fallback Commands: ", 19 );
    askf_print_char( (ascii)'\n' );

    AskForthFallBackCmd* cmds = __get_fallback_cmds();

    if ( !cmds ) {
        askf_print( (ascii*)"'help' could not get available commands", 38 );
        askf_print_char( (ascii)'\n' );
        return;
    }

    for (u64 x = 0; x < ASKF_FALLBACK_NUM_CMDS; x++) { 
        AskForthFallBackCmd* cmd = &cmds[x];
        askf_print( (ascii*)"[ ", 2 );
        if ( cmd->cmd_tkn.base )
            askf_print( cmd->cmd_tkn.base, cmd->cmd_tkn.length );
        askf_print( (ascii*)" ] ", 2 );

        if ( cmd->description.base )
            askf_print( cmd->description.base, cmd->description.length );
        askf_print_char( (ascii)'\n' );
    }
}

static void _askf_fallback_cmd_status( AskForthVm* vm ) {
}

static void _askf_fallback_cmd_trace( AskForthVm* vm ) {
    AskForthErrorTrace* tracer  = vm->error_tracer;

    u64 head = tracer->head;
    head--;

    askf_print( (ascii*)"Tracer's errors ( oldest -> newest ): ", 38 );
    askf_print_char( (ascii)'\n' );

    while ( TRUE ) {
        u64 idx            = head % tracer->capacity;
        head--;
        AskForthError* err = &tracer->errors[idx];

        askf_print_error( err );
    
        if ( idx == 0 )
            break;
    }

    if ( vm->tframes_stack->index > 0 ) {
        askf_print_char( (ascii)'\n' );
        askf_print( (ascii*)"Execution trace ( oldest -> newest ):", 37 );
        askf_print_char( (ascii)'\n' );
        for ( u64 x = 0; x < vm->tframes_stack->index; x++ ) {
            AskForthThreadedFrame frame = vm->tframes_stack->frames[x];
            askf_print( ((AskForth_Word*)frame.word)->name ,
                    ((AskForth_Word*)frame.word)->name_len );
            askf_print( (ascii*)" -> ", 4 );
        }
        askf_print_char( (ascii)'\n' );
    }
}

static void _askf_fallback_cmd_stack( AskForthVm* vm ) {
    AskForth_Stack* stack = vm->stack;

    if ( stack->is_signed ) 
        askf_print( (ascii*)"SIGNED ", 7 );
    else
        askf_print( (ascii*)"UNSIGNED ", 9 );

    AskForth_Cell cell = askf_new_cell_payload( stack, stack->is_signed );

    switch ( stack->cell_scale ) {
        case ASKF_BITS64:
            askf_print( (ascii*)"64 BITS ", 8 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_64[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        case ASKF_BITS32:
            askf_print( (ascii*)"32 BITS ", 8 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_32[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        case ASKF_BITS16:
            askf_print( (ascii*)"16 BITS ", 8 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_16[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        case ASKF_BITS8:
            askf_print( (ascii*)"8 BITS ", 7 );
            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_8[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        default:
            askf_print( (ascii*)"Unknown cell scale", 18 );
            break;
    }

    askf_print_char( (ascii)'\n' );
}

static void _askf_fallback_cmd_rstack( AskForthVm* vm ) {
    AskForth_Stack* stack = vm->rstack;

    AskForth_Cell cell = askf_new_cell_payload( stack, stack->is_signed );
    if ( stack->is_signed ) 
        askf_print( (ascii*)"SIGNED ", 7 );
    else
        askf_print( (ascii*)"UNSIGNED ", 9 );


    switch ( stack->cell_scale ) {
        case ASKF_BITS64:
            askf_print( (ascii*)"64 BITS ", 8 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_64[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        case ASKF_BITS32:
            askf_print( (ascii*)"32 BITS ", 8 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_32[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        case ASKF_BITS16:
            askf_print( (ascii*)"16 BITS ", 8 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_16[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        case ASKF_BITS8:
            askf_print( (ascii*)"8 BITS ", 7 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_8[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        default:
            askf_print( (ascii*)"Unknown cell scale", 18 );
            break;
    }

    askf_print_char( (ascii)'\n' );

}

static void _askf_fallback_cmd_cfstack( AskForthVm* vm ) {
    AskForth_Stack* stack = vm->cf_stack;

    AskForth_Cell cell = askf_new_cell_payload( stack, stack->is_signed );
    if ( stack->is_signed ) 
        askf_print( (ascii*)"SIGNED ", 7 );
    else
        askf_print( (ascii*)"UNSIGNED ", 9 );


    switch ( stack->cell_scale ) {
        case ASKF_BITS64:
            askf_print( (ascii*)"64 BITS ", 8 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_64[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        case ASKF_BITS32:
            askf_print( (ascii*)"32 BITS ", 8 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_32[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        case ASKF_BITS16:
            askf_print( (ascii*)"16 BITS ", 8 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_16[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        case ASKF_BITS8:
            askf_print( (ascii*)"8 BITS ", 7 );

            askf_print( (ascii*)"<", 1 );
            cell.val._64u = stack->index;
            askf_print_cell( &cell );
            askf_print( (ascii*)"> ", 2 );

            for ( u64 x = 0; x < stack->index; x++ ) {
                cell.val._64u = 0;
                cell.val._64u = stack->cells.space_8[x];
                askf_print_cell(&cell);
                askf_print( (ascii*)" ", 1 );
            }
            break;
        default:
            askf_print( (ascii*)"Unknown cell scale", 18 );
            break;
    }

    askf_print_char( (ascii)'\n' );

}

static void __print_which_failed_token_is( u64 offset, AskForthTokenizer* tknizer ) {
    for (u64 x = 0; x < offset; x++) {
        askf_print_char( (ascii)' ' );
    }

    for ( u64 x = 0; x < tknizer->tokens[tknizer->ctx.idx].length; x++ ) {
        askf_print_char( (ascii)'^' );
    }
    askf_print( (ascii*)" Failed here.", 13 );
    askf_print_char( (ascii)'\n' );

}

static void _askf_fallback_cmd_input( AskForthVm* vm ) {
    AskForthTokenizer* tknizer = NULL;

    askf_print( (ascii*)"Parse type: ", 12 );
    switch ( vm->parse_type ) {
        case ASKF_MAIN_PARSER:
            askf_print( (ascii*)"ASKF_MAIN_PARSER", 16 );
            tknizer = vm->tokenizer;
            break;
        case ASKF_X_PARSER:
            askf_print( (ascii*)"ASKF_X_PARSER", 13 );
            tknizer = vm->tokenizer_x;
            break;
        default: 
            askf_print( (ascii*)"Unknown parser type", 19);
            askf_print_char( (ascii)'\n' );
            return;
    }
    askf_print_char( (ascii)'\n' );
    askf_print( (ascii*)"Input: ", 7 );
    askf_print_char( (ascii)'\n' );

    u64 line_len                = 1024 / 16;
    u64 wrote                   = 0;
    u64 line_offset             = 0;
    boolean reached_failed_word = FALSE;
    for (u64 x = 0; x < tknizer->index; x++) {
        if ( x == tknizer->ctx.idx )  {
            reached_failed_word = TRUE;
            line_offset = wrote;
        }

        if ( wrote + tknizer->tokens[x].length > line_len ) {
            askf_print_char( (ascii)'\n' );
            if ( reached_failed_word ) {
                __print_which_failed_token_is( line_offset, tknizer );
                reached_failed_word = FALSE;
            }

            wrote = 0;
        } 


        ascii tmp = tknizer->tokens[x].base[tknizer->tokens[x].length];
        tknizer->tokens[x].base[tknizer->tokens[x].length] = '\0';
        askf_print( tknizer->tokens[x].base, tknizer->tokens[x].length );
        tknizer->tokens[x].base[tknizer->tokens[x].length] = tmp;

        wrote += tknizer->tokens[x].length;
        askf_print_char( (ascii)' ' );
        wrote += 1;

        if ( x + 1 >= tknizer->index && reached_failed_word ) {
            askf_print_char( (ascii)'\n' );
            __print_which_failed_token_is( line_offset, tknizer );
        }
    }
    askf_print_char( (ascii)'\n' );
}

static void _askf_fallback_cmd_continue( AskForthVm* vm ) {
    // skip the failed word
    switch ( vm->parse_type ) {
        case ASKF_MAIN_PARSER:
            vm->tokenizer->ctx.idx++;
            break;
        case ASKF_X_PARSER:
            vm->tokenizer_x->ctx.idx++;
            break;
        default: 
            break;
    }
    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_EXECUTE_CONTINUE );
}

static void _askf_fallback_cmd_quit( AskForthVm* vm ) {
    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_SHUTDOWN_REQUEST );
}

static void _askf_fallback_cmd_abort( AskForthVm* vm ) {
    askf_tokenizer_reset( vm->tokenizer );
    askf_tokenizer_reset( vm->tokenizer_x );
    askf_reset_input_buffer( vm, ASKF_MAIN_PARSER );
    askf_reset_input_buffer( vm, ASKF_X_PARSER );
    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_BLOCKING_INPUT );
    vm->interpret_state = ASKF_INTERPRET;
}

static void _askf_fallback_cmd_reset( AskForthVm* vm ) {
    // TODO:
}

AskForthFallBackCmd askf_fallback_cmds[ASKF_FALLBACK_NUM_CMDS] = {
    {   
        { (ascii*)"help",     4 }, 
        { (ascii*)"Shows all the fallback operations and respective descriptions.", 62 }, 
        _askf_fallback_cmd_help      
    },  // FALLBACK_CMD_HELP 
    {   
        { (ascii*)"status",   6 }, 
        { (ascii*)"Shows various VM's states", 25 },
        _askf_fallback_cmd_status    
    },  // FALLBACK_CMD_STATUS    
    {   
        { (ascii*)"trace",    5 }, 
        { (ascii*)"Displays errors in VM's tracer", 30 },
        _askf_fallback_cmd_trace     
    },  // FALLBACK_CMD_TRACE    
    {   
        { (ascii*)"stack",    5 },
        { (ascii*)"Displays all stack contents", 27 },
        _askf_fallback_cmd_stack     
    },  // FALLBACK_CMD_STACK      
    {   
        { (ascii*)"rstack",   6 }, 
        { (ascii*)"Displays all return stack contents", 34 },
        _askf_fallback_cmd_rstack    
    },  // FALLBACK_CMD_RSTACK    
    {   
        { (ascii*)"cfstack",  7 }, 
        { (ascii*)"Displays all control-flow stack contents", 40 },
        _askf_fallback_cmd_cfstack   
    },  // FALLBACK_CMD_CFSTACK   
    {   
        { (ascii*)"input",    5 },
        { (ascii*)"Shows input states and where code came from", 43 },
        _askf_fallback_cmd_input     
    },  // FALLBACK_CMD_INPUT     
    {   
        { (ascii*)"continue", 8 }, 
        { (ascii*)"Continue execution after error", 30 },
        _askf_fallback_cmd_continue  
    },  // FALLBACK_CMD_CONTINUE  
    {   
        { (ascii*)"quit",     4 }, 
        { (ascii*)"Exits out of the VM", 19 },
        _askf_fallback_cmd_quit      
    },  // FALLBACK_CMD_QUIT      
    {   
        { (ascii*)"abort",    5 }, 
        { (ascii*)"Abort current execution and go back to interpretation", 53 },
        _askf_fallback_cmd_abort     
    },  // FALLBACK_CMD_ABORT     
    {   
        { (ascii*)"reset",    5 }, 
        { (ascii*)"**WIP** Reset VM to initial state (This includes defined words and HERE allocations)", 84 },
        _askf_fallback_cmd_reset     
    },  // FALLBACK_CMD_RESET     
};

AskForthFallBackCmd* __get_fallback_cmds(void) {
    return askf_fallback_cmds;
}

static boolean _strequal( ascii* str, ascii* to_compare, u64 len ) {
    for ( u64 x = 0; x < len; x++ )
        if ( str[x] != to_compare[x] )
            return FALSE;

    return TRUE;
}

static void _askf_parse_fallback_input_buffer( AskForthVm* forth_vm ) {
    AskForthInputBuffer* ib     = forth_vm->fallback_input;
    AskForthTokenizer* tknzr    = forth_vm->fallback_tokenizer;

    if (ib->index == 0)
        return;

    askf_tokenizer_reset( tknzr );

    ascii*  base_token  = ib->base;
    u64     length      = 0;
    
    for ( u64 x = 0; x < ib->index; x++ ) {
        if ( ib->base[x] == ' ' || ib->base[x] == '\n' || ib->base[x] == '\0' ) {
            if ( length > 0 ) {
                AskForthToken new_token = {0};
                new_token.base          = base_token;
                new_token.length        = length;

                if ( ib->base[x] == '\n' || ib->base[x] == '\0' )
                    new_token.line_end      = TRUE;

                askf_tokenizer_add( tknzr, new_token );

                length = 0;
            } 

            if ( x + 1 < ib->index )
                base_token  = &ib->base[x + 1];
        } else  {
            length++;
        }
    }

}

void askforth_fallbackloop_run( AskForthVm* vm ) {
    askf_print_char( (ascii)'\n' );
    askf_print( (ascii*)"[RECOVERY FALLBACK]", 19 );
    askf_print_char( (ascii)'\n' );
    askf_print( (ascii*)"Type 'help' for more", 20 );
    askf_print_char( (ascii)'\n' );

    while ( TRUE ) {
        askf_print( (ascii*)"@>", 2 );

        u32 read = 
            askf_read_input_blocking_tobuff( 
                    vm, 
                    vm->fallback_input->base, 
                    vm->fallback_input->capacity );
        vm->fallback_input->index += read;

        _askf_parse_fallback_input_buffer( vm );

        for ( u64 x = 0; x < vm->fallback_tokenizer->index ; x++ ) {
            AskForthToken* new_tkn = &vm->fallback_tokenizer->tokens[x];

            for (u64 x = 0; x < ASKF_FALLBACK_NUM_CMDS; x++) {
                AskForthFallBackCmd* cmd = &askf_fallback_cmds[x];
                if ( cmd->cmd_tkn.length != new_tkn->length )
                    continue;

                if ( _strequal( new_tkn->base, cmd->cmd_tkn.base, cmd->cmd_tkn.length ) )  {
                    cmd->fn( vm );

                    if ( vm->outer_state != ASKF_VM_OUTER_STATE_FAILED_CRITICAL && 
                            vm->outer_state != ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL)
                        break;
                }
            }
        }

        askf_tokenizer_reset( vm->fallback_tokenizer );

        if ( vm->outer_state != ASKF_VM_OUTER_STATE_FAILED_CRITICAL &&
            vm->outer_state != ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL)
            break;
    }

    vm->fallback_input->index = 0;
    FILL( vm->fallback_input->base, 0, vm->fallback_input->capacity );
    askf_tokenizer_reset( vm->fallback_tokenizer );
}
