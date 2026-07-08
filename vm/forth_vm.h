#ifndef ASKFORTH_VM_H
#define ASKFORTH_VM_H

#include "../memory/backend_blob.h"
#include "../stack/stack.h"

typedef struct {
    ascii*  base;
    u64     capacity;
    u64     index;
} AskForthInputBuffer;

typedef struct {
    AskForth_Stack*         stack;
    AskForth_Ram*           ram;
    AskForthInputBuffer*    input_buffer;
} AskForthVm;

void askf_exec( AskForthVm* vm );

#endif
