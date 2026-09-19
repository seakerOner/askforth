#include "input_stack.h"

void askf_start_istack( AskForth_InputStack* Istack, ascii* base, u64 byte_cap ) {
    Istack->index = 0;
    Istack->capacity = ASKF_INPUT_STACK_MAX_DEPTH;

    Istack->sources[Istack->index].base         = base;
    Istack->sources[Istack->index].in           = 0;    
    Istack->sources[Istack->index].in_max       = 0;    
    Istack->sources[Istack->index].blk          = 0;    // not block
    Istack->sources[Istack->index].source_id    = 0;    // user input
    Istack->sources[Istack->index++].byte_cap   = byte_cap;
}

boolean askf_istack_push( AskForth_InputStack* Istack, ascii* base, u64 byte_cap , i64 source_id, u64 blk ) {
    if ( Istack->index >= Istack->capacity ) 
        return FALSE;

    Istack->sources[Istack->index].base         = base;
    Istack->sources[Istack->index].in           = 0;    
    Istack->sources[Istack->index].in_max       = byte_cap;    
    Istack->sources[Istack->index].source_id    = source_id;
    Istack->sources[Istack->index].blk          = blk;
    Istack->sources[Istack->index++].byte_cap   = byte_cap;
    return TRUE;
};

AskForth_InputSource* askf_istack_peek( AskForth_InputStack* Istack ) {
    return &Istack->sources[Istack->index-1];
}

boolean askf_istack_pop( AskForth_InputStack* Istack ) {
    // cannot remove the main inputbuffer source but we reset it
    if ( Istack->index <= 1 )  {
        return FALSE;
    }

    Istack->index--;
    return TRUE;
}

AskForthToken askf_next_token( AskForth_InputSource* source ) {
    ascii*  base  = source->base;
    AskForthToken new_token = {0};
    u64 length = 0;
    
    while ( source->in < source->in_max ) {
        if ( base[source->in] == '\r' )
            base[source->in] = ' ';

        if ( base[source->in] == ' ' || base[source->in] == '\n' || base[source->in] == '\0' ) {
            if ( length > 0 ) {
                new_token.base          = &base[source->in - length];
                new_token.length        = length;

                if ( base[source->in] == '\n' || base[source->in] == '\0' )
                    new_token.line_end      = TRUE;

                source->in++;
                return new_token;
            }  else {
                source->in++;
            }

         } else  {
             length++;
             source->in++;
         }
    }

    new_token.base          = NULL;
    new_token.length        = 0;
    return new_token;
}
