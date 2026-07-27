#include "input.h"

#ifdef TARGET_LINUX
    #include "unistd.h"
    #include "stdio.h"
#endif

void askf_reset_input_buffer( AskForthVm* vm ) {
    vm->input_buffer->index = 0;
    FILL( vm->input_buffer->base, 0, vm->input_buffer->capacity );
};

void askf_read_input_blocking( AskForthVm* vm  ) {
    #ifdef TARGET_LINUX
        int res = read( STDIN_FILENO, 
                        ( vm->input_buffer->base + vm->input_buffer->index     ) , 
                        ( vm->input_buffer->capacity - vm->input_buffer->index ) );
        if ( res == 0 )
            return;

        vm->input_buffer->index += + res;
        vm->input_buffer->base[vm->input_buffer->index] = '\0';
    #endif

}

void askf_print( ascii* buff, u32 len ) {
    #ifdef TARGET_LINUX
        fprintf(stdout, "%*s", len, buff);
        fflush(stdout);
    #endif
}
