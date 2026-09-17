#include "load_extern_library.h"

#include "../vm/forth_vm.h"
#include <stdlib.h>
#include "../input/input.h"

#if defined( TARGET_LINUX )
    static void* askf_load_foreign_object_linux( const ascii* obj_name ) {
        return dlopen( ( const char* )obj_name, RTLD_NOW );
    }
#elif defined( TARGET_WINDOWS )
    static HMODULE askf_load_foreign_object_windows( const ascii* obj_name ) {
        return LoadLibraryA( ( const char* )obj_name );
    }
#endif

boolean askf_load_foreign_object( const ascii* obj_name, u64 len ) {
    AskForthForeignManager* manager = askf_get_global_vm()->foreign_manager;

    ascii* saved_name = malloc( sizeof( ascii ) * len+1 );
    COPY( obj_name, saved_name, len );
    saved_name[len] = '\0';

    #if defined( TARGET_LINUX )
        void* obj_handle   = askf_load_foreign_object_linux( saved_name );
    #elif defined( TARGET_WINDOWS )
        HMODULE obj_handle = askf_load_foreign_object_windows( saved_name );
    #endif

    if ( obj_handle == NULL )  {
        free( saved_name );
        return FALSE;
    }

    AskForthForeignObject* previous_obj =  NULL;

    if ( manager->recent_obj )
        previous_obj = manager->recent_obj;

    manager->recent_obj = malloc( sizeof( AskForthForeignObject ) );

    #if defined( TARGET_LINUX )
        manager->recent_obj->linux_handle   = obj_handle;
    #elif defined( TARGET_WINDOWS )
        manager->recent_obj->windows_handle = obj_handle;
    #endif

    manager->recent_obj->name_len = len;
    manager->recent_obj->name     = saved_name;

    if ( !manager->objects ) 
        manager->objects = manager->recent_obj;

    if ( previous_obj ) {
        manager->recent_obj->prev = previous_obj;
        previous_obj->next        = manager->recent_obj;
    }

    return TRUE;
}

boolean askf_bind_function( const ascii* func_name, u64 func_name_len, AskForthForeignFuncSig* signature  ) {
    AskForthForeignManager* manager = askf_get_global_vm()->foreign_manager;

    ascii* saved_string = malloc( sizeof(ascii) * func_name_len+1 );
    COPY( func_name, saved_string, func_name_len );
    saved_string[func_name_len] = '\0';

    AskForthForeignObject* base = manager->recent_obj;

    while ( base ) {
        #if defined( TARGET_LINUX )
            void* func = dlsym( base->linux_handle, ( const char* )saved_string );
        #elif defined( TARGET_WINDOWS )
            FARPROC func = GetProcAddress( base->windows_handle, ( const char* )saved_string );
        #endif

        if ( !func )
            goto skip_obj;

        free( saved_string );
        signature->handle = ( void* )func;
        return TRUE;

        skip_obj:
        base = base->prev;
    }

    free( saved_string );
    return FALSE;
}

boolean askf_free_foreign_object( const ascii* obj_name, u64 len ) {
    AskForthForeignManager* manager = askf_get_global_vm()->foreign_manager;

    AskForthForeignObject* base = manager->recent_obj;

    while ( base ) {
        if ( base->name_len != len )
            goto next_obj;

        for ( askf_addr_t x = 0; x < len; x++ ) 
            if ( base->name[x] != obj_name[x] )
                goto next_obj;

        if ( base->prev && base->next ) {
            base->prev->next = base->next;
            base->next->prev = base->prev;
        } else if ( base->prev && !base->next ) {
            base->prev->next = NULL;
        } else if ( !base->prev && !base->next ) {
            manager->objects    = NULL;
            manager->recent_obj = NULL;
        }

        #if defined( TARGET_LINUX )
            dlclose( base->linux_handle );
        #elif defined( TARGET_WINDOWS )
            FreeLibrary( base->windows_handle );
        #endif


        free( base->name );
        free( base );

        return TRUE;;

        next_obj:
        base = base->prev;
    }

    return FALSE;
}


void switch_context( void* rsp, AskForthForeignFuncSig* signature );

void switch_to_forth_ctx_C( u64 result ) {
    AskForthVm* vm = askf_get_global_vm();
    vm->stack->cells.space_64[vm->stack->index++] = result;
    switch_context( NULL, NULL );
}


