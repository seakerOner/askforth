#ifndef ASKF_INPUT_STACK_H
#define ASKF_INPUT_STACK_H

#include "../inttype.h"
#include "../input/tokenizer.h"

#define ASKF_INPUT_STACK_MAX_DEPTH 32

typedef struct {
    ascii*  base;
    u64     in;
    u64     in_max;
    u64     byte_cap;

    i64     source_id;  // 0 if source is user input, -1 string from EVALUATE, or N for a FILEID
    u64     blk;        // 0 if input source is not a BLOCK, N if it is, and N is the block number
} AskForth_InputSource;

typedef struct {
  AskForth_InputSource sources[ASKF_INPUT_STACK_MAX_DEPTH];
  u64 index;
  u64 capacity;
} AskForth_InputStack;

void askf_start_istack( AskForth_InputStack* Istack, ascii* base, u64 byte_cap );

boolean askf_istack_push( AskForth_InputStack* Istack, ascii* base, u64 byte_cap , i64 source_id, u64 blk );

AskForth_InputSource* askf_istack_peek( AskForth_InputStack* Istack );

boolean askf_istack_pop( AskForth_InputStack* Istack );

AskForthToken askf_next_token( AskForth_InputSource* source );

#endif
