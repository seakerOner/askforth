#include "library.h"
#include "../vm/forth_vm.h"

#include "../errors/error_thrower.h"
#include "../errors/errors.h"
#include "../loader/loader.h"

AskForth_Library* askf_create_library( AskForthVm* vm ) {
    AskForth_Library* lib = NULL;

    askf_align_ram( vm );

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

    askf_align_ram( vm );
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

    askf_align_ram( vm );
    lib->dictionaries_base->name = askf_alloc( sizeof(ascii) * (lib->dictionaries_base->name_len+1) );
    COPY( core_dic_name, lib->dictionaries_base->name, (lib->dictionaries_base->name_len+1) );

    lib->dictionaries_base->words_base  = NULL;
    lib->dictionaries_base->next        = NULL;
    lib->dictionaries_base->recent_word = NULL;

    return lib;
}

AskForth_ShadowLibrary* askf_create_shadowlibrary( AskForthVm* vm ) {
    AskForth_ShadowLibrary* lib = NULL;

    askf_align_ram( vm );

    askf_set_recoverable_blob_offset( vm );
    lib = ( AskForth_ShadowLibrary * ) askf_blob_alloc( vm->ram, sizeof( AskForth_ShadowLibrary ) );

    if ( lib == NULL ) {
        AskForthError err = 
            {   .zone = ASKF_ERROR_ZONE_OUTER, 
                .error = ASKF_ERROR_FAILED_LIB_ALLOC,
                .opt_message = NULL
            };
        askf_throw_error( err );
        return NULL;
    }
    lib->dict_base = NULL;
    lib->word_base = NULL;

    return lib;
}

AskForth_Dictionary*    askf_create_dic( AskForthVm* vm, ascii* name, u64 name_len ) {
    AskForth_Library*    lib        = ( AskForth_Library* )vm->lib;
    AskForth_Dictionary* dic        = lib->recent_dic;
    AskForth_Dictionary* new_dic    = NULL;

    askf_align_ram( vm );
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
    new_dic->prev        = dic;
    new_dic->name        = name;
    new_dic->name_len    = name_len;

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

AskForth_Word* askf_library_find_word_from_dic( AskForth_Dictionary* dic, AskForthToken* token ) {
    AskForth_Word* base_word = dic->recent_word;

    while ( base_word != NULL ) {
        if ( token->length != base_word->name_len )
            goto skip_word;

        for (u64 x = 0; x < token->length; x++) 
            if ( token->base[x] != base_word->name[x] )
                goto skip_word;

        return base_word;

        skip_word:
        base_word = base_word->prev;
    }

    return NULL;
}

boolean askf_dic_add_word_native( 
        AskForthToken dic_name, 
        boolean is_immediate,
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

    askf_align_ram( vm );
    AskForth_Word* new_word = askf_alloc( sizeof( AskForth_Word ) );

    if ( !dic->words_base ) {
        dic->words_base = new_word;
    }
    if ( dic->recent_word ) 
        dic->recent_word->next              = new_word;

    new_word->prev                      = dic->recent_word;
    dic->recent_word                    = new_word;

    new_word->next                      = NULL;
    new_word->is_immediate              = is_immediate;

    new_word->source.type               = ASKF_WORD_NATIVE;
    new_word->source.source.native_code = native_subroutine;

    new_word->dict_origin               = dic;
    new_word->name_len                  = word_name.length;
    new_word->name                      = word_name.base;

    return TRUE;
}

boolean askf_dic_add_word_foreign_native( 
        AskForthToken dic_name, 
        AskForthForeignFuncSig* signature,
        AskForthToken word_name ) {

    AskForthVm* vm = askf_get_global_vm();
    
    if ( word_name.length > ASKF_MAX_NAME_LEN ) {
        return FALSE;
    }

    AskForth_Dictionary* dic =  askf_library_find_dic( vm, &dic_name );

    if ( !dic )
        return FALSE;

    askf_align_ram( vm );
    AskForth_Word* new_word = askf_alloc( sizeof( AskForth_Word ) );

    if ( !dic->words_base ) {
        dic->words_base = new_word;
    }
    if ( dic->recent_word ) 
        dic->recent_word->next          = new_word;

    new_word->prev                      = dic->recent_word;
    dic->recent_word                    = new_word;

    new_word->next                      = NULL;
    new_word->is_immediate              = FALSE;

    new_word->source.type               = ASKF_WORD_NATIVE_FOREIGN;

    askf_align_ram( vm );
    new_word->source.source.foreign_sig = askf_alloc( sizeof( AskForthForeignFuncSig ) );
    COPY( signature, new_word->source.source.foreign_sig, sizeof( AskForthForeignFuncSig ) );

    new_word->name_len                  = word_name.length;
    new_word->dict_origin               = dic;
    new_word->name                      = word_name.base;

    return TRUE;

}


boolean askf_dic_add_word_threaded( AskForth_Dictionary* dic, AskForthToken word_name ) {
    AskForthVm* vm = askf_get_global_vm();
    
    // TODO: throw error
    if ( !vm )
        return FALSE;


    // TODO: throw error
    if ( word_name.length > ASKF_MAX_NAME_LEN ) {
        return FALSE;
    }

    askf_align_ram( vm );
    AskForth_Word* new_word = askf_alloc( sizeof( AskForth_Word ) );

    if ( !dic->words_base ) {
        dic->words_base = new_word;
    }
    if ( dic->recent_word ) 
        dic->recent_word->next              = new_word;

    new_word->prev                      = dic->recent_word;
    dic->recent_word                    = new_word;

    new_word->next                      = NULL;
    new_word->is_immediate              = FALSE;

    new_word->source.type               = ASKF_WORD_THREADED;
    askf_align_ram( vm );
    new_word->source.source.threaded_code_start_addr = (u64)askf_alloc( sizeof(u64) );

    new_word->name_len                  = word_name.length;
    new_word->name                      = word_name.base;

    ( (AskForth_Library*)vm->lib )->curr_compiling.word = new_word;
    ( (AskForth_Library*)vm->lib )->curr_compiling.dic  = dic;
    ( (AskForth_Library*)vm->lib )->curr_compiling.here = 
        (u64*)new_word->source.source.threaded_code_start_addr;

    new_word->dict_origin               = dic;
    return TRUE;
}

boolean askf_compile_threaded_memory( u64 val ) {
    AskForthVm* vm = askf_get_global_vm();

    if ( !((AskForth_Library*)vm->lib)->curr_compiling.here ) {
        return FALSE;
    }

    *((AskForth_Library*)vm->lib)->curr_compiling.here = val;
    (( AskForth_Library* )vm->lib)->curr_compiling.here = 
                askf_alloc( sizeof( u64 ) );

    if ( !((AskForth_Library*)vm->lib)->curr_compiling.here ) {
        return FALSE;
    }

    return TRUE;
}
