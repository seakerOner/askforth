
#include "../vm/forth_vm.h"
#include "../memory/backend_blob.h"

#ifdef TARGET_LINUX
    #define ASKF_MAX_DICS     32
    #define ASKF_MAX_NAME_LEN 64
#else
    #define ASKF_MAX_DICS     10 
    #define ASKF_MAX_NAME_LEN 28
#endif

typedef struct AskForth_Word_t AskForth_Word;

typedef enum {
    ASKF_WORD_NATIVE,
    ASKF_WORD_THREADED
} AskForth_WordType;

typedef struct {
    AskForth_WordType type;
    union {
        void (*native_code)(void);
        u64  threaded_code_start_addr;
    } source;
} AskForth_WordSource;

typedef struct AskForth_Word_t {
    ascii name[ASKF_MAX_NAME_LEN];
    u64   name_len;

    AskForth_WordSource source;
    boolean is_immediate;
    AskForth_Word* prev;
    AskForth_Word* next;
} AskForth_Word;

typedef struct AskForth_Dictionary_t AskForth_Dictionary;

typedef struct AskForth_Dictionary_t {
    AskForth_Word*          words_base;
    AskForth_Word*          recent_word;
    AskForth_Dictionary*    next;
    ascii                   name[ASKF_MAX_NAME_LEN];
    u64                     name_len;
} AskForth_Dictionary;

typedef struct {
    AskForth_Word*          word;
    AskForth_Dictionary*     dic;
    u64*                    here;
} AskForth_WordCompiling;

typedef struct {
    AskForth_Dictionary*    dictionaries_base;
    AskForth_Dictionary*    recent_dic;
    AskForth_WordCompiling  curr_compiling;
} AskForth_Library;


AskForth_Library*       askf_create_library( AskForthVm* vm );

AskForth_Dictionary*    askf_create_dic( AskForthVm* vm, ascii name[ASKF_MAX_NAME_LEN], u64 name_len );

AskForth_Dictionary* askf_library_find_dic( AskForthVm* vm, AskForthToken* token );

AskForth_Word* askf_library_find_word( AskForthVm* vm, AskForthToken* token );

boolean askf_dic_add_word_native( 
        AskForthToken dic_name, 
        boolean is_immediate,
        void(*native_subroutine)(void), 
        AskForthToken word_name );

boolean askf_dic_add_word_threaded( AskForth_Dictionary* dic, AskForthToken word_name );
