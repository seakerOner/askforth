#ifndef ASKF_BACKEND_BLOB_H
#define ASKF_BACKEND_BLOB_H

#include "../inttype.h"

#ifdef TARGET_LINUX
    #include "sys/mman.h"
#endif

u8* askf_create_backend_blob( u64 length_bytes, void* opt_addr );

#endif
