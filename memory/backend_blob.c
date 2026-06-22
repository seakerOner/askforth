#include "./backend_blob.h"

#ifdef TARGET_LINUX
    #include "sys/mman.h"
#endif

#include "sys/mman.h"

void askf_create_backend_blob( u64 length_bytes, void* opt_addr, AskForth_Ram* ram_struct ) {
    void* blob = NULL;

    if ( opt_addr != NULL ) {
        blob = opt_addr;
    }

    #ifdef TARGET_LINUX 
        blob = mmap( NULL, length_bytes, PROT_EXEC | PROT_READ | PROT_WRITE , MAP_ANONYMOUS, -1, 0 );
    #endif

    if ( blob == NULL ) {
        // TODO: throw error
    }

    ram_struct->length      = length_bytes;
    ram_struct->start_ptr   = blob;

};
