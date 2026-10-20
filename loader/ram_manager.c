/*
 *  so the loader needs to able to read the RAM area it wants to preserve, preserving includes transforming 
 *  all stored addresses into offsets relative to the start of HERE.
 *
 *  and the reverse applies, AskForth on boot needs the loader to be able to read that preserved RAM image and 
 *  turn it into  a operable state, that includes turning the addresses offsets back into new addresses
 *
 *  the loader must also store into the image what foreign functions are foreign libraries are linked
 *  and on boot it must relink them
 *
 *  NOTE: 
 *      The loader will not preserve memory or offset addresses  that are not from the range inside AskForth's RAM.
 *      Example: Addresses given by an external allocator ( Memory-Allocation optional word set )
 *               fileid given by a foreign function       ( File-Access optional word set )
 *
 * -------------------------------------------------------------
 *  
 *  AskForth Ram looks like this when the  executable reaches the interpreter:
 *
 *   ___________________________________  <-- ** RAM START **
 *  |                                   |
 *  |                                   | <--- ram used by internal structures of AskForth's VM, these are initialized every boot by the executable
 *  |-----------------------------------| <--- (RECOVERY POINT): This is where the image starts
 *  |   (8 bytes aligned on 64 BITS)    |                        The recovery start address contains the AskForth's Shadow library
 *  | 32 bytes AskForth_ShadowLibrary   |   
 *  |   -----------------------------   |
 *  |   (8 bytes aligned on 64 BITS)    |                        
 *  | 40 bytes AskForth_Library         |   
 *  |   -----------------------------   |
 *  |   (8 bytes aligned on 64 BITS)    |
 *  | 72 bytes "core" dictionary        |
 *  |   -----------------------------   |
 *  |   (8 bytes aligned on 64 BITS)    | <-- If no footer and image is found on the executable the core words and core dictionary will be added by the 
 *  | 88 bytes each "core" native word  |       executable itsefl
 *  |   core has N words                | 
 *  |   -----------------------------   | 
 *  |         Forth user code           |
 *  |  ** words, foreign funcs, etc **  |
 *  |___________________________________| <-- ** RAM END **
 *
 *  INVARIANT: This must be the RAM structure every time AskForth boots up, with or without an image
 *              ( The "core" dictionary may or may not be listed inside the library, but will always live in memory )
 *
 * -------------------------------------------------------------
 *
 *  A AskForth valid Image will have the following structure:
 *  
 *  AskForthImage _________
 *      |    Image Header  |  ---> metadata and offsets for structures inside the image body
 *      |__________________|
 *      |    Image Body    |  ---> the actual preserved ram 
 *      |__________________|
 *
 *      NOTE: The size of the Image is on the binary's footer
 *            The Image Header and Body doesn't have a fixed size 
 *
 * -------------------------------------------------------------
 *
 *  A AskForth valid Image's Header will have the following structure:
 *
 *  typedef struct {
 *      askf_addr_t  magic;
 *      askf_addr_t header_size;
 *  
 *      askf_addr_t library_offset;
 *      askf_addr_t shadow_library_offset;
 *
 *      askf_addr_t foreign_manager_offset;
 *  } AskForthImageHeader;
 *
 * askf_addr_t is 64 bits on 64 bit arquitectures
 * */

#include "../inttype.h"
#include "../vm/forth_vm.h"
#include "../library/library.h"
#include "../words/askforth_words.h"
#include "loader.h"
#include <stdio.h>

typedef struct {
    askf_addr_t  magic;
    askf_addr_t header_size;
    
    askf_addr_t library_offset;
    askf_addr_t shadow_library_offset;

    askf_addr_t foreign_manager_offset;
} AskForthImageHeader;

typedef struct {
    AskForthImageHeader header;
} AskForthImage;

askf_addr_t __offset_address( askf_addr_t delta, askf_addr_t addr ) {
    return addr - delta;
}

askf_addr_t __relocate_address( askf_addr_t delta, askf_addr_t addr ) {
    return addr + delta;
}

