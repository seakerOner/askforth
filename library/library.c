#include "library.h"
#include "../vm/forth_vm.h"

#include "../errors/error_thrower.h"
#include "../errors/errors.h"

AskForth_Library* askf_create_library( AskForthVm* vm ) {
    AskForth_Library* lib = NULL;

    lib = ( AskForth_Library * ) askf_blob_alloc( vm->ram, sizeof( AskForth_Library ) );

    if ( lib == NULL ) {
        AskForthError err = 
            {   .zone = ASKF_ERROR_ZONE_OUTER, 
                .error = ASKF_ERROR_FAILED_LIB_ALLOC,
                .opt_message = NULL
            };
        askf_throw_error( err );
        return NULL;
    }

    lib->dictionaries_base = ( AskForth_Dictionary* ) askf_blob_alloc( vm->ram, sizeof( AskForth_Dictionary ) );

    if ( lib->dictionaries_base == NULL ) {
        AskForthError err = 
            {   .zone = ASKF_ERROR_ZONE_OUTER, 
                .error = ASKF_ERROR_FAILED_CORE_DIC_ALLOC ,
                .opt_message = NULL
            };
        askf_throw_error( err );

        return NULL;
    }

    lib->recent_dic = lib->dictionaries_base;

    ascii core_dic_name[ASKF_MAX_NAME_LEN] = "core";
    lib->dictionaries_base->name_len = 4;

    COPY( core_dic_name, lib->dictionaries_base->name, ASKF_MAX_NAME_LEN );

    lib->dictionaries_base->words_base  = NULL;
    lib->dictionaries_base->next        = NULL;
    lib->dictionaries_base->recent_word = NULL;

    return lib;
}

AskForth_Dictionary*    askf_create_dic( AskForthVm* vm, ascii name[ASKF_MAX_NAME_LEN], u64 name_len ) {
    AskForth_Library*    lib        = ( AskForth_Library* )vm->lib;
    AskForth_Dictionary* dic        = lib->recent_dic;
    AskForth_Dictionary* new_dic    = NULL;

    new_dic = ( AskForth_Dictionary* )askf_blob_alloc( vm->ram, sizeof( AskForth_Dictionary ) );

    if ( new_dic == NULL ) {
        AskForthError err = 
            {   .zone = ASKF_ERROR_ZONE_OUTER, 
                .error = ASKF_ERROR_FAILED_DIC_ALLOC,
                .opt_message = NULL
            };

        askf_throw_error( err );

        return NULL;
    }

    new_dic->next        = NULL;
    new_dic->recent_word = NULL;
    new_dic->words_base  = NULL;
    COPY(name, new_dic->name, name_len );
    new_dic->name_len = name_len;

    dic->next = new_dic;
    lib->recent_dic = new_dic;


    return dic;
}

AskForth_Dictionary* askf_library_find_dic( AskForthVm* vm, AskForthToken* token ) {
    AskForth_Library* lib = ( AskForth_Library*) vm->lib;

    AskForth_Dictionary* base_dic = lib->dictionaries_base;

    while ( base_dic != NULL ) {
        if ( token->length > base_dic->name_len )
            goto skip_dic;

        for (u64 x = 0; x < base_dic->name_len; x++)
            if ( base_dic->name[x] != token->base[x] )
                goto skip_dic;

        return base_dic;

        skip_dic:
        base_dic = ( AskForth_Dictionary* ) base_dic->next;
    }

    return NULL;
}

AskForth_Word* askf_library_find_word( AskForthVm* vm, AskForthToken* token ) {
    AskForth_Library* lib = ( AskForth_Library*) vm->lib;

    AskForth_Dictionary* base_dic = lib->dictionaries_base;

    while ( base_dic != NULL ) {
        AskForth_Word* base_word = base_dic->recent_word;

        while ( base_word != NULL ) {

            if ( token->length != base_word->name_len )
                goto skip_token;

            for (u64 x = 0; x < token->length; x++) 
                if ( token->base[x] != base_word->name[x] )
                    goto skip_token;

            return base_word;

            skip_token:
            base_word = ( AskForth_Word* ) base_word->prev;
        }

        base_dic = ( AskForth_Dictionary* ) base_dic->next;
    }

    return NULL;
}

boolean askf_dic_add_word_native( 
        AskForthToken dic_name, 
        void(*native_subroutine)(void), 
        AskForthToken word_name ) {

    AskForthVm* vm = askf_get_global_vm();
    
    // TODO: throw error
    if ( !vm )
        return FALSE;


    // TODO: throw error
    if ( word_name.length > ASKF_MAX_NAME_LEN ) {
        return FALSE;
    }

    AskForth_Dictionary* dic =  askf_library_find_dic( vm, &dic_name );

    // TODO: throw error
    if ( !dic )
        return FALSE;

    AskForth_Word* new_word = askf_alloc( sizeof( AskForth_Word ) );

    new_word->prev                      = dic->recent_word;
    new_word->next                      = NULL;
    dic->recent_word                    = new_word;

    new_word->source.type               = ASKF_WORD_NATIVE;
    new_word->source.source.native_code = native_subroutine;

    new_word->name_len                  = word_name.length;
    COPY(word_name.base, new_word->name, word_name.length);

    return TRUE;
}
