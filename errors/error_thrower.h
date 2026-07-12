#ifndef ASKF_ERROR_THROWER_H
#define ASKF_ERROR_THROWER_H

#include "../inttype.h"
#include "../memory/backend_blob.h"
#include "errors.h"

typedef struct {
    u64 head;
    u64 capacity;
    AskForthError* errors;
} AskForthErrorTrace;

void askf_start_error_tracer( AskForth_Ram* ram, AskForthErrorTrace* tracer, u64 tracer_capacity );

void askf_throw_error( AskForthError error );

#endif