void __offset_library( askf_addr_t delta, AskForth_Library* lib ) {
    if ( lib->dictionaries_base )
        lib->dictionaries_base = (AskForth_Dictionary*)__offset_address( delta, (askf_addr_t)lib->dictionaries_base );

    lib->curr_compiling.dic  = NULL;
    lib->curr_compiling.here = NULL;
    lib->curr_compiling.word = NULL;

    if ( lib->recent_dic )
        lib->recent_dic      = (AskForth_Dictionary*)__offset_address( delta, (askf_addr_t)lib->recent_dic );
}
void __relocate_library( askf_addr_t delta, AskForth_Library* lib ) {
    if ( lib->dictionaries_base )
        lib->dictionaries_base = (AskForth_Dictionary*)__relocate_address( delta, (askf_addr_t)lib->dictionaries_base );

    if ( lib->recent_dic )
        lib->recent_dic      = (AskForth_Dictionary*)__relocate_address( delta, (askf_addr_t)lib->recent_dic );

}
void __offset_shadow_library( askf_addr_t delta, AskForth_ShadowLibrary* lib ) {
    if ( lib->word_base )
        lib->word_base = (AskForth_Word*)__offset_address( delta, (askf_addr_t)lib->word_base );
    if ( lib->dict_base )
        lib->dict_base = (AskForth_Dictionary*)__offset_address( delta, (askf_addr_t)lib->dict_base);
    if ( lib->dict_recent )
        lib->dict_recent = (AskForth_Dictionary*)__offset_address( delta, (askf_addr_t)lib->dict_recent );
    if ( lib->word_recent )
        lib->word_recent = (AskForth_Word*)__offset_address( delta, (askf_addr_t)lib->word_recent );
}
void __relocate_shadow_library( askf_addr_t delta, AskForth_ShadowLibrary* lib ) {
    if ( lib->word_base )
        lib->word_base = (AskForth_Word*)__relocate_address( delta, (askf_addr_t)lib->word_base );
    if ( lib->dict_base )
        lib->dict_base = (AskForth_Dictionary*)__relocate_address( delta, (askf_addr_t)lib->dict_base);
    if ( lib->dict_recent )
        lib->dict_recent = (AskForth_Dictionary*)__relocate_address( delta, (askf_addr_t)lib->dict_recent );
    if ( lib->word_recent )
        lib->word_recent = (AskForth_Word*)__relocate_address( delta, (askf_addr_t)lib->word_recent );
}
void __offset_foreign_manager( askf_addr_t delta, AskForthForeignManager* fm ) {
}
void __relocate_foreign_manager( askf_addr_t delta, AskForthForeignManager* fm ) {
}

void __offset_dictionary( askf_addr_t delta, AskForth_Dictionary* dict ) {
    if ( dict->next )
        dict->next = (AskForth_Dictionary*)__offset_address( delta, (askf_addr_t)dict->next );
    if ( dict->prev )
        dict->prev = (AskForth_Dictionary*)__offset_address( delta, (askf_addr_t)dict->prev );
    if ( dict->recent_word )
        dict->recent_word = (AskForth_Word*)__offset_address( delta, (askf_addr_t)dict->recent_word );
    if ( dict->words_base )
        dict->words_base  = (AskForth_Word*)__offset_address( delta, (askf_addr_t)dict->words_base );
    if ( dict->name ) 
        dict->name = (ascii*)__offset_address( delta, (askf_addr_t)dict->name );
}

void __relocate_dictionary( askf_addr_t delta, AskForth_Dictionary* dict ) {
    if ( dict->next )
        dict->next = (AskForth_Dictionary*)__relocate_address( delta, (askf_addr_t)dict->next );
    if ( dict->prev )
        dict->prev = (AskForth_Dictionary*)__relocate_address( delta, (askf_addr_t)dict->prev );
    if ( dict->recent_word )
        dict->recent_word = (AskForth_Word*)__relocate_address( delta, (askf_addr_t)dict->recent_word );
    if ( dict->words_base )
        dict->words_base  = (AskForth_Word*)__relocate_address( delta, (askf_addr_t)dict->words_base );
    if ( dict->name ) 
        dict->name = (ascii*)__relocate_address( delta, (askf_addr_t)dict->name );
}

