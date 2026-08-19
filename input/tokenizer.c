#include "tokenizer.h"

#include "../memory/backend_blob.h"
#include "../vm/forth_vm.h"

void askf_tokenizer_new( AskForthTokenizer* tokenizer, u64 max_tokens ) {
    if ( tokenizer == NULL ) {
        // TODO: throw error
        return;
    }

    AskForthVm* global_vm               = askf_get_global_vm();

    tokenizer->capacity = max_tokens;
    tokenizer->index    = 0;
    tokenizer->tokens   = ( AskForthToken* )askf_blob_alloc
        ( global_vm->ram, ( sizeof( AskForthToken ) * max_tokens ) );

    tokenizer->ctx.token        = NULL;
    tokenizer->ctx.idx          = 0;
    tokenizer->comment_state    = ASKF_COMMENT_STATE_NONE;

    if ( tokenizer->tokens == NULL ) {
        // TODO: throw error
    }

}

void askf_tokenizer_reset( AskForthTokenizer* tokenizer ) {
    if ( tokenizer == NULL ) {
        // TODO: throw error
    }
    tokenizer->index            = 0;
    tokenizer->ctx.idx          = 0;
    tokenizer->ctx.token        = NULL;
}

void askf_tokenizer_add( AskForthTokenizer* tokenizer, AskForthToken new_token ) {
    if ( tokenizer->index >= tokenizer->capacity )
        return;

    COPY( &new_token, &tokenizer->tokens[tokenizer->index], sizeof( AskForthToken ) );
    tokenizer->index++;
}

// TODO: add number transformation by number base ( binary, decimal, hexadecimal )
boolean askf_parse_token_to_num( AskForthToken* token, AskForth_Cell* out_cell ) {
    u64 result              = 0;
    boolean make_negative   = FALSE;

    if ( token->base[0] == '-' ) {
        make_negative = TRUE;
    }
    if ( make_negative && token->length == 1 ) {
        // TODO: throw error
        return FALSE;
    }

    for ( u64 x = make_negative; x < token->length; x++ ) {
        ascii character = token->base[x];

        if ( character < '0' || character > '9' ) {
            // TODO: throw error
            return FALSE;
        }

        result = result * 10 + ( character - '0' );
    }

    if ( make_negative )
        result = (u64)(-(i64)result);

    out_cell->val._64u = result;

    return TRUE;
}
