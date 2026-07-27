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

AskForth_Word* askf_library_find( AskForthVm* vm, AskForthToken* token ) {
    AskForth_Library* lib = ( AskForth_Library*) vm->lib;

    AskForth_Dictionary* base_dic = lib->dictionaries_base;

    while ( base_dic != NULL ) {
        AskForth_Word* base_word = base_dic->words_base;

        while ( base_word != NULL ) {

            if ( token->length != base_word->name_len )
                goto skip_token;

            for (u64 x = 0; x < token->length; x++) 
                if ( token->base[x] != base_word->name[x] )
                    goto skip_token;

            return base_word;

            skip_token:
            base_word = ( AskForth_Word* ) base_word->next;
        }

        base_dic = ( AskForth_Dictionary* ) base_dic->next;
    }

    return NULL;
}
