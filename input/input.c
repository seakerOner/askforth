#include "input.h"

#if defined( TARGET_LINUX )
    #include "unistd.h"
    #include "stdio.h"
#elif defined( TARGET_WINDOWS )
    #include <windows.h>
    #include "stdio.h"
#endif

void askf_read_input_blocking( AskForthVm* vm  ) {
    vm->input_buffer->index   = 0;
    vm->istack->sources[0].in = 0;

    // TODO: use proper way to read input ( move back characters, delete and what not )
    #if defined( TARGET_LINUX )
        int res = read( STDIN_FILENO, 
                         vm->input_buffer->base  , 
                         vm->input_buffer->capacity  );
    #elif defined( TARGET_WINDOWS )
        HANDLE askf_stdin = GetStdHandle( STD_INPUT_HANDLE );

        DWORD res = 0;

        if ( !ReadFile( askf_stdin, 
                     vm->input_buffer->base ,
                    (DWORD) vm->input_buffer->capacity ,
                    &res, NULL) ) {
            return;
        }

    #endif
        if ( res == 0 )
            return;

        vm->input_buffer->index += res;
        vm->istack->sources[0].in_max = vm->input_buffer->index;
        vm->input_buffer->base[vm->input_buffer->index] = '\0';
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
        fprintf(stdout, "%*s", len, buff);
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
    u8 radix          = askf_get_global_vm()->num_base;
    ascii* flag       = NULL;
    boolean is_signed = *cell->is_signed;
    
    switch ( radix ) {
        case ASKF_OCTAL:
            flag = (ascii*)"%llo";
            break;
        default:
        case ASKF_DECIMAL:
            if ( is_signed ) 
                flag = (ascii*)"%lld";
            else
                flag = (ascii*)"%llu";
            break;
        case ASKF_HEXADECIMAL:
            flag = (ascii*)"%llx";
            break;
    }
    #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
        if ( is_signed ) {
            fprintf( stdout, (const char*)flag, cell->val._64s );
        }else {
            fprintf( stdout, (const char*)flag, cell->val._64u );
        }
    #endif
}
