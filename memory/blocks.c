#include "blocks.h"

#include "../vm/forth_vm.h"

#ifdef TARGET_LINUX
    #include <unistd.h>
    #define ASKF_MAX_CWD_CHARS 1024
    #include <unistd.h>
    #include <sys/mman.h>
    #include <stdio.h>
    FILE* global_blocks_file  = NULL;

#endif


void askf_blocks_start( u64 num_blocks, u64 block_bytes ) {
    AskForthVm* vm =  askf_get_global_vm();

    if ( !vm->blocks ) {
        // TODO: throw error
        return;
    }

    vm->blocks->capacity    = num_blocks;
    vm->blocks->block_size  = block_bytes;

    void* tmp_scratch = ((u8*)vm->ram->start_ptr +  vm->ram->byte_index );

    #ifdef TARGET_LINUX

    if ( getcwd(  (char*)tmp_scratch, ASKF_MAX_CWD_CHARS ) == NULL ) {
        // TODO: throw error
        return;
    }

    char* block_name        = "/blocks.fb";
    u64   block_name_len    = 10;
    u64   cwd_size          = 0;

    for (u64 x = 0; x < ASKF_MAX_CWD_CHARS; x++) 
        if ( (( char* )tmp_scratch )[x] == '\0' )
            break;
        else
            cwd_size++;

    char* ptr_for_name = ( ( char* )tmp_scratch ) + cwd_size;
    COPY( block_name, ptr_for_name, block_name_len );
    ptr_for_name[block_name_len] = '\0';

    FILE* blocks_file = fopen( tmp_scratch , "w+" );
    global_blocks_file = blocks_file;

    int block_fd      = fileno(global_blocks_file);
    u64 blocks_len    = vm->blocks->capacity * vm->blocks->block_size;
    if ( ftruncate( block_fd, blocks_len ) == -1 ) {
        // TODO: throw error
        return;
    }

    void* blocks_base = mmap( NULL, blocks_len, PROT_READ | PROT_WRITE, MAP_SHARED, block_fd, 0);

    if ( blocks_base == MAP_FAILED ) {
        // TODO: throw error
        return;
    }

    vm->blocks->start_blocks = blocks_base;
    FILL( ((ascii*)blocks_base), 0, blocks_len );
    askf_blocks_update();
    #endif

}

int askf_blocks_update( void ) {
    AskForthVm* vm = askf_get_global_vm();

    #ifdef TARGET_LINUX
        return msync( vm->blocks->start_blocks, vm->blocks->capacity * vm->blocks->block_size, MS_SYNC );
    #endif
}

void askf_blocks_close( void ) {
    AskForthVm* vm = askf_get_global_vm();
    #ifdef TARGET_LINUX
        if ( global_blocks_file ) {
            munmap( vm->blocks->start_blocks, vm->blocks->block_size * vm->blocks->capacity );
            fclose( global_blocks_file );
        }
    #endif
};
