
#include "../vm/forth_vm.h"
#include "../memory/backend_blob.h"

#ifdef TARGET_LINUX
    #define ASKF_MAX_DICS     32
    #define ASKF_MAX_NAME_LEN 64
#else
    #define ASKF_MAX_DICS     10 
    #define ASKF_MAX_NAME_LEN 28
#endif

typedef struct AskForth_Word_t  AskForth_Word;

typedef struct AskForth_Word_t {
    ascii name[ASKF_MAX_NAME_LEN];
    void* next;
} AskForth_Word;

typedef struct AskForth_Dictionary_t {
    ascii                   name[ASKF_MAX_NAME_LEN];
    AskForth_Word*          words_base;
    AskForth_Word*          recent_word;
    void* next;
} AskForth_Dictionary;

typedef struct {
    AskForth_Dictionary*    dictionaries_base;
    AskForth_Dictionary*    recent_dic;
} AskForth_Library;

AskForth_Library* askf_create_library( AskForthVm* vm );

AskForth_Dictionary* askf_create_dic( AskForthVm* vm, ascii name[ASKF_MAX_NAME_LEN] );
