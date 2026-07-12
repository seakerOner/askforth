#ifndef ASKF_STACK_H
#define ASKF_STACK_H

#include "../inttype.h"

// dont use a value superior to 255 ( treated as a u8 type )
#define ASKF_STACK_SCALE_BASE 16

#define ASKF_STACK_64BITS_SCALE 1
#define ASKF_STACK_32BITS_SCALE 2
#define ASKF_STACK_16BITS_SCALE 3
#define ASKF_STACK_8BITS_SCALE  4

#define ASKF_STACK_64BIT_SIZE ( ASKF_STACK_SCALE_BASE * ASKF_STACK_64BITS_SCALE )
#define ASKF_STACK_32BIT_SIZE ( ASKF_STACK_SCALE_BASE * ASKF_STACK_32BITS_SCALE )
#define ASKF_STACK_16BIT_SIZE ( ASKF_STACK_SCALE_BASE * ASKF_STACK_16BITS_SCALE )
#define ASKF_STACK_8BIT_SIZE  ( ASKF_STACK_SCALE_BASE * ASKF_STACK_8BITS_SCALE  )

typedef enum { 
    ASKF_BITS8 = 8, ASKF_BITS16 = 16, ASKF_BITS32 = 32, ASKF_BITS64 = 64
} AskForth_CellSize;

typedef struct {
  union {
    u64 space_64[ASKF_STACK_64BIT_SIZE];
    u32 space_32[ASKF_STACK_32BIT_SIZE];
    u16 space_16[ASKF_STACK_16BIT_SIZE];
    u8  space_8 [ASKF_STACK_8BIT_SIZE];
  } cells;

  AskForth_CellSize cell_scale; 
  boolean is_signed;
  u8 index;
  u8 current_max_depth;
} AskForth_Stack;

typedef struct {
    union {
     i64 _64s;
     i32 _32s;
     i16 _16s;
     i8  _8s;
     u64 _64u;
     u32 _32u;
     u16 _16u;
     u8  _8u;
    } val;
    AskForth_CellSize* cell_scale;
    boolean is_signed;
} AskForth_Cell;

u32 askf_start_stack( AskForth_CellSize cell_scale, AskForth_Stack* stack );

inline AskForth_Cell askf_new_cell_payload( AskForth_Stack* stack, boolean is_signed ) {
    AskForth_Cell cell  = {0};
    cell.cell_scale     = &stack->cell_scale;
    cell.is_signed      = is_signed;

    return cell;
}

void askf_stack_push( AskForth_Cell* cell, AskForth_Stack* stack );

u32 askf_stack_pop( AskForth_Cell* out_cell , AskForth_Stack* stack );

#endif
