#include "blocks.h"

#include "../vm/forth_vm.h"

#if defined( TARGET_LINUX )
    #define ASKF_MAX_CWD_CHARS 1024
    #include <stdio.h>
    #include <unistd.h>
    #include <sys/mman.h>
    FILE* global_blocks_file  = NULL;

#elif defined( TARGET_WINDOWS )
    #define ASKF_MAX_CWD_CHARS 1024
    #include <stdio.h>
    #include <windows.h>
    #include <direct.h>
    FILE* global_blocks_file  = NULL;
    HANDLE askf_win_block_mapping;
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

    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )

        #if defined( TARGET_LINUX )
            if ( getcwd(  (char*)tmp_scratch, ASKF_MAX_CWD_CHARS ) == NULL ) {
                // TODO: throw error
                return;
            }
        #elif defined( TARGET_WINDOWS )
            if ( _getcwd(  (char*)tmp_scratch, ASKF_MAX_CWD_CHARS ) == NULL ) {
                // TODO: throw error
                return;
            }
        #endif

        #if defined( TARGET_LINUX )
            char* block_name        = "/blocks.fb";
        #elif defined( TARGET_WINDOWS )
            char* block_name        = "\\blocks.fb";
        #endif

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

        FILE* blocks_file = fopen( tmp_scratch , "r+b" );

        boolean first_start = FALSE;
    
        if ( !blocks_file ) {
            blocks_file = fopen( tmp_scratch, "w+b" );
            first_start = TRUE;
            if ( !blocks_file ) {
                // TODO: throw error
                return;
            }
        }

        global_blocks_file = blocks_file;

        u64 blocks_len    = vm->blocks->capacity * vm->blocks->block_size;

        #if defined( TARGET_LINUX )
            int block_fd      = fileno(global_blocks_file);
            if ( ftruncate( block_fd, blocks_len ) == -1 ) {
                // TODO: throw error
                return;
            }

            void* blocks_base = mmap( NULL, blocks_len, PROT_READ | PROT_WRITE, MAP_SHARED, block_fd, 0);

            if ( blocks_base == MAP_FAILED ) {
                // TODO: throw error
                return;
            }
        #elif defined( TARGET_WINDOWS )
            HANDLE blocks_handle = ( HANDLE )_get_osfhandle( _fileno( global_blocks_file ) );

            if ( _chsize_s( _fileno( global_blocks_file ), blocks_len) != 0 ) {
                return;
            }

            askf_win_block_mapping = CreateFileMappingA(
                    blocks_handle,
                    NULL,
                    PAGE_READWRITE,
                    (DWORD)( blocks_len >> 32 ),
                    (DWORD)( blocks_len & 0xFFFFFFFF ),
                    NULL);

            if ( !askf_win_block_mapping ) {
                return;
            }

            void* blocks_base = MapViewOfFile(
                    askf_win_block_mapping,
                    FILE_MAP_READ | FILE_MAP_WRITE,
                    0, 0, blocks_len );

            if ( !blocks_base ) {
                CloseHandle( askf_win_block_mapping );
                return;
            }
        #endif

        vm->blocks->start_blocks = blocks_base;
        if ( first_start ) {
            FILL( ((ascii*)blocks_base), 0, blocks_len );
        }
        askf_blocks_update();
    #endif

}

int askf_blocks_update( void ) {
    AskForthVm* vm = askf_get_global_vm();

    #if defined( TARGET_LINUX )
        return msync( vm->blocks->start_blocks, vm->blocks->capacity * vm->blocks->block_size, MS_SYNC | MS_INVALIDATE);
    #elif defined( TARGET_WINDOWS )
        u64 blocks_len = vm->blocks->capacity * vm->blocks->block_size;
        BOOL res = FlushViewOfFile( vm->blocks->start_blocks, blocks_len );

        if ( !res ) 
            return -1;

        if ( !FlushFileBuffers( (HANDLE)_get_osfhandle( _fileno( global_blocks_file ))) )
            return -1;

        return 0;
    #endif
}

void askf_blocks_close( void ) {
    AskForthVm* vm = askf_get_global_vm();
    #if defined( TARGET_LINUX )
        if ( global_blocks_file ) {
            munmap( vm->blocks->start_blocks, vm->blocks->block_size * vm->blocks->capacity );
            fclose( global_blocks_file );
        }
    #elif defined( TARGET_WINDOWS )
        if ( global_blocks_file ) {
            UnmapViewOfFile( vm->blocks->start_blocks );
            CloseHandle( askf_win_block_mapping );
            fclose( global_blocks_file );
        }
    #endif
};
