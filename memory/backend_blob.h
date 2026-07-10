#ifndef ASKF_BACKEND_BLOB_H
#define ASKF_BACKEND_BLOB_H

#include "../inttype.h"

#ifdef TARGET_LINUX
    #include "sys/mman.h"
#endif

typedef struct {
    void* start_ptr;
    u64   byte_index;
    u64   length;
} AskForth_Ram;

void askf_create_backend_blob( u64 length_bytes, void* opt_addr, AskForth_Ram* ram_struct );

void* askf_blob_alloc( AskForth_Ram* ram_struct, u64 bytes );

#endif
