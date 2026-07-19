#include "tokenizer.h"

#include "../memory/backend_blob.h"
#include "../vm/forth_vm.h"

void askf_tokenizer_new( AskForthTokenizer* tokenizer, u64 max_tokens ) {
    if ( tokenizer == NULL ) {
        // TODO: throw error
    }

    AskForthVm* global_vm               = askf_get_global_vm();

    tokenizer->capacity = max_tokens;
    tokenizer->index    = 0;
    tokenizer->tokens = ( AskForthToken* )askf_blob_alloc( global_vm->ram, ( sizeof( AskForthToken ) * max_tokens ) );

    if ( tokenizer->tokens == NULL ) {
        // TODO: throw error
    }

}

void askf_tokenizer_reset( AskForthTokenizer* tokenizer ) {
    if ( tokenizer == NULL ) {
        // TODO: throw error
    }
    tokenizer->index = 0;
}

void askf_tokenizer_add( AskForthTokenizer* tokenizer, AskForthToken new_token ) {
    if ( tokenizer->index >= tokenizer->capacity )
        return;

    COPY( &new_token, &tokenizer->tokens[tokenizer->index], sizeof( AskForthToken ) );
    tokenizer->index++;
}

