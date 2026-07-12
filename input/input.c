#include "input.h"

#ifdef TARGET_LINUX
    #include "unistd.h"
    #include "stdio.h"
#endif

void askf_read_input_blocking( AskForthVm* vm  ) {
    #ifdef TARGET_LINUX
        int res = read( STDIN_FILENO, 
                        ( vm->input_buffer->base + vm->input_buffer->index     ) , 
                        ( vm->input_buffer->capacity - vm->input_buffer->index ) );
        if ( res == 0 )
            return;

        vm->input_buffer->index = vm->input_buffer->index + res;
    #endif

    askf_vm_change_outer_state( ASKF_VM_OUTER_STATE_EXECUTE );
}

void askf_print( ascii* buff, u32 len ) {
    #ifdef TARGET_LINUX
        printf("%*s",len, buff);
    #endif
}


