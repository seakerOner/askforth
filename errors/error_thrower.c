#include "error_thrower.h"
#include "../vm/forth_vm.h"
#include "../input/input.h"

AskForthErrorMessage error_messages[6] = {
    {
        // ASKF_ERROR_FAILED_LIB_ALLOC  
        .message = ( ascii* )"Failed to allocate memory for the library.",        
        .length  = 42
    },
    {
        // ASKF_ERROR_FAILED_CORE_DIC_ALLOC
        .message = ( ascii* )"Failed to allocate memory for dictionary 'core'.", 
        .length  = 48
    },
    {
        // ASKF_ERROR_FAILED_DIC_ALLOC 
        .message = ( ascii* )"Failed to allocate memory for new dictionary.",     
        .length  = 45
    },
    {
        // ASKF_ERROR_UNKNOWN_WORD 
        .message = ( ascii* )"Unknown word: ",
        .length  = 14
    },
    {
        // ASKF_ERROR_WORD_NAME_OVERFLOW
        .message = ( ascii* )"Name for new word is too large (>28): ",
        .length  = 38
    },
    {
        // ASKF_ERROR_UNKNOWN_DIC
        .message = ( ascii* )"Unknown dictionary name: ",
        .length  = 25
    }
};

void askf_start_error_tracer( AskForth_Ram* ram, AskForthErrorTrace* tracer, u64 tracer_capacity ) {
    tracer->capacity = tracer_capacity;
    tracer->head     = 0;
    tracer->errors   = ( AskForthError* ) askf_blob_alloc( ram, (tracer_capacity * sizeof( AskForthError ) ));

    if ( tracer->errors == NULL ) {
        // TODO: error
    }
}

void askf_throw_error( AskForthError error ) {
    switch ( error.zone ) {
        case ASKF_ERROR_ZONE_OUTER:
            askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_FAILED_CRITICAL );
        break;
        case ASKF_ERROR_ZONE_INNER:
            askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL );
        break;
        default:
        break;
    }

    askf_vm_trace_error( error );
}

void askf_print_error( AskForthError* error ) {
    if ( error == NULL )
        return;

    AskForthErrorMessage* msg = &error_messages[error->error];

    askf_print( (ascii*)"[ERROR] ", 8 );
    askf_print( msg->message, msg->length );

    if ( error->opt_message && error->opt_message->message )
        askf_print( error->opt_message->message, ( u32 )error->opt_message->length );

    askf_print( (ascii*)"\n", 1 );
}