void __offset_threaded_code( askf_addr_t delta, askf_addr_t code ) {
    askf_addr_t* ip = (askf_addr_t*)code;
    AskForthDispatchOps dispatch_calls = askf_get_global_vm()->dispatch_calls;

    while ( TRUE ) {
        askf_addr_t flag = *ip++;
        askf_addr_t* op_code = ip - 1;

        if ( flag == (askf_addr_t)dispatch_calls.op_literal ) {
            *op_code = OP_LITERAL;
            ip++;
        }  
        else if ( flag == (askf_addr_t)dispatch_calls.op_literal_relocatable ) {
            *op_code = OP_LITERAL_RELOCATABLE;
            *ip = __offset_address( delta, *ip );
            ip++;
        }
        else if ( flag == (askf_addr_t)dispatch_calls.op_native ) {
            *op_code = OP_NATIVE;
            ip++;   // native handle. no need to touch it here
            *ip = __offset_address( delta, *ip );    // offset word address
            ip++;
        } 
        else if ( flag == (askf_addr_t)dispatch_calls.op_threadedword ) {
            *op_code = OP_THREADEDWORD;
            *ip = __offset_address( delta, *ip );   // offset start code
            ip++;
            *ip = __offset_address( delta, *ip );   // offset word address
            ip++;
        } 
        else if ( flag == (askf_addr_t)dispatch_calls.opt_noop ) {
            *op_code = OPT_NOOP;
        }
        else if ( flag == (askf_addr_t)dispatch_calls.op_skippable ) {
            *op_code = OP_SKIPPABLE;
            u64 bytes_toskip = *ip;
            ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
        }
        else if ( flag == (askf_addr_t)dispatch_calls.op_0branch ) {
            *op_code = OP_0BRANCH;
            ip++; // skip offset
        }
        else if ( flag == (askf_addr_t)dispatch_calls.op_branch ) {
            *op_code = OP_BRANCH;
            ip++; // skip offset
        }
        else if ( flag == (askf_addr_t)dispatch_calls.opt_type_string ) {
            *op_code = OPT_TYPE_STRING;
            *ip = __offset_address( delta, *ip );
            ip++;
            ip++;
        } 
        else if ( flag == (askf_addr_t)dispatch_calls.op_endword  ) {
            *op_code = OP_ENDWORD;
        } else if ( flag == (askf_addr_t)dispatch_calls.op_dispatch_error ) {
            *op_code = OP_DISPATCH_ERROR;
            *ip = __offset_address( delta, *ip );
            ip++;   
            ip++;
        } else if ( flag == ASKF_WORD_REDZONE ) {
            break;
        }
    }
}

