#ifndef ASKF_TOKENIZER_H
#define ASKF_TOKENIZER_H

#include "../inttype.h"
#include "../stack/stack.h"

typedef struct {
    ascii*  base;
    u64     length;
    boolean line_end;
} AskForthToken;

typedef struct {
    u64 idx;
    AskForthToken* token;
} AskForthTokenizerCtx;

typedef enum {
    ASKF_COMMENT_STATE_NONE,
    ASKF_COMMENT_STATE_PAREN,
    ASKF_COMMENT_STATE_SLASH,
} AskForthCommentState;

typedef struct {
    AskForthToken*  tokens;
    u64             index;
    u64             capacity;

    AskForthTokenizerCtx ctx;
    AskForthCommentState comment_state;
} AskForthTokenizer;


void askf_tokenizer_new( AskForthTokenizer* tokenizer, u64 max_tokens );

void askf_tokenizer_reset( AskForthTokenizer* tokenizer );

void askf_tokenizer_add( AskForthTokenizer* tokenizer, AskForthToken new_token );

boolean askf_parse_token_to_num( AskForthToken* token, AskForth_Cell* out_cell );

#endif