// x86_64 Windows specific implementation
#if defined( ARQBITS64 ) && defined( TARGET_WINDOWS )
void __attribute__((naked)) trampoline_forth_ctx_x86_64_winv( AskForthForeignFuncSig* signature ) {
    __asm__ __volatile__(
            "pushq %rbp\n"  // save context registers on stack ( calle )
            "pushq %rbx\n"
            "pushq %rdi\n"
            "pushq %rsi\n"
            "pushq %r12\n"
            "pushq %r13\n"
            "pushq %r14\n"
            "pushq %r15\n"
            "movq  %rcx, %rdx\n" // move 'signature' from 1st argument to 2nd argument
            "movq  %rsp, %rcx\n" // stack pointer to 1st argument
            "jmp switch_context");
}

void __attribute__((naked)) remake_forth_ctx_x86_64_winv( void* rsp ) {
    __asm__ __volatile__(
            "movq %rcx, %rsp \n"   // set stack pointer from saved context
            "popq %r15\n"
            "popq %r14\n"
            "popq %r13\n"
            "popq %r12\n"
            "popq %rsi\n"
            "popq %rdi\n"
            "popq %rbx\n"
            "popq %rbp\n"
            "ret\n");
}

#define POP_TO_REG( reg )       \
            "testq %rax, %rax\n" \
            "jz .no_args\n"     \
            "decq %rax\n"       \
            "popq %" #reg "\n"  \


void __attribute__((naked)) enter_foreign_ctx_x86_64_winv( void* rsp, u64 arg_count ) {
    __asm__ __volatile__(
            "movq %rcx, %rsp \n"   // set stack pointer from saved context
            // skip padding if flag true
            "popq %rax\n"
            "testq %rax, %rax\n"
            "jz .no_padding\n"
            "addq $8, %rsp\n"

            ".no_padding:\n"
            "movq %rdx, %rax \n"   // set argument count to RAX
                                  
            "popq %r15\n"
            "popq %r14\n"
            "popq %r13\n"
            "popq %r12\n"
            "popq %rsi\n"
            "popq %rdi\n"
            "popq %rbx\n"
            "popq %rbp\n"

            // decrease RAX until its 0 and store stack->correspondent registers in order
            POP_TO_REG( rcx )
            POP_TO_REG( rdx )
            POP_TO_REG( r8  )
            POP_TO_REG( r9  )
            // 5th argument and above are stored on the stack already so no need to pop it

            ".no_args:\n"
            "ret\n");
}

 void __attribute__((naked)) switch_to_forth_ctx_x86_64_winv( void ) {
    __asm__ __volatile__(
            "movq %rax, %rcx\n"
            "jmp switch_to_forth_ctx_C\n");
 }

void set_foreign_call_for_trampoline_ctx_x86_64_winv
    ( AskForthContextManager* manager, void* handle, AskForthForeignFuncSig* signature ) {
    AskForthVm* vm = askf_get_global_vm();
    void** rsp = manager->trampoline_ctx.rsp;

    // stack args (5th+ args) have to be placed AFTER the foreign function's return address 
    // since we are building backwords, we do them first 
    u64 extraparams_to_remove = 0;
    if ( signature->count > 4  ) {
        for ( u64 x = signature->count; x > 4; x-- ) {
            *(--rsp) = (void*)vm->stack->cells.space_64[vm->stack->index - 1 - x];
            extraparams_to_remove++;
        }
        vm->stack->index -= (u8)extraparams_to_remove;
    }

    // shadow space: 32 bytes
    *(--rsp) = 0;
    *(--rsp) = 0;
    *(--rsp) = 0;
    *(--rsp) = 0;

    // always aligned
    *(--rsp) = switch_to_forth_ctx_x86_64_winv;    // ret after function
    *(--rsp) = handle;                             // foreign func to 'ret'

    if ( signature->args[0] == ASKF_ARG_VOID )  {
        *(--rsp) = 0; // rbp
        *(--rsp) = 0; // rbx
        *(--rsp) = 0; // rdi
        *(--rsp) = 0; // rsi
        *(--rsp) = 0; // r12
        *(--rsp) = 0; // r13
        *(--rsp) = 0; // r14
        *(--rsp) = 0; // r15
        *(--rsp) = 0; // no padding flag
    } else {
        for ( u64 x = 0; x < signature->count - extraparams_to_remove; x++ ) 
            *(--rsp) = (void*)vm->stack->cells.space_64[vm->stack->index-1-x];

        // 16-byte WinV alignment   
        // add 8-byte padding when the number of register arguments is odd
        u64 padding_if_odd = ( signature->count - extraparams_to_remove ) & 1;

        vm->stack->index -= (u8)(signature->count - extraparams_to_remove);
        *(--rsp) = 0; // rbp
        *(--rsp) = 0; // rbx
        *(--rsp) = 0; // rdi
        *(--rsp) = 0; // rsi
        *(--rsp) = 0; // r12
        *(--rsp) = 0; // r13
        *(--rsp) = 0; // r14
        *(--rsp) = 0; // r15
                      
        if ( padding_if_odd ) {
            *(--rsp) = 0;
            *(--rsp) = (void*)1;
        } else {
            *(--rsp) = 0;
        }
    }

    manager->trampoline_ctx.rsp = rsp;
}
#endif

