#ifndef ASKFORTH_WORDS_H
#define ASKFORTH_WORDS_H

#include "../inttype.h"

typedef struct {
    askf_addr_t native_address;
} AskForth_NativeWordRelocation;

void askf_initialize_core( void );

// native functions only
// returns NULL on failure
ascii* askf_get_wid_word_address( askf_addr_t wid );
// returns -1 on fail
askf_addr_t askf_get_word_id( ascii* name, askf_addr_t len );

// used for AskForth images
AskForth_NativeWordRelocation* askf_get_relocation_array( void );
void askf_free_relocation_array( AskForth_NativeWordRelocation* table );

void askf_add_core_words( void );

void askf_word_align( void );
void askf_word_refill( void );

#endif
