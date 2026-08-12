#ifndef ASKFORTH_VM_H
#define ASKFORTH_VM_H

#include "../memory/backend_blob.h"
#include "../stack/stack.h"
#include "../errors/error_thrower.h"
#include "../input/tokenizer.h"

#include "../memory/blocks.h"

#define THREADED_FLAG_END           0x0
#define THREADED_FLAG_LITERAL       0x1
#define THREADED_FLAG_THREADEDWORD  0x2
#define THREADED_FLAG_SKIPPABLE     0x3

#define THREADED_FLAG_0BRANCH       0x5
#define THREADED_FLAG_BRANCH        0x6

typedef enum {
    ASKF_VM_OUTER_STATE_BLOCKING_INPUT,
    ASKF_VM_OUTER_STATE_EXECUTE,
    ASKF_VM_OUTER_STATE_FAILED_CRITICAL,
    ASKF_VM_OUTER_STATE_INNER_FAILED_CRITICAL,
    ASKF_VM_OUTER_STATE_SHUTDOWN_REQUEST,
} AskForthVmOuterState;

typedef enum {
    ASKF_INTERPRET,
    ASKF_COMPILE
} AskForthVmInterpreterState;

typedef struct {
    ascii*  base;
    u64     capacity;
    u64     index;
} AskForthInputBuffer;

typedef enum {
    ASKF_BINARY     = 2,
    ASKF_DECIMAL    = 10,
    ASF_HEXADECIMAL = 16
} AskForthNumBase;

typedef enum {
    ASKF_MAIN_PARSER,
    ASKF_X_PARSER
} AskForthParseType;

typedef struct AskForthVm_t {
    AskForth_Stack*                         stack;
    AskForth_Stack*                         cf_stack;
    AskForth_Ram*                           ram;
    AskForthInputBuffer*                    input_buffer;
    AskForthInputBuffer*                    input_buffer_x;
    AskForthBlocks*                         blocks;

    void*                                   lib;
    volatile AskForthVmOuterState           outer_state;
    volatile AskForthVmInterpreterState     interpret_state;
    volatile AskForthParseType              parse_type;
    AskForthErrorTrace*                     error_tracer;

    AskForthTokenizer*                      tokenizer;
    AskForthTokenizer*                      tokenizer_x;
    AskForthNumBase                         num_base;

} AskForthVm;

void askf_vm_to_global_state( AskForthVm* vm );
AskForthVm* askf_get_global_vm( void );

void askf_exec( AskForthVm* vm, AskForthParseType parse_type );

void askf_exec_token( AskForthVm* vm, AskForthToken* token, u64 tokenizer_idx );

void askf_vm_change_cell_scale( AskForth_CellSize new_cell_size );

void askf_vm_change_outer_state( AskForthVmOuterState new_state );

void askf_execute_threaded_word( void );

void askf_vm_trace_error( AskForthError error );
AskForthError* askf_vm_get_most_recent_error( void );

#endif
