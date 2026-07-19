#ifndef ASKFORTH_VM_H
#define ASKFORTH_VM_H

#include "../memory/backend_blob.h"
#include "../stack/stack.h"
#include "../errors/error_thrower.h"
#include "../input/tokenizer.h"

typedef enum {
    ASKF_VM_OUTER_STATE_BLOCKING_INPUT,
    ASKF_VM_OUTER_STATE_EXECUTE,
    ASKF_VM_OUTER_STATE_FAILED_CRITICAL,
    ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL,
    ASKF_VM_OUTER_STATE_SHUTDOWN_REQUEST,
} AskForthVmOuterState;

typedef struct {
    ascii*  base;
    u64     capacity;
    u64     index;
} AskForthInputBuffer;

typedef struct {
    AskForth_Stack*         stack;
    AskForth_Ram*           ram;
    AskForthInputBuffer*    input_buffer;
    void*                   lib;
    AskForthVmOuterState    outer_state;
    AskForthErrorTrace*     error_tracer;

    AskForthTokenizer*      tokenizer;
} AskForthVm;

void askf_vm_to_global_state( AskForthVm* vm );
AskForthVm* askf_get_global_vm( void );

void askf_exec( AskForthVm* vm );

void askf_vm_change_cell_scale( AskForth_CellSize new_cell_size );

void askf_vm_change_outer_state( AskForthVmOuterState new_state );

void askf_vm_trace_error( AskForthError error );

#endif
