#ifndef ASKF_BLOCKS_H
#define ASKF_BLOCKS_H

#include "../inttype.h"

typedef struct {
    u8* start_blocks;
    u64 block_size;
    u64 capacity;

} AskForthBlocks;

void askf_blocks_start( u64 num_blocks, u64 block_bytes );

void askf_blocks_update( void );

void askf_blocks_close( void );

#endif
