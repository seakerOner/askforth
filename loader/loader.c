#include "loader.h"

#include "../input/input.h"
#include "../forth_version.h"

#if defined( TARGET_LINUX ) 
    #include <stdio.h>
    #include <unistd.h>
#elif defined( TARGET_WINDOWS )
    #include <stdio.h>
    #include "windows.h"
#endif

#include "ram_manager.c"

#define SCRATCH_SIZE 512

char executable_path[SCRATCH_SIZE] = {0};

static AskForthFooter footer    = {0};

static askf_addr_t ram_recoverable_offset = 0;

// used on runtime to add a starter xt word to the new footer
static askf_addr_t new_starter  = 0;

static boolean askf_get_exec_path( char* path, askf_addr_t size ) {
    #if defined( TARGET_WINDOWS )
        // GetModuleFileNameA includes the null terminator, except on Windows XP..

        DWORD len   = GetModuleFileNameA( NULL, path, (DWORD)size );

        return len > 0 && len < size;
        
    #elif defined ( TARGET_LINUX )
        ssize_t len = readlink( "/proc/self/exe", path, size);
    
        if ( len < 0 || len >= (ssize_t)size - 1 )
            return FALSE;

        path[size-1] = '\0';

    #endif
    return TRUE;
}

boolean askf_image_is_available( void ) {
    if ( !askf_get_exec_path( executable_path, SCRATCH_SIZE ) ) {
        askf_print( (ascii*)"Could not get executable path.", 30 );
        return FALSE;
    }

    FILE* src = fopen( executable_path, "rb" );

    if ( !src ) 
        return FALSE;

    if ( fseek( src, -(sizeof( AskForthFooter )), SEEK_END ) != 0 )  {
        fclose( src );
        return FALSE;
    }
    
    if ( fread( (void *)&footer, sizeof( AskForthFooter ), 1, src ) != 1 ) {
        fclose( src );
        return FALSE;
    }

    fclose( src );

    if ( footer.magic_number != ASKF_MAGIC_NUM ) {
        footer.magic_number     = 0;
        footer.has_image        = FALSE;
        footer.has_starter      = FALSE;
        footer.image_version    = 0;
        return FALSE;   // no footer
    }

    return TRUE;
}

AskForthFooter* askf_get_footer( void ) {
    return &footer;
}

void askf_set_recoverable_blob_offset( AskForthVm* vm ) {
    ram_recoverable_offset = ( askf_addr_t )vm->ram->byte_index;
}

// TODO:
boolean askf_load_image( AskForthVm* vm, AskForthFooter* footer ) {
    if ( footer->magic_number != ASKF_MAGIC_NUM ) {
        askf_print( (ascii*)"Image footer magic number not found", 35 );
        askf_print_char( '\n' );
        return FALSE;
    }
    if ( footer->forth_version != ASKFORTH_VERSION ) {
        askf_print( (ascii*)"Image footer Forth version is different from the executable Forth version", 73 );
        askf_print_char( '\n' );
        return FALSE;
    }
    if ( !footer->has_image ) {
        askf_print( (ascii*)"has_image flag is set to FALSE, aborting...", 43 );
        askf_print_char( '\n' );
        return FALSE;
    }

    FILE* src = fopen( executable_path, "rb" );

    if ( !src )  {
        askf_print( (ascii*)"Could open executable to load image", 35 );
        askf_print_char( '\n' );
        return FALSE;
    }

    if ( fseek( src, -(ASKF_FOOTER_SIZE + footer->image_size) , SEEK_END ) != 0 )  {
        askf_print( (ascii*)"Could not get to image header", 29 );
        askf_print_char( '\n' );
        fclose( src );
        return FALSE;
    }
    AskForthImageHeader header;
    
    if ( fread( (void *)&header, sizeof( AskForthImageHeader ), 1, src ) != 1 ) {
        askf_print( (ascii*)"Could not read image header", 27 );
        askf_print_char( '\n' );
        fclose( src );
        return FALSE;
    }

    askf_addr_t delta = (askf_addr_t)vm->ram->start_ptr;

    vm->lib = (void*)(( (askf_addr_t)header.library_offset ) + delta);
    vm->shadow_lib = (void*)(( (askf_addr_t)header.shadow_library_offset ) + delta);
    vm->foreign_manager = (AskForthForeignManager*)(( (askf_addr_t)header.foreign_manager_offset ) + delta);

    u8* here = ((u8*)vm->ram->start_ptr) + vm->ram->byte_index;

    askf_addr_t read = fread( here, sizeof(u8), ( footer->image_size - header.header_size ) , src );
    if ( read != footer->image_size - header.header_size ) {
        askf_print( (ascii*)"Could not read image into RAM...", 32 );
        askf_print_char( '\n' );
        fclose( src );
    }

    ram_manager_relocate_ram( vm );
    return TRUE;
}

