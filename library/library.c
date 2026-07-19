#include "library.h"
#include "../vm/forth_vm.h"

AskForth_Library* askf_create_library( AskForthVm* vm ) {
    AskForth_Library* lib = NULL;

    lib = ( AskForth_Library * ) askf_blob_alloc( vm->ram, sizeof( AskForth_Library ) );

    if ( lib == NULL ) {
        AskForthError err = 
            {   .zone = ASKF_ERROR_ZONE_OUTER, 
                .error = ASKF_ERROR_FAILED_LIB_ALLOC 
            };
        askf_throw_error( err );
        return NULL;
    }

    lib->dictionaries_base = ( AskForth_Dictionary* ) askf_blob_alloc( vm->ram, sizeof( AskForth_Dictionary ) );

    if ( lib->dictionaries_base == NULL ) {
        AskForthError err = 
            {   .zone = ASKF_ERROR_ZONE_OUTER, 
                .error = ASKF_ERROR_FAILED_CORE_DIC_ALLOC 
            };
        askf_throw_error( err );

        return NULL;
    }

    lib->recent_dic = lib->dictionaries_base;

    ascii core_dic_name[ASKF_MAX_NAME_LEN] = "Core";

    COPY( core_dic_name, lib->dictionaries_base->name, ASKF_MAX_NAME_LEN );

    lib->dictionaries_base->words_base  = NULL;
    lib->dictionaries_base->next        = NULL;
    lib->dictionaries_base->recent_word = NULL;

    return lib;
}

AskForth_Dictionary* askf_create_dic( AskForthVm* vm, ascii name[ASKF_MAX_NAME_LEN] ) {
    AskForth_Library*    lib        = ( AskForth_Library* )vm->lib;
    AskForth_Dictionary* dic        = lib->recent_dic;
    AskForth_Dictionary* new_dic    = NULL;

    new_dic = ( AskForth_Dictionary* )askf_blob_alloc( vm->ram, sizeof( AskForth_Dictionary ) );

    if ( new_dic == NULL ) {
        AskForthError err = 
            {   .zone = ASKF_ERROR_ZONE_OUTER, 
                .error = ASKF_ERROR_FAILED_DIC_ALLOC 
            };
        askf_throw_error( err );

        return NULL;
    }

    new_dic->next        = NULL;
    new_dic->recent_word = NULL;
    new_dic->words_base  = NULL;
    COPY(name, new_dic->name, ASKF_MAX_NAME_LEN );

    dic->next = new_dic;
    lib->recent_dic = new_dic;


    return dic;
}
