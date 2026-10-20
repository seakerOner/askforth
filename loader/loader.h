#ifndef ASKF_LOADER_H 
#define ASKF_LOADER_H

#include "../inttype.h"
#include "../vm/forth_vm.h"

#if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
#include <stdio.h>
#endif

#define ASKF_MAGIC_NUM POISON   // 0xDEDEDEDEDEDEDEDEULL

#define ASKF_FOOTER_SIZE 32

typedef struct {
    u64 magic_number;

    u8  forth_version;
    u8  image_version;

    u8  has_image;
    u8  has_starter;

    u64 image_size;
    u64 starter_offset; // relative to the valid start of HERE
} AskForthFooter;

boolean askf_image_is_available( void );

AskForthFooter* askf_get_footer( void );

void askf_set_recoverable_blob_offset( AskForthVm* vm );

boolean askf_load_image( AskForthVm* vm, AskForthFooter* footer );

boolean askf_save_image( AskForthVm* vm , ascii* filename, askf_addr_t len );

void askf_new_starterword( askf_addr_t xt );

#endif
