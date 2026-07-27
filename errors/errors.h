#ifndef ASKF_ERRORS_H
#define ASKF_ERRORS_H

#include "../inttype.h"

typedef enum {
    ASKF_ERROR_FAILED_LIB_ALLOC         = 0,
    ASKF_ERROR_FAILED_CORE_DIC_ALLOC    = 1,
    ASKF_ERROR_FAILED_DIC_ALLOC         = 2,

    ASKF_ERROR_UNKNOWN_WORD             = 3,

    ASKF_ERROR_WORD_NAME_OVERFLOW       = 4,
    ASKF_ERROR_UNKNOWN_DIC              = 5
} AskForthErrorType;

typedef struct {
    ascii* message;
    u64    length;
} AskForthErrorMessage;

typedef enum {
    ASKF_ERROR_ZONE_OUTER,
    ASKF_ERROR_ZONE_INNER
} AskForthErrorZone;

typedef struct {
    AskForthErrorType error;
    AskForthErrorZone zone;
    AskForthErrorMessage* opt_message;
} AskForthError;

#endif
