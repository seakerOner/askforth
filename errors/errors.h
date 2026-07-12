#ifndef ASKF_ERRORS_H
#define ASKF_ERRORS_H

typedef enum {
    ASKF_ERROR_FAILED_LIB_ALLOC         = 0x00,
    ASKF_ERROR_FAILED_CORE_DIC_ALLOC    = 0x01,
    ASKF_ERROR_FAILED_DIC_ALLOC         = 0x02,

} AskForthErrorType;

typedef enum {
    ASKF_ERROR_ZONE_OUTER,
    ASKF_ERROR_ZONE_INNER
} AskForthErrorZone;

typedef struct {
    AskForthErrorType error;
    AskForthErrorZone zone;
} AskForthError;

#endif
