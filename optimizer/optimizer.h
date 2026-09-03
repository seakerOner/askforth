#ifndef ASKF_OPTIMIZER_H
#define ASKF_OPTIMIZER_H

/*
 * Threaded code optimizer.
 *
 * Recognizes predefined instruction patterns and replaces them in-place
 * with equivalent VM superinstructions.
 *
 * Superinstructions are implemented as direct-threaded code inside the
 * VM's threaded executor, using its existing dispatch labels. The optimizer
 * does not generate machine code dynamically.
 *
 * The goal is to reduce dispatch calls and unnecessary data movement
 * while preserving the semantics of the original Forth definition.
 *
 * Optimizations are applied only to compiled threaded code; individual
 * Forth words remain unchanged and retain their normal semantics.
 *
 * The optimizer will:
 *      - inline trivial words     (ex: : add core + ; will just be + to avoid indirection)
 *      - remove trivial sequences (ex: 'over over' will just be '2dup' )
 *      - fuse stack operations
 *      - fuse arithmetic/data movement
 *
 */

#include "../inttype.h"


#define PATTERN_RPOP              0 
#define PATTERN_SPOP              1
#define PATTERN_LITERAL           2
#define PATTERN_SPUSH             3 
#define PATTERN_RPUSH             4 

#define PATTERN_ARIT_PLUS         5 
#define PATTERN_ARIT_MINUS        6
#define PATTERN_ARIT_MUL          7
#define PATTERN_ARIT_DIVMOD       8

#define PATTERN_STACK_OP_DUP      9
#define PATTERN_STACK_OP_2DUP     10
#define PATTERN_STACK_OP_SWAP     11
#define PATTERN_STACK_OP_2SWAP    12
#define PATTERN_STACK_OP_DROP     13
#define PATTERN_STACK_OP_2DROP    14
#define PATTERN_STACK_OP_OVER     15


#define SIGNATURE_BITS            4
#define SIGNATURE_MAX_OPS        16


#define MASK_PATTERN( pat )       ( 1ULL << pat )

#define SIGNATURE_ADDA( sig, p_op )     \
    (sig) = ( ((sig) << SIGNATURE_BITS ) | (p_op)

#define SIGNATURE_OP( sig, sig_len , index )                          \
        ( ((sig) >> ( ( (sig_len) - 1 - (index) ) * SIGNATURE_BITS )) & 0xFULL)  \

#define OPT_PATTERN( mask, sig, len )   \
        {                               \
            .opcode_mask   = (mask),    \
            .signature     = (sig),     \
            .signature_len = (len)      \
        }                               \

typedef struct {
    u64 opcode_mask; // what opcodes/words are in the pattern
    u64 signature;   // order of the opcodes/words
    u8  signature_len;
} AskForthOptimizerPatternKey;

// a pattern key tells the optimizer what superinstruction it should do 

void askf_optimize_threaded_code( u64* start_address );

#endif