void __relocate_threaded_code( AskForth_NativeWordRelocation* table, askf_addr_t delta, askf_addr_t code ) {
    askf_addr_t* ip = (askf_addr_t*)code;
    AskForthDispatchOps dispatch_calls = askf_get_global_vm()->dispatch_calls;

    while ( TRUE ) {
        askf_addr_t flag = *ip++;
        askf_addr_t* op_code = ip - 1;

        if ( flag == OP_LITERAL  ) {
            *op_code = (askf_addr_t)dispatch_calls.op_literal;
            ip++;
        }  
        else if ( flag == OP_LITERAL_RELOCATABLE ) {
            *op_code = (askf_addr_t)dispatch_calls.op_literal_relocatable;
            *ip = __relocate_address( delta, *ip );
            ip++;
        }
        else if ( flag == OP_NATIVE ) {
            *op_code = (askf_addr_t)dispatch_calls.op_native;
            askf_addr_t* native_handle = ip++;
            *ip = __relocate_address( delta, *ip );    // offset word address
            *native_handle = table[((AskForth_Word*)*ip)->source.source.wid].native_address;
            ip++;
        } 
        else if ( flag == OP_THREADEDWORD  ) {
            *op_code = (askf_addr_t)dispatch_calls.op_threadedword ;
            *ip = __relocate_address( delta, *ip );   // offset start code
            ip++;
            *ip = __relocate_address( delta, *ip );   // offset word address
            ip++;
        } 
        else if ( flag == OPT_NOOP ) {
            *op_code = (askf_addr_t)dispatch_calls.opt_noop;
        }
        else if ( flag == OP_SKIPPABLE ) {
            *op_code = (askf_addr_t)dispatch_calls.op_skippable;
            u64 bytes_toskip = *ip;
            ip = (u64*)( ( (u8*)ip ) + bytes_toskip );
        }
        else if ( flag == OP_0BRANCH  ) {
            *op_code = (askf_addr_t)dispatch_calls.op_0branch;
            ip++; // skip offset
        }
        else if ( flag == OP_BRANCH ) {
            *op_code = (askf_addr_t)dispatch_calls.op_branch;
            ip++; // skip offset
        }
        else if ( flag == OPT_TYPE_STRING  ) {
            *op_code = (askf_addr_t)dispatch_calls.opt_type_string;
            *ip = __relocate_address( delta, *ip );
            ip++;
            ip++;
        } 
        else if ( flag == OP_ENDWORD  ) {
            *op_code = (askf_addr_t)dispatch_calls.op_endword;
        } else if ( flag == OP_DISPATCH_ERROR ) {
            *op_code = (askf_addr_t)dispatch_calls.op_dispatch_error;
            *ip = __relocate_address( delta, *ip );
            ip++;   
            ip++;
        } else if ( flag == ASKF_WORD_REDZONE ) {
            break;
        }
    }
}

void __offset_word( askf_addr_t delta, AskForth_Word* word ) {
    switch ( word->source.type ) {
        case ASKF_WORD_NATIVE:
            word->source.source.wid = askf_get_word_id( word->name, word->name_len );
            break;
        case ASKF_WORD_THREADED:
            __offset_threaded_code( delta, word->source.source.threaded_code_start_addr );
            word->source.source.wid = 
                __offset_address( delta, (askf_addr_t)word->source.source.threaded_code_start_addr);
            if ( word->name ) 
                word->name = (ascii*)__offset_address( delta, (askf_addr_t)word->name );
            break;
        case ASKF_WORD_NATIVE_FOREIGN:
            if ( word->name ) 
                word->name = (ascii*)__offset_address( delta, (askf_addr_t)word->name );
            break;
    }
    if ( word->prev )
        word->prev = (AskForth_Word*)__offset_address( delta, (askf_addr_t)word->prev );
    if ( word->next )
        word->next = (AskForth_Word*)__offset_address( delta, (askf_addr_t)word->next );
    if ( word->dict_origin )
        word->dict_origin = (AskForth_Dictionary*)__offset_address( delta, (askf_addr_t)word->dict_origin );
}

void __relocate_word( AskForth_NativeWordRelocation* table, askf_addr_t delta, AskForth_Word* word ) {
    switch ( word->source.type ) {
        case ASKF_WORD_NATIVE:
            word->name = askf_get_wid_word_address(word->source.source.wid);
            word->source.source.wid = table[word->source.source.wid].native_address;
            break;
        case ASKF_WORD_THREADED:
            word->source.source.
                threaded_code_start_addr = __relocate_address( delta, word->source.source.threaded_code_start_addr );
            __relocate_threaded_code( table, delta, word->source.source.threaded_code_start_addr );
            if ( word->name ) 
                word->name = (ascii*)__relocate_address( delta, (askf_addr_t)word->name );
            break;
        case ASKF_WORD_NATIVE_FOREIGN:
            if ( word->name ) 
                word->name = (ascii*)__relocate_address( delta, (askf_addr_t)word->name );
            break;
    }
    if ( word->prev )
        word->prev = (AskForth_Word*)__relocate_address( delta, (askf_addr_t)word->prev );
    if ( word->next )
        word->next = (AskForth_Word*)__relocate_address( delta, (askf_addr_t)word->next );
    if ( word->dict_origin )
        word->dict_origin = (AskForth_Dictionary*)__relocate_address( delta, (askf_addr_t)word->dict_origin );
}

