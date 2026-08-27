#include "input.h"

#if defined( TARGET_LINUX )
    #include "unistd.h"
    #include "stdio.h"
#elif defined( TARGET_WINDOWS )
    #include <windows.h>
    #include "stdio.h"
#endif

void askf_reset_input_buffer( AskForthVm* vm, AskForthParseType parse_type ) {
    switch ( parse_type ) {
        case ASKF_MAIN_PARSER:
            vm->input_buffer->index = 0;
            FILL( vm->input_buffer->base, 0, vm->input_buffer->capacity );
            break;
        case ASKF_X_PARSER:
            vm->input_buffer_x->index = 0;
            FILL( vm->input_buffer_x->base, 0, vm->input_buffer_x->capacity );
            break;
        default:
            break;
    }
};

void askf_read_input_blocking( AskForthVm* vm  ) {
    // TODO: use proper way to read input ( move back characters, delete and what not )
    #if defined( TARGET_LINUX )
        int res = read( STDIN_FILENO, 
                        ( vm->input_buffer->base + vm->input_buffer->index     ) , 
                        ( vm->input_buffer->capacity - vm->input_buffer->index ) );
        if ( res == 0 )
            return;

        vm->input_buffer->index += res;
        vm->input_buffer->base[vm->input_buffer->index] = '\0';
    #elif defined( TARGET_WINDOWS )
        HANDLE askf_stdin = GetStdHandle( STD_INPUT_HANDLE );

        DWORD res = 0;

        if ( !ReadFile( askf_stdin, 
                    ( vm->input_buffer->base + vm->input_buffer->index ),
                    (DWORD)( vm->input_buffer->capacity - vm->input_buffer->index ),
                    &res, NULL) ) {
            return;
        }

        if ( res == 0 )
            return;

        vm->input_buffer->index += res;
        vm->input_buffer->base[vm->input_buffer->index] = '\0';
    #endif
}

u32 askf_read_input_blocking_tobuff( AskForthVm* vm, ascii* buffer, u64 cap ) {
    UNUSED( vm );
    #if defined( TARGET_LINUX )
        int res = read( STDIN_FILENO, 
                        buffer,
                        cap );

        return res;
    #elif defined( TARGET_WINDOWS )
        HANDLE askf_stdin = GetStdHandle( STD_INPUT_HANDLE );

        DWORD res = 0;

        if ( !ReadFile( askf_stdin, 
                    buffer,
                    (DWORD)cap,
                    &res, NULL) ) {
            return 0;
        }

        return (u32)res;
    #endif
}

void askf_print( ascii* buff, u32 len ) {
    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        fprintf(stdout, "%.*s", len, buff);
        fflush(stdout);
    #endif
}

void askf_print_char( ascii _char ) {
    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        fprintf(stdout, "%c", _char);
        fflush(stdout);
    #endif
}

void askf_print_cell( AskForth_Cell* cell ) {
    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        if (cell->is_signed) {
            fprintf( stdout, "%lld", cell->val._64s );
        }else {
            fprintf( stdout, "%llu", cell->val._64u );
        }
    #endif
}
