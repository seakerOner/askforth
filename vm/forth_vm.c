#include "forth_vm.h"
#include "../input/input.h"
#include "../library/library.h"

//#include <stdio.h>

AskForthVm* global_vm = NULL;

void askf_vm_to_global_state( AskForthVm* vm ) {
    global_vm = vm;
}

AskForthVm* askf_get_global_vm( void ) {
    return global_vm;
}

static void _askf_parse_input_buffer( AskForthVm* forth_vm, AskForthParseType parse_type ) {
    AskForthInputBuffer* ib     = NULL;
    AskForthTokenizer* tknzr    = NULL;
    switch ( parse_type ) {
        case ASKF_MAIN_PARSER:
            ib       = forth_vm->input_buffer;
            tknzr    = forth_vm->tokenizer;
            break;
        case ASKF_X_PARSER:
            ib       = forth_vm->input_buffer_x;
            tknzr    = forth_vm->tokenizer_x;
            break;
        default:
            return;
            break;
    }

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

typedef void nat_code(void);

static void _askf_execute_threaded_word( AskForth_Word* word ) {
    AskForthVm* vm = askf_get_global_vm();
    u64* ip = (u64*) word->source.source.threaded_code_start_addr;

    // u64* copy_ip = ip;
    //
    // printf("Threaded memory of %.*s: \n ", (int)word->name_len, word->name);
    // while ( *copy_ip != THREADED_FLAG_END ) {
    //     printf("IP = %p, *IP = %ld\n", copy_ip, *copy_ip);
    //     copy_ip++;
    // }
    //     printf("IP = %p, *IP = %ld\n", copy_ip, 0);

    while ( *ip != THREADED_FLAG_END ) {
        // immediate value comming
        if ( *ip == THREADED_FLAG_LITERAL ) {
            ip++;

            AskForth_Cell new_cell = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
            new_cell.val._64u      = *ip;
            askf_stack_push( &new_cell, vm->stack );
        
        // threaded code coming
        } else if ( *ip == THREADED_FLAG_THREADEDWORD ) {
            ip++;
            AskForth_Word* word = (AskForth_Word*)*ip;
            _askf_execute_threaded_word( word );

        // memory to skip over
        } else if ( *ip == THREADED_FLAG_SKIPPABLE ) {
            ip++;
            u64 bytes_toskip = *ip;
            ip = (u64*)( ( (u8*)ip ) + bytes_toskip );

        // 0BRANCH
        } else if ( *ip == THREADED_FLAG_0BRANCH ) {
            ip++;

            if ( vm->stack->index < 1 ) {
                // TODO: do a branch failure
                askf_print( (ascii*)"0branch failure on word: ", 25 );
                askf_print( word->name , word->name_len );
                return;
            }

            AskForth_Cell flag = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
            askf_stack_pop( &flag, vm->stack );

            if ( flag.val._64u == 0 ) {
                u64 bytes_toskip = *ip;
                ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
                continue;
            }

        // BRANCH
        } else if ( *ip == THREADED_FLAG_BRANCH ) {
            ip++;
            u64 bytes_toskip = *ip;
            ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
            continue;

        } else { //native code  
            ((nat_code*)*ip)();
        }
        ip++;
    }
}

void askf_execute_threaded_word( void ) {
    AskForthVm* vm = askf_get_global_vm();

    AskForth_Cell word = askf_new_cell_payload( vm->stack, vm->stack->is_signed );
    askf_stack_pop( &word, vm->stack );

    _askf_execute_threaded_word( (AskForth_Word*)word.val._64u );
}

static boolean _strequal( ascii* str, ascii* to_compare, u64 len ) {
    for ( u64 x = 0; x < len; x++ )
        if ( str[x] != to_compare[x] )
            return FALSE;

    return TRUE;
}

void askf_exec( AskForthVm* vm, AskForthParseType parse_type ) {
    // only used by Forth words inside execution ( ex: PARSE-NAME )
    vm->parse_type = parse_type;

    _askf_parse_input_buffer( vm, parse_type );

    AskForthTokenizer* tokenizer = NULL;
    AskForthInputBuffer* ib      = NULL;

    switch ( parse_type ) {
        case ASKF_MAIN_PARSER:
            ib        = vm->input_buffer;
            tokenizer = vm->tokenizer;
            break;
        case ASKF_X_PARSER:
            ib          = vm->input_buffer_x;
            tokenizer   = vm->tokenizer_x;
            break;
        default:
            return;
            break;
    }

    for (u64 x = 0; x < tokenizer->index; x++) {
        AskForth_Word* word =  askf_library_find_word( vm, &tokenizer->tokens[x] );

        if ( word == NULL ) {
            AskForth_Cell new_cell =  askf_new_cell_payload( vm->stack, vm->stack->is_signed );

            boolean is_number = askf_parse_token_to_num( &tokenizer->tokens[x] , &new_cell );

            if ( !is_number ) {
                AskForthErrorMessage* failed_token = ( AskForthErrorMessage* ) &tokenizer->tokens[x];
                failed_token->message[failed_token->length] = '\0';

                AskForthError err = 
                {   .zone = ASKF_ERROR_ZONE_OUTER, 
                    .error = ASKF_ERROR_UNKNOWN_WORD,
                    .opt_message = failed_token
                };

                // register where the failed token is on the tokenizer
                tokenizer->ctx.token = &tokenizer->tokens[x];
                tokenizer->ctx.idx   = x;

                askf_throw_error( err );
                return;
            } else {
                switch ( vm->interpret_state ) {
                    case ASKF_COMPILE:
                        // flag for immediate value
                        *( (AskForth_Library*)vm->lib )->curr_compiling.here = 
                            THREADED_FLAG_LITERAL;
                        ( (AskForth_Library*)vm->lib )->curr_compiling.here = 
                            askf_alloc( sizeof(u64) );

                        // actual number
                        *( (AskForth_Library*)vm->lib )->curr_compiling.here = 
                            new_cell.val._64u;
                        ( (AskForth_Library*)vm->lib )->curr_compiling.here = 
                            askf_alloc( sizeof(u64) );
                        break;
                    case ASKF_INTERPRET:
                        askf_stack_push( &new_cell, vm->stack );
                        break;
                }
                continue;
            }
        }

        tokenizer->ctx.idx = x;
        tokenizer->ctx.token = &tokenizer->tokens[x];

        switch ( vm->interpret_state ) {
            case ASKF_INTERPRET:
                switch ( word->source.type ) {
                    case ASKF_WORD_NATIVE:
                        word->source.source.native_code();
                        break;
                    case ASKF_WORD_THREADED:
                         _askf_execute_threaded_word( word );
                        break;
                }
               break;

            case ASKF_COMPILE:
               if ( word->is_immediate ) {
                   switch ( word->source.type ) {
                       case ASKF_WORD_NATIVE:
                           word->source.source.native_code();
                           break;
                       case ASKF_WORD_THREADED:
                           _askf_execute_threaded_word( word );
                           break;
                   }
               } else {
                    switch ( word->source.type ) {
                        case ASKF_WORD_NATIVE:
                            *( (AskForth_Library*)vm->lib )->curr_compiling.here = 
                                (u64)word->source.source.native_code;
                            ( (AskForth_Library*)vm->lib )->curr_compiling.here = askf_alloc( sizeof(u64) );
                            break;
                        case ASKF_WORD_THREADED:
                            if ( _strequal( (ascii*)"EXIT", word->name, 4 ) ) {
                                *( (AskForth_Library*)vm->lib )->curr_compiling.here = 
                                    THREADED_FLAG_END;
                                ( (AskForth_Library*)vm->lib )->curr_compiling.here = askf_alloc( sizeof(u64) );
                                break;
                            }

                            *( (AskForth_Library*)vm->lib )->curr_compiling.here = 
                                THREADED_FLAG_THREADEDWORD;
                            ( (AskForth_Library*)vm->lib )->curr_compiling.here = askf_alloc( sizeof(u64) );
                            *( (AskForth_Library*)vm->lib )->curr_compiling.here = (u64)word;
                            ( (AskForth_Library*)vm->lib )->curr_compiling.here = askf_alloc( sizeof(u64) );
                            break;
                    }
               }
               break; 
        }

        if ( tokenizer->ctx.idx > x )
            x = tokenizer->ctx.idx;
    }
    
    askf_tokenizer_reset( tokenizer );
    askf_reset_input_buffer( vm, parse_type );

    if ( vm->outer_state == ASKF_VM_OUTER_STATE_EXECUTE && parse_type == ASKF_MAIN_PARSER ) {
        if ( vm->interpret_state == ASKF_INTERPRET ) 
            askf_print( ( ascii* )"ok.\n", 4 );
        else if ( vm->interpret_state == ASKF_COMPILE ) 
            askf_print( ( ascii* )"compiling.\n", 11 );
    }

}

void askf_vm_change_cell_scale( AskForth_CellSize new_cell_size ) {
    AskForth_Stack* stack = global_vm->stack;

    if ( new_cell_size == stack->cell_scale )
        return;

    switch ( new_cell_size ) {
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
            // TODO: throw error
            break;

    }

    stack->index = ( stack->index * stack->cell_scale + new_cell_size - 1 ) / new_cell_size;

    if ( stack->index > stack->current_max_depth )
        stack->index = stack->current_max_depth;

    stack->cell_scale = new_cell_size;
}

void askf_vm_change_outer_state( AskForthVmOuterState new_state ) {
    switch ( new_state ) {
        case ASKF_VM_OUTER_STATE_BLOCKING_INPUT:
        case ASKF_VM_OUTER_STATE_EXECUTE:
        case ASKF_VM_OUTER_STATE_FAILED_CRITICAL:
        case ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL:
        case ASKF_VM_OUTER_STATE_SHUTDOWN_REQUEST:
            global_vm->outer_state = new_state;
            break;
        default:
            break;
   
    }
}

void askf_vm_trace_error( AskForthError error ) {
    AskForthErrorTrace* tracer  = global_vm->error_tracer;

    u64 absolute_idx            = tracer->head % tracer->capacity;

    COPY( &error, &tracer->errors[absolute_idx], sizeof( AskForthError ) );

    tracer->head++;
};

AskForthError* askf_vm_get_most_recent_error( void ) {
    AskForthErrorTrace* tracer  = global_vm->error_tracer;

    u64 absolute_idx            = ( tracer->head - 1 ) % tracer->capacity;

    return &tracer->errors[absolute_idx];
}
