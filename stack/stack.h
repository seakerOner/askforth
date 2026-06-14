#ifndef ASKF_STACK_H
#define ASKF_STACK_H

#include "../inttype.h"

// dont use a value superior to 255 ( treated as a u8 type )
#define ASKF_STACK_64BIT_SIZE 16
#define ASKF_STACK_32BIT_SIZE 32
#define ASKF_STACK_16BIT_SIZE 64
#define ASKF_STACK_8BIT_SIZE 128

typedef enum { 
    ASKF_BITS8 = 8, ASKF_BITS16 = 16, ASKF_BITS32 = 32, ASKF_BITS64 = 64
} CellSize;

typedef struct {
  union {
    u64 space_64[ASKF_STACK_64BIT_SIZE];
    u32 space_32[ASKF_STACK_32BIT_SIZE];
    u16 space_16[ASKF_STACK_16BIT_SIZE];
    u8  space_8 [ASKF_STACK_8BIT_SIZE];
  } cells;

  CellSize cell_scale; 
  u8 index;
  u8 current_max_depth;
} AskForth_Stack;

u32 askf_start_stack( CellSize cell_scale );

#endif
