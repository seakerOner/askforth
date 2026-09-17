#ifndef ASKF_LOAD_EXTERN_LIBRARY_H
#define ASKF_LOAD_EXTERN_LIBRARY_H

#include "../inttype.h"

#if defined( TARGET_LINUX )
    #include <dlfcn.h>
#elif defined( TARGET_WINDOWS )
    #include <windows.h>
#endif

#define ASKF_CONTEXT_STACK_SIZE ( 64 * 1024 )

typedef enum {
    ASKF_ARG_U64,
    ASKF_ARG_I64,
    ASKF_ARG_PTR,
    ASKF_ARG_VOID
} AskForth_ArgTypes;

typedef struct {
    AskForth_ArgTypes args[16];
    askf_addr_t          count;
    AskForth_ArgTypes ret_type;

    void* handle;
} AskForthForeignFuncSig;

typedef struct AskForthForeignObject_t {
    #if defined( TARGET_LINUX )
        void*  linux_handle;
    #elif defined( TARGET_WINDOWS )
        HMODULE windows_handle;
    #endif

    ascii* name;
      u64  name_len;

    struct AskForthForeignObject_t* prev;
    struct AskForthForeignObject_t* next;
} AskForthForeignObject;

typedef struct {
    void* rsp;
    void* stack;
} AskForthContext;

typedef enum {
    ASKF_CTX_MAIN,
    ASKF_CTX_TRAMPOLINE
} AskForthContextState;

typedef struct {
    AskForthContext      main_ctx;
    AskForthContext      trampoline_ctx;
    AskForthContextState state;
} AskForthContextManager;

typedef struct {
   AskForthForeignObject* objects;
   AskForthForeignObject* recent_obj;
   AskForthContextManager ctxs;
} AskForthForeignManager;

boolean askf_load_foreign_object( const ascii* obj_name, u64 len );

boolean askf_free_foreign_object( const ascii* obj_name, u64 len );

boolean askf_bind_function( const ascii* func_name, u64 func_name_len, AskForthForeignFuncSig* signature  );

boolean askf_trampoline( AskForthForeignFuncSig* signature );

#endif
