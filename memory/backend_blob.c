#include "./backend_blob.h"

#include "../vm/forth_vm.h"

#if  defined( TARGET_LINUX )
    #include "sys/mman.h"
#elif defined ( TARGET_WINDOWS )
    #include "windows.h"
#endif

boolean askf_create_backend_blob( u64 length_bytes, void* opt_addr, AskForth_Ram* ram_struct ) {
    void* blob = NULL;

    if ( opt_addr != NULL ) {
        blob = opt_addr;
    } else {

        #if defined( TARGET_LINUX )
            blob = mmap( NULL, length_bytes, 
                    PROT_EXEC | PROT_READ | PROT_WRITE , 
                    MAP_ANONYMOUS | MAP_SHARED, -1, 0 );

            if ( blob == MAP_FAILED ) {
                // TODO: throw error
                return FALSE;
            }
        #elif defined( TARGET_WINDOWS )
            blob = VirtualAlloc( NULL, length_bytes, 
                    MEM_RESERVE | MEM_COMMIT , 
                    PAGE_EXECUTE_READWRITE );

            if ( blob == NULL ) {
                // TODO: throw error
                return FALSE;
            }
        #endif
    }

    ram_struct->length      = length_bytes;
    ram_struct->start_ptr   = blob;
    ram_struct->byte_index  = 0;

    FILL( ram_struct->start_ptr, 0, ram_struct->length );
    return TRUE;
};

void* askf_blob_alloc( AskForth_Ram* ram_struct, u64 bytes ) {
    u64 remaining = ram_struct->length - ram_struct->byte_index;
    if ( remaining <= bytes )
        return NULL;

    void* ptr = (void *) ( (( u8* ) ram_struct->start_ptr ) + ram_struct->byte_index ) ;
    ram_struct->byte_index += bytes;

    return ptr;
}


void* askf_alloc( u64 bytes ) {
    AskForthVm* vm = askf_get_global_vm();

    void* blob = askf_blob_alloc( vm->ram, bytes );

    return blob;
}
