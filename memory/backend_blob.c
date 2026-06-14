#include "./backend_blob.h"

#ifdef TARGET_LINUX
    #include "sys/mman.h"
#endif

#include "sys/mman.h"

u8* askf_create_backend_blob( u64 length_bytes, void* opt_addr ) {
    if ( opt_addr == NULL ) {
        mmap(void *addr, size_t len, int prot, int flags, int fd, __off_t offset);
    }
}