void ram_manager_ram_to_image_offsets( AskForthVm* vm ) {
    AskForth_Library*            lib = (AskForth_Library*)vm->lib;
    AskForth_ShadowLibrary*     slib = (AskForth_ShadowLibrary*)vm->shadow_lib;
    AskForthForeignManager* fmanager = vm->foreign_manager;

    askf_addr_t delta = (askf_addr_t)vm->ram->start_ptr;

    // offset all possible addresses 
    // strings: 
    //    native word's names are stored in a string table, use WID on boot to get the new address
    //    , threaded and foreign words' structs are stored in HERE, apply offset
    // native word procedures: 
    //         on boot a realocation table with the new addresses can be used to realocate the procedure addresses 
    //   native string table and realocation table both share the same ids for words
    // threaded code:
    //         must do a one pass on all threaded code found and offset all addresses ( including literals marked as relocatable )
    //         must also switch OP labels for OP ids
    // foreign words:
    //         nothing has to be done except the string name that is offset
    //
    // all these are also implied to shadow dictionaries and shadow words

    // main library and words
    AskForth_Dictionary* dict = lib->dictionaries_base;
    while ( dict != NULL ) {
        AskForth_Word*               word = dict->recent_word;
        AskForth_Dictionary*    next_dict = dict->next;

        while ( word != NULL ) {
            AskForth_Word* next = word->next;
            __offset_word( delta, word );
            word = next;
        }
        __offset_dictionary( delta, dict );
        dict = next_dict;
    }


    // shadow library
    dict = slib->dict_base;
    while ( dict != NULL ) {
        AskForth_Word*               word = dict->recent_word;
        AskForth_Dictionary*    next_dict = dict->next;

        while ( word != NULL ) {
            AskForth_Word* next = word->next;
            __offset_word( delta, word );
            word = next;
        }
        __offset_dictionary( delta, dict );
        dict = next_dict;
    }

    // // shadow words
    AskForth_Word* sword = slib->word_base;
    while ( sword != NULL ) {
        AskForth_Word* next_sword = sword->next;
        __offset_word( delta, sword );
        sword = next_sword;
    }


    // foreign linking
    AskForthForeignObject* fobj = fmanager->objects;
    while( fobj ) {
        AskForthForeignObject* next_obj = fobj->next;
        fobj->name = (ascii*)__offset_address( delta, (askf_addr_t)fobj->name );
        if ( fobj->next )
            fobj->next = (AskForthForeignObject*)__offset_address( delta, (askf_addr_t)fobj->next );
        if ( fobj->prev )
            fobj->prev = (AskForthForeignObject*)__offset_address( delta, (askf_addr_t)fobj->prev );

        fobj = next_obj;
    }

    __offset_library( delta, lib );
    __offset_shadow_library( delta, slib );
    __offset_foreign_manager( delta, fmanager );
}

