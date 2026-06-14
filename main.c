#ifdef TARGET_LINUX
#endif

#include "inttype.h"
#include "./memory/backend_blob.h"
#include "./stack/stack.h"

#define RAW_RAM_START_ADDRESS NULL

int main( void ) {
    u8* bck_blob = askf_create_backend_blob( MB(4), RAW_RAM_START_ADDRESS );
    if ( bck_blob == NULL ) {
    };

    CellSize cell_scale = ASKF_BITS8;
    askf_start_stack( cell_scale );

  return 0;
}
