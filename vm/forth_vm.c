#include "forth_vm.h"
#include "../input/input.h"
#include "../library/library.h"
#include "../words/askforth_words.h"

AskForthVm* global_vm = NULL;

AskForth_Cell* vm_c00 = NULL;
AskForth_Cell* vm_c01 = NULL;

void askf_vm_to_global_state( AskForthVm* vm ) {
    global_vm = vm;
    AskForth_Cell new_cell = askf_new_cell_payload( global_vm->stack );
    vm_c00 = askf_alloc( sizeof(AskForth_Cell) );
    vm_c01 = askf_alloc( sizeof(AskForth_Cell) );

    COPY( &new_cell, vm_c00, sizeof(AskForth_Cell) );
    COPY( &new_cell, vm_c01, sizeof(AskForth_Cell) );
}

AskForthVm* askf_get_global_vm( void ) {
    return global_vm;
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

    // GCC labels-as-values are local to this function.
    // Capture their addresses for the threaded-code compiler.
    if ( !vm->dispatch_calls.initialized ) {
        vm->dispatch_calls.op_literal        = &&op_literal;
        vm->dispatch_calls.op_threadedword   = &&op_threadedword;
        vm->dispatch_calls.op_0branch        = &&op_0branch;
        vm->dispatch_calls.op_branch         = &&op_branch;
        vm->dispatch_calls.op_native         = &&op_native;
        vm->dispatch_calls.op_native_foreign = &&op_native_foreign;
        vm->dispatch_calls.op_skippable      = &&op_skippable;
        vm->dispatch_calls.op_dispatch_error = &&op_dispatch_error;
        vm->dispatch_calls.op_endword        = &&op_endword;

        vm->dispatch_calls.opt_noop          = &&opt_noop;
        vm->dispatch_calls.opt_type_string   = &&opt_type_string;
        vm->dispatch_calls.initialized       = TRUE;
        return;
    }

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
    while ( TRUE ) {
        RUN_OP();
        
        op_literal: {
            vm_c00->val._64u      = *ip;
            askf_stack_push( vm_c00, vm->stack );
            NEXT();
            continue;
        }
        op_threadedword:{
            u64* new_code = (u64*)*ip;
            NEXT();
            AskForth_Word* new_word = (AskForth_Word*)*( ip );

            _askf_push_ip_frame( vm, word, (u64)(ip + 1), TRUE); // next threaded execution

            word = new_word;
            ip   = new_code;
            continue;
        }
        op_skippable: {
            u64 bytes_toskip = *ip;
            ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
            NEXT();
            continue;
        }
        op_0branch:{
            if ( !askf_stack_pop( vm_c00, vm->stack ) ) {
                _askf_word_failed( (ascii*)"0branch expects value on the stack", 34 );
                _askf_word_failed( word->name , word->name_len );
                return;
            }

            if ( vm_c00->val._64u == 0 ) {
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
            NEXT(); // skip word address 

            if ( vm->outer_state == ASKF_VM_OUTER_STATE_FAILED_CRITICAL ||
                    vm->outer_state == ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL) {
                _askf_push_ip_frame( vm, word, (u64)ip, TRUE);
                return;
            } else if ( vm->outer_state == ASKF_VM_OUTER_STATE_BLOCKING_INPUT ) 
                return;

            continue;
        }
        op_native_foreign:{
            AskForth_Word* word = (AskForth_Word*)*ip;
            NEXT();

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

                _askf_push_ip_frame( vm, word, (u64)ip, TRUE);
                return;
            }
        }
        op_dispatch_error: {
            NEXT(); // skip literal flag
            vm_c00->val._addr_t = *ip++;
            NEXT(); // skip literal flag
            vm_c01->val._addr_t = *ip;
            _askf_word_failed( (ascii*)vm_c00->val._addr_t, vm_c01->val._addr_t );

            _askf_push_ip_frame( vm, word, (u64)(ip + 1), TRUE); // next op
            return;
            //break;
        }
        op_endword:{
            break;
        }

        opt_noop: {
            continue;
        }
        opt_type_string: {
            NEXT(); // skip literal flag
            ascii* base = (ascii*)*ip++;
            NEXT(); // skip literal flag
            u64     len = *ip++;
            askf_print( base, len );
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

}

void askf_execute_threaded_word( void ) {
    AskForthVm* vm = askf_get_global_vm();

    askf_stack_pop( vm_c00, vm->stack );

    _askf_push_ip_frame( vm, 
            (AskForth_Word*)vm_c00->val._64u, 
            ((AskForth_Word*)vm_c00->val._64u)->source.source.threaded_code_start_addr, FALSE );
    _askf_execute_threaded_frames();
}

static boolean _strequal( ascii* str, ascii* to_compare, u64 len ) {
    for ( u64 x = 0; x < len; x++ )
        if ( str[x] != to_compare[x] )
            return FALSE;

    return TRUE;
}

void askf_exec( AskForthVm* vm ) {

    switch ( vm->comment_state ) {
        case ASKF_COMMENT_STATE_SLASH:
            askf_continue_comment_slash();
            if ( vm->comment_state == ASKF_COMMENT_STATE_SLASH ) 
                goto end_exec;

            break;
        case ASKF_COMMENT_STATE_PAREN:
            askf_continue_comment_paren();
            if ( vm->comment_state == ASKF_COMMENT_STATE_PAREN ) 
                goto end_exec;

            break;
        case ASKF_COMMENT_STATE_NONE:
            break;
        default:
            return;
    }

    if ( vm->outer_state == ASKF_VM_OUTER_STATE_EXECUTE_CONTINUE ) { 
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

    AskForthToken token = {0};

    while ( TRUE ) {
        AskForth_InputSource* source = askf_istack_peek( vm->istack );
        token = askf_next_token( source );
        if ( token.length == 0 && token.base == NULL )
            break;

        AskForth_Word* word =  askf_library_find_word( vm, &token );

        if ( word == NULL ) {
            boolean is_number = askf_parse_token_to_num( &token , vm_c00 );

            if ( !is_number ) {
                AskForthErrorMessage* failed_token = ( AskForthErrorMessage* ) &token;
                failed_token->message[failed_token->length] = '\0';

                AskForthError err = 
                {   .zone = ASKF_ERROR_ZONE_OUTER, 
                    .error = ASKF_ERROR_UNKNOWN_WORD,
                    .opt_message = failed_token
                };

                askf_throw_error( err );
                return;
            } else {
                switch ( vm->interpret_state ) {
                    case ASKF_COMPILE:
                        askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_literal );
                        askf_compile_threaded_memory( vm_c00->val._64u );
                        break;
                    case ASKF_INTERPRET:
                        askf_stack_push( vm_c00, vm->stack );
                        break;
                }
                continue;
            }
        }

        switch ( vm->interpret_state ) {
            case ASKF_INTERPRET:
                switch ( word->source.type ) {
                    case ASKF_WORD_NATIVE:
                        word->source.source.native_code();
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
                            askf_compile_threaded_memory( (u64)word );
                            break;
                       case ASKF_WORD_NATIVE_FOREIGN:
                            askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_native_foreign );
                            askf_compile_threaded_memory( (u64)word );
                            break;
                        case ASKF_WORD_THREADED:
                            if ( word->is_inline ) {
                                u64* ip = (u64*)word->source.source.threaded_code_start_addr;
                                while( *ip != (u64)vm->dispatch_calls.op_endword )
                                    askf_compile_threaded_memory(*ip++);
                            } else {
                                askf_compile_threaded_memory( (u64)vm->dispatch_calls.op_threadedword );
                                askf_compile_threaded_memory( 
                                        (u64)word->source.source.threaded_code_start_addr );
                                askf_compile_threaded_memory( (u64)word );
                            }
                            break;
                    }
               }
               break; 
        }


        if ( vm->outer_state != ASKF_VM_OUTER_STATE_EXECUTE )
            return;
    }

    AskForth_InputSource* source = askf_istack_peek( vm->istack );
    if ( vm->outer_state == ASKF_VM_OUTER_STATE_EXECUTE && source->source_id == 0 && source->blk == 0 ) {
        vm->input_buffer->index = 0;
        source->in_max          = 0;
        FILL( vm->input_buffer->base, 0, vm->input_buffer->capacity );
        if ( vm->interpret_state == ASKF_INTERPRET ) 
            askf_print( ( ascii* )"ok.\n", 4 );
        else if ( vm->interpret_state == ASKF_COMPILE ) 
            askf_print( ( ascii* )"compiling.\n", 11 );
    } 

    end_exec:
    // this will not pop the stack if we are in the main input buffer
     askf_istack_pop( vm->istack );
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