void ram_manager_relocate_ram( AskForthVm* vm ) {
    AskForth_Library*            lib = (AskForth_Library*)vm->lib;
    AskForth_ShadowLibrary*     slib = (AskForth_ShadowLibrary*)vm->shadow_lib;
    AskForthForeignManager* fmanager = vm->foreign_manager;

    askf_addr_t delta = (askf_addr_t)vm->ram->start_ptr;

    __relocate_library( delta, lib );
    __relocate_shadow_library( delta, slib );
    __relocate_foreign_manager( delta, fmanager );

    printf("Relocated main library, shadow library, foreign manager structs \n");

    // offset all possible addresses 
    // strings: 
    //    native word's names are stored in a string table, use WID on boot to get the new address
    //    , threaded and foreign words' structs are stored in HERE, apply offset
    // native word procedures: 
    //         on boot a realocation table with the new addresses can be used to realocate the procedure addresses 
    //   native string table and realocation table both share the same ids for words
    // threaded code:
    //         must do a one pass on all threaded code found and offset all addresses ( including literals marked as relocatable )
    //         must also switch OP labels for OP ids
    // foreign words:
    //         relocate string offsets and relink handle
    //
    // all these are also implied to shadow dictionaries and shadow words
    AskForth_NativeWordRelocation* table = askf_get_relocation_array();

    // main library and words
    AskForth_Dictionary* dict = lib->dictionaries_base;

    while ( dict != NULL ) {
        __relocate_dictionary( delta, dict );
        AskForth_Word*               word = dict->recent_word;
        AskForth_Dictionary*    next_dict = dict->next;

        while ( word != NULL ) {
            __relocate_word( table, delta, word );
            AskForth_Word* next = word->next;
            word = next;
        }
        dict = next_dict;
    }

    printf("Relocated main library \n");

    // shadow library
    dict = slib->dict_base;
    while ( dict != NULL ) {
        __relocate_dictionary( delta, dict );
        AskForth_Word*               word = dict->recent_word;
        AskForth_Dictionary*    next_dict = dict->next;

        while ( word != NULL ) {
            __relocate_word( table, delta, word );
            AskForth_Word* next = word->next;
            word = next;
        }
        dict = next_dict;
    }
    printf("Relocated shadow library \n");

    // shadow words
    AskForth_Word* sword = slib->word_base;
    while ( sword != NULL ) {
        __relocate_word( table, delta, sword );
        AskForth_Word* next_sword = sword->next;
        sword = next_sword;
    }
    printf("Relocated shadow words   \n");

    // foreign linking
    AskForthForeignObject* fobj = fmanager->objects;
    while( fobj ) {
        fobj->name = (ascii*)__relocate_address( delta, (askf_addr_t)fobj->name );
        if ( fobj->next )
            fobj->next = (AskForthForeignObject*)__relocate_address( delta, (askf_addr_t)fobj->next );
        if ( fobj->prev )
            fobj->prev = (AskForthForeignObject*)__relocate_address( delta, (askf_addr_t)fobj->prev );

        AskForthForeignObject* next_obj = fobj->next;
        fobj = next_obj;
    }
    printf("Relocated foreign manager \n");

    askf_free_relocation_array( table );
}

void ram_manager_new_image_header( AskForthVm* vm, askf_addr_t delta, AskForthImageHeader* header) {
    header->magic = ASKF_MAGIC_NUM;
    header->foreign_manager_offset = __offset_address( delta, (askf_addr_t)vm->foreign_manager);
    header->library_offset         = __offset_address( delta, (askf_addr_t)vm->lib);
    header->shadow_library_offset  = __offset_address( delta, (askf_addr_t)vm->shadow_lib);
    header->header_size            = (askf_addr_t)sizeof( AskForthImageHeader );
}

// if return value is TRUE image_size will say the image size
boolean ram_manager_write_image( 
        AskForthImageHeader* header, AskForthVm* vm, askf_addr_t* image_size, askf_addr_t ram_recoverable_offset, FILE* executable ) {

    askf_addr_t recovery_point  = (askf_addr_t)( ( u8* )vm->ram->start_ptr + ram_recoverable_offset );
    askf_addr_t end_of_ram      = (askf_addr_t)( ( u8* )vm->ram->start_ptr + vm->ram->byte_index    );
    askf_addr_t ram_bytes = end_of_ram - recovery_point;

    // create image header 
    fwrite( header, sizeof(AskForthImageHeader), 1, executable );

    // write RAM image
    fwrite( (const void *)recovery_point, sizeof(u8), ram_bytes, executable );

    *image_size = header->header_size + ram_bytes;
    return TRUE;
}


