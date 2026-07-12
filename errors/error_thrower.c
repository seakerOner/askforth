#include "error_thrower.h"
#include "../vm/forth_vm.h"


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