// x86_64 SystemV specific implementation
#if defined( ARQBITS64 ) && defined( TARGET_LINUX )

void __attribute__((naked)) trampoline_forth_ctx_x86_64_sysv( AskForthForeignFuncSig* signature ) {
    __asm__ __volatile__(
            "pushq %rbp\n"  // save context registers on stack ( calle )
            "pushq %rbx\n"
            "pushq %r12\n"
            "pushq %r13\n"
            "pushq %r14\n"
            "pushq %r15\n"
            "movq  %rdi, %rsi\n" // move 'signature' from 1st argument to 2nd argument
            "movq  %rsp, %rdi\n" // stack pointer to 1st argument
            "jmp switch_context");
}

void __attribute__((naked)) remake_forth_ctx_x86_64_sysv( void* rsp ) {
    __asm__ __volatile__(
            "movq %rdi, %rsp \n"   // set stack pointer from saved context
            "popq %r15\n"
            "popq %r14\n"
            "popq %r13\n"
            "popq %r12\n"
            "popq %rbx\n"
            "popq %rbp\n"
            "ret\n");
}

#define POP_TO_REG( reg )       \
            "testq %rax, %rax\n" \
            "jz .no_args\n"     \
            "decq %rax\n"       \
            "popq %" #reg "\n"  \


void __attribute__((naked)) enter_foreign_ctx_x86_64_sysv( void* rsp, u64 arg_count ) {
    __asm__ __volatile__(
            "movq %rdi, %rsp \n"   // set stack pointer from saved context
            // skip padding if flag true
            "popq %rax\n"
            "testq %rax, %rax\n"
            "jz .no_padding\n"
            "addq $8, %rsp\n"

            ".no_padding:\n"
            "movq %rsi, %rax \n"   // set argument count to RAX
                                  
            "popq %r15\n"
            "popq %r14\n"
            "popq %r13\n"
            "popq %r12\n"
            "popq %rbx\n"
            "popq %rbp\n"

            // decrease RAX until its 0 and store stack->correspondent registers in order
            POP_TO_REG( rdi )
            POP_TO_REG( rsi )
            POP_TO_REG( rdx )
            POP_TO_REG( rcx )
            POP_TO_REG( r8  )
            POP_TO_REG( r9  )
            // 7th argument and above are stored on the stack already so no need to pop it

            ".no_args:\n"
            "ret\n");
}


 void __attribute__((naked)) switch_to_forth_ctx_x86_64_sysv( void ) {
    __asm__ __volatile__(
            "movq %rax, %rdi\n"
            "jmp switch_to_forth_ctx_C\n");
 }

