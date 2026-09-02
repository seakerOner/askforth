#include "forth_vm.h"
#include "../input/input.h"
#include "../library/library.h"

#include "../words/askforth_words.h"

AskForthVm* global_vm = NULL;

boolean dispatch_calls_set  = FALSE;

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
    }

    if (ib->index == 0)
        return;

    askf_tokenizer_reset( tknzr );

    ascii*  base_token  = ib->base;
    u64     length      = 0;
    
    for ( u64 x = 0; x < ib->index; x++ ) {
        if ( ib->base[x] == '\r' )
            ib->base[x] = ' ';
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

static void _askf_word_failed( ascii* msg, u64 len ) {
    AskForthError err = {0};
    err.error = ASKF_ERROR_WORD_FAILED;
    err.zone  = ASKF_ERROR_ZONE_INNER;

    AskForthErrorMessage* opt_msg = 
    askf_alloc_new_opt_message( msg, len );
    err.opt_message = opt_msg;
    askf_throw_error( err );
}

typedef void nat_code(void);

static boolean _askf_push_ip_frame( AskForthVm* vm, AskForth_Word* word, 
        u64 ip, boolean to_resume ) {
    if ( vm->tframes_stack->index >= vm->tframes_stack->capacity ) 
        return FALSE;

    u64 idx = vm->tframes_stack->index++;

    if ( word )
        vm->tframes_stack->frames[idx].word = (u64)word;

    if ( to_resume )
        vm->tframes_stack->frames[idx].resume_ip = ip;
    else 
        vm->tframes_stack->frames[idx].base_ip   = ip;

    vm->tframes_stack->frames[idx].to_resume = to_resume;
    return TRUE;
}

static AskForthThreadedFrame* _askf_pop_ip_frame( AskForthVm* vm ) {
    if ( vm->tframes_stack->index == 0 ) 
        return NULL;

    vm->tframes_stack->index--;
    return &vm->tframes_stack->frames[vm->tframes_stack->index];
}

#define NEXT() ip++
#define RUN_OP() goto *(void*)*ip++

void _askf_execute_threaded_frames( void ) {
    AskForthVm* vm               = askf_get_global_vm();

    if ( !dispatch_calls_set )
        goto set_dispatch_calls;

    AskForthThreadedFrame* frame = _askf_pop_ip_frame( vm );

    if ( !frame )
        return;

    AskForth_Word* word          = (AskForth_Word*)frame->word;
    u64* ip                      = NULL;

    if ( frame->to_resume )
        ip = (u64*)frame->resume_ip;
    else 
        ip = (u64*)frame->base_ip;


return_call:
    while ( *ip != THREADED_FLAG_END ) {
        RUN_OP();
        // if ( *ip == THREADED_FLAG_LITERAL ) {
        //     ip++;
        //
        //     AskForth_Cell new_cell = askf_new_cell_payload( vm->stack );
        //     new_cell.val._64u      = *ip;
        //     askf_stack_push( &new_cell, vm->stack );
        //     ip++;
        //
        // } else if ( *ip == THREADED_FLAG_THREADEDWORD ) {
        //     AskForth_Word* new_word = (AskForth_Word*)*( ip + 1 );
        //
        //     _askf_push_ip_frame( vm, word, (u64)(ip + 2), TRUE); // next threaded execution
        //
        //     word = new_word;
        //     ip   = ( u64* )new_word->source.source.threaded_code_start_addr;
        //
        // } else if ( *ip == THREADED_FLAG_SKIPPABLE ) {
        //     ip++;
        //     u64 bytes_toskip = *ip;
        //     ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
        //     ip++;
        //
        // } else if ( *ip == THREADED_FLAG_0BRANCH ) {
        //     ip++;
        //
        //     if ( vm->stack->index < 1 ) {
        //         _askf_word_failed( (ascii*)"0branch expects value on the stack", 34 );
        //         _askf_word_failed( word->name , word->name_len );
        //         return;
        //     }
        //
        //     AskForth_Cell flag = askf_new_cell_payload( vm->stack );
        //     askf_stack_pop( &flag, vm->stack );
        //
        //     if ( flag.val._64u == 0 ) {
        //         u64 bytes_toskip = *ip;
        //         ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
        //     } else 
        //         ip++;
        //
        // } else if ( *ip == THREADED_FLAG_BRANCH ) {
        //     ip++;
        //     u64 bytes_toskip = *ip;
        //     ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
        //
        // } else { //native code  
        //     ((nat_code*)*ip)();
        //
        //     ip++;
        //     if ( vm->outer_state == ASKF_VM_OUTER_STATE_FAILED_CRITICAL ||
        //             vm->outer_state == ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL) {
        //         _askf_push_ip_frame( vm, word, (u64)ip, TRUE);
        //         return;
        //     }
        // }

        op_literal: {
            AskForth_Cell new_cell = askf_new_cell_payload( vm->stack );
            new_cell.val._64u      = *ip;
            askf_stack_push( &new_cell, vm->stack );
            NEXT();
            continue;
        }
        op_threadedword:{
            AskForth_Word* new_word = (AskForth_Word*)*( ip );

            _askf_push_ip_frame( vm, word, (u64)(ip + 1), TRUE); // next threaded execution

            word = new_word;
            ip   = ( u64* )new_word->source.source.threaded_code_start_addr;
            continue;
        }
        op_skippable: {
            u64 bytes_toskip = *ip;
            ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
            NEXT();
            continue;
        }
        op_0branch:{
            if ( vm->stack->index < 1 ) {
                _askf_word_failed( (ascii*)"0branch expects value on the stack", 34 );
                _askf_word_failed( word->name , word->name_len );
                return;
            }

            AskForth_Cell flag = askf_new_cell_payload( vm->stack );
            askf_stack_pop( &flag, vm->stack );

            if ( flag.val._64u == 0 ) {
                u64 bytes_toskip = *ip;
                ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
            } else 
                NEXT();

            continue;
        }
        op_branch:{
            u64 bytes_toskip = *ip;
            ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
            continue;
        }
        op_native:{
            ((nat_code*)*ip)();
            NEXT();

            if ( vm->outer_state == ASKF_VM_OUTER_STATE_FAILED_CRITICAL ||
                    vm->outer_state == ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL) {
                _askf_push_ip_frame( vm, word, (u64)ip, TRUE);
                return;
            }
            continue;
        }
    }

    if ( vm->outer_state == ASKF_VM_OUTER_STATE_FAILED_CRITICAL ||
        vm->outer_state == ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL || 
        vm->tframes_stack->index == 0 ) {
        return;
    }

    AskForthThreadedFrame* restored_frame = _askf_pop_ip_frame( vm );
    word = ( AskForth_Word* )restored_frame->word;
    if ( restored_frame->to_resume )
        ip = (u64*)restored_frame->resume_ip;
    else 
        ip = (u64*)restored_frame->base_ip;

    goto return_call;

    // GCC labels-as-values are local to this function.
    // Capture their addresses for the threaded-code compiler.
set_dispatch_calls:
    vm->dispatch_calls.op_literal       = &&op_literal;
    vm->dispatch_calls.op_threadedword  = &&op_threadedword;
    vm->dispatch_calls.op_0branch       = &&op_0branch;
    vm->dispatch_calls.op_branch        = &&op_branch;
    vm->dispatch_calls.op_native        = &&op_native;
    vm->dispatch_calls.op_skippable     = &&op_skippable;
    dispatch_calls_set  = TRUE;
}

void askf_execute_threaded_word( void ) {
    AskForthVm* vm = askf_get_global_vm();

    AskForth_Cell word = askf_new_cell_payload( vm->stack );
    askf_stack_pop( &word, vm->stack );

    _askf_push_ip_frame( vm, 
            (AskForth_Word*)word.val._64u, 
            ((AskForth_Word*)word.val._64u)->source.source.threaded_code_start_addr, FALSE );
    _askf_execute_threaded_frames();
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
    }

    UNUSED( ib );

    if ( vm->outer_state == ASKF_VM_OUTER_STATE_EXECUTE ) 
        _askf_parse_input_buffer( vm, parse_type );

    u64 start_idx = 0;

    switch ( tokenizer->comment_state ) {
        case ASKF_COMMENT_STATE_SLASH:
            askf_continue_comment_slash();
            if ( tokenizer->comment_state == ASKF_COMMENT_STATE_SLASH )
                goto parse_done;

            start_idx = tokenizer->ctx.idx;
            break;
        case ASKF_COMMENT_STATE_PAREN:
            askf_continue_comment_paren();
            if ( tokenizer->comment_state == ASKF_COMMENT_STATE_PAREN )
                goto parse_done;

           start_idx = tokenizer->ctx.idx;
            break;
        case ASKF_COMMENT_STATE_NONE:
            break;
        default:
            return;
    }

    if ( vm->outer_state == ASKF_VM_OUTER_STATE_EXECUTE_CONTINUE ) { 
        start_idx       = tokenizer->ctx.idx;
        vm->outer_state = ASKF_VM_OUTER_STATE_EXECUTE;

        // having a ASKF_VM_OUTER_STATE_EXECUTE_CONTINUE flag means we continuing 
        // execution after an error ocurred, if the threadedframes stack has content inside
        // it means we stopped execution inside a threaded word and we must resume it until
        // no nested words are left to execute
        
        while ( vm->tframes_stack->index > 0 )  {
            _askf_execute_threaded_frames();

            if ( vm->outer_state == ASKF_VM_OUTER_STATE_FAILED_CRITICAL ||
                    vm->outer_state == ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL) {
                return;
            }
        }
    }

    for (u64 x = start_idx; x < tokenizer->index; x++) {
        AskForth_Word* word =  askf_library_find_word( vm, &tokenizer->tokens[x] );

        if ( word == NULL ) {
            AskForth_Cell new_cell =  askf_new_cell_payload( vm->stack );

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
                        askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_literal );
                        askf_compile_threaded_memory( new_cell.val._64u );
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
                        _askf_push_ip_frame( vm, 
                            word, 
                            word->source.source.threaded_code_start_addr, FALSE );
                         _askf_execute_threaded_frames();
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
                            _askf_push_ip_frame( vm, 
                                word, 
                                word->source.source.threaded_code_start_addr, FALSE );
                            _askf_execute_threaded_frames();
                           break;
                   }
               } else {
                    switch ( word->source.type ) {
                        case ASKF_WORD_NATIVE:
                            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_native );
                            askf_compile_threaded_memory( (u64)word->source.source.native_code );
                            break;
                        case ASKF_WORD_THREADED:
                            if ( _strequal( (ascii*)"EXIT", word->name, 4 ) ) {
                                askf_compile_threaded_memory( THREADED_FLAG_END );
                                break;
                            }

                            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_threadedword );
                            askf_compile_threaded_memory( (u64)word );
                            break;
                    }
               }
               break; 
        }


        if ( tokenizer->ctx.idx > x ) 
            x = tokenizer->ctx.idx;

        if ( vm->outer_state != ASKF_VM_OUTER_STATE_EXECUTE )
            return;
    }

    parse_done:
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
        case ASKF_VM_OUTER_STATE_EXECUTE_CONTINUE:
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

