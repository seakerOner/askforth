#include "library.h"

AskForth_Library* askf_create_library( AskForthVm* vm ) {
    AskForth_Library* lib = NULL;

    lib = askf_blob_alloc( vm->ram, sizeof( AskForth_Library ) );

    if ( lib == NULL )
        return NULL;

    // TODO: linked list style

    return lib;
}

AskForth_Dictionary* askf_create_dic( AskForthVm* vm, ascii name[ASKF_MAX_NAME_LEN] ) {
    AskForth_Library* lib    = ( AskForth_Library* )vm->lib;
    AskForth_Dictionary* dic = NULL;

    // TODO: linked list style

    COPY(name, dic->name, ASKF_MAX_NAME_LEN );

    return dic;
}