static boolean _askf_create_new_binary( boolean had_previous_footer, AskForthFooter* new_footer, ascii* filename, askf_addr_t len ) {
    FILE* f_old_executable = fopen( executable_path, "rb" );
    filename[len] = 0;
    FILE* f_new_executable = fopen( (char*)filename, "wb" );

    u8 scratch[256];

    while ( TRUE ) {
        askf_addr_t read    = fread( scratch, sizeof(u8), 256, f_old_executable );
        if ( read == 0 ) {
            if ( ferror( f_old_executable ) ) {
                fclose( f_old_executable );
                fclose( f_new_executable );
                return FALSE;
            }
        }
        fwrite( scratch, sizeof(u8), read, f_new_executable );

        if ( feof( f_old_executable ) ) {
            fclose( f_old_executable );
            break;
        }
    }

    // check if a footer and image was already on the binary, if so we need to swap them for the new ones that are in memory
    if ( had_previous_footer ) {
        // we wrote the old image and footer on the binary, so we go back and truncate the file removing the old image and footer
        fseek( f_new_executable, -( footer.image_size + sizeof(footer) ) , SEEK_END);
        #if defined( TARGET_LINUX )
            int x = ftruncate( fileno( f_new_executable ), ftell( f_new_executable ));
            UNUSED(x);
        #elif defined( TARGET_WINDOWS )
            _chsize_s( _fileno( f_new_executable ), ftell( f_new_executable ) );
        #endif
    }

    // write new recoverable image
    AskForthVm* vm = askf_get_global_vm();
    askf_addr_t image_size = 0;

    AskForthImageHeader image_header;
    ram_manager_new_image_header( vm, (askf_addr_t)vm->ram->start_ptr, &image_header );

    ram_manager_ram_to_image_offsets( vm );

    if ( !ram_manager_write_image( &image_header, vm, &image_size, ram_recoverable_offset, f_new_executable ) ) {
        fclose( f_new_executable );
        return FALSE;
    }

    ram_manager_relocate_ram( vm );

    // write new footer
    new_footer->image_size = image_size;
    if ( fwrite( new_footer, sizeof(AskForthFooter) , 1, f_new_executable ) != 1 ) {
        fclose( f_new_executable );
        return FALSE;
    }

    fclose( f_new_executable );
    return TRUE;
}

boolean askf_save_image( AskForthVm* vm , ascii* filename, askf_addr_t len ) {

    boolean had_previous_footer;
    footer.magic_number == 0 ? ( had_previous_footer = TRUE ) : ( had_previous_footer = FALSE );
    boolean has_starter;
    new_starter == 0 ? ( has_starter = FALSE ) : ( has_starter = TRUE ) ;

    AskForthFooter new_footer = {
        .magic_number   = ASKF_MAGIC_NUM,
        .forth_version  = ASKFORTH_VERSION,
        .has_image      = TRUE,
        // .image_size  ( will set later after building the image into the new binary! )
        .image_version  = had_previous_footer ? ( footer.image_version + 1 ) : 1,
        .has_starter    = has_starter,
        .starter_offset = has_starter ? ( new_starter - (askf_addr_t)vm->ram->start_ptr ) : 0 
    };

    // create new binary with new footer
    if ( !_askf_create_new_binary( had_previous_footer, &new_footer, filename, len) )
        return FALSE;

    return TRUE;
}

void askf_new_starterword( askf_addr_t xt ) {
    new_starter = xt;
}