void set_foreign_call_for_trampoline_ctx_x86_64_sysv
    ( AskForthContextManager* manager, void* handle, AskForthForeignFuncSig* signature ) {
    AskForthVm* vm = askf_get_global_vm();
    void** rsp = manager->trampoline_ctx.rsp;

    // stack args (7th+ args) have to be placed AFTER the foreign function's return address 
    // since we are building backwords, we do them first 
    u64 extraparams_to_remove = 0;
    if ( signature->count > 6  ) {
        for ( u64 x = signature->count; x > 6; x-- ) {
            *(--rsp) = (void*)vm->stack->cells.space_64[vm->stack->index - 1 - x];
            extraparams_to_remove++;
        }
        vm->stack->index -= (u8)extraparams_to_remove;
    }

    // always aligned
    *(--rsp) = switch_to_forth_ctx_x86_64_sysv;    // ret after function
    *(--rsp) = handle;                             // foreign func to 'ret'

    if ( signature->args[0] == ASKF_ARG_VOID )  {
        *(--rsp) = 0; // rbp
        *(--rsp) = 0; // rbx
        *(--rsp) = 0; // r12
        *(--rsp) = 0; // r13
        *(--rsp) = 0; // r14
        *(--rsp) = 0; // r15
        *(--rsp) = 0; // no padding flag
    } else {
        for ( u64 x = 0; x < signature->count - extraparams_to_remove; x++ ) 
            *(--rsp) = (void*)vm->stack->cells.space_64[vm->stack->index-1-x];

        // 16-byte SysV alignment   
        // add 8-byte padding when the number of register arguments is odd
        u64 padding_if_odd = ( signature->count - extraparams_to_remove ) & 1;

        vm->stack->index -= (u8)(signature->count - extraparams_to_remove);
        *(--rsp) = 0; // rbp
        *(--rsp) = 0; // rbx
        *(--rsp) = 0; // r12
        *(--rsp) = 0; // r13
        *(--rsp) = 0; // r14
        *(--rsp) = 0; // r15
                      
        if ( padding_if_odd ) {
            *(--rsp) = 0;
            *(--rsp) = (void*)1;
        } else {
            *(--rsp) = 0;
        }
    }

    manager->trampoline_ctx.rsp = rsp;
}
#endif

void _remake_ctx_stack_trampolin( AskForthContextManager* manager ) {
    if ( !manager->trampoline_ctx.stack )
        manager->trampoline_ctx.stack = malloc( ASKF_CONTEXT_STACK_SIZE );

    #if defined( ARQBITS64 )
        #if defined( TARGET_LINUX ) || defined( TARGET_WINDOWS )
            askf_addr_t top = (askf_addr_t)manager->trampoline_ctx.stack + ASKF_CONTEXT_STACK_SIZE;
            top &= ~0xF;    // Align to 16 bytes to able to use 'ret' on  x86_64 SysV and x86_64 WinV
            manager->trampoline_ctx.rsp = ( void * )top;
        #endif
    #endif
}

void switch_context( void* rsp, AskForthForeignFuncSig* signature ) {
    AskForthForeignManager* manager = askf_get_global_vm()->foreign_manager;
    switch ( manager->ctxs.state ) {
        case ASKF_CTX_MAIN:
            manager->ctxs.main_ctx.rsp   = rsp;
            manager->ctxs.state = ASKF_CTX_TRAMPOLINE;
            _remake_ctx_stack_trampolin( &manager->ctxs );

            #if defined( ARQBITS64 )
                #if defined( TARGET_LINUX ) 
                    set_foreign_call_for_trampoline_ctx_x86_64_sysv( &manager->ctxs, signature->handle, signature );
                    u64 arg_count = signature->args[0] == ASKF_ARG_VOID ? 0 : ( signature->count > 6 ? 6 : signature->count );

                    enter_foreign_ctx_x86_64_sysv(manager->ctxs.trampoline_ctx.rsp, arg_count );
                #elif defined( TARGET_WINDOWS ) 
                    set_foreign_call_for_trampoline_ctx_x86_64_winv( &manager->ctxs, signature->handle, signature );
                    u64 arg_count = signature->args[0] == ASKF_ARG_VOID ? 0 : ( signature->count > 4 ? 4 : signature->count );

                    enter_foreign_ctx_x86_64_winv(manager->ctxs.trampoline_ctx.rsp, arg_count );
                #endif
            #endif
            break;
        case ASKF_CTX_TRAMPOLINE:
            manager->ctxs.state = ASKF_CTX_MAIN;

            #if defined( ARQBITS64 )
                #if defined( TARGET_LINUX ) 
                    remake_forth_ctx_x86_64_sysv( manager->ctxs.main_ctx.rsp );
                #elif defined( TARGET_WINDOWS ) 
                    remake_forth_ctx_x86_64_winv( manager->ctxs.main_ctx.rsp );
                #endif
            #endif
            break;
        default: break;
    }
}

boolean askf_trampoline( AskForthForeignFuncSig* signature ) {
    if ( askf_get_global_vm()->stack->index < signature->count) 
        return FALSE;

    #if defined( ARQBITS64 )
        #if defined( TARGET_LINUX ) 
            trampoline_forth_ctx_x86_64_sysv( signature );
        #elif defined( TARGET_WINDOWS )
            trampoline_forth_ctx_x86_64_winv( signature );
        #endif
    #endif

    return TRUE;
}
