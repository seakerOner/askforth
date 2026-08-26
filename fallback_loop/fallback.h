#ifndef ASKF_FALLBACKLOOP_H
#define ASKF_FALLBACKLOOP_H

#include "../vm/forth_vm.h"

typedef struct {
    ascii* base;
    u64  length; 
} AskForthTokenCmd;

typedef struct {
    AskForthTokenCmd cmd_tkn;
    AskForthTokenCmd description;
    void(*fn)(AskForthVm*);
} AskForthFallBackCmd;


typedef enum {
    FALLBACK_CMD_HELP = 0,  // show commands
    FALLBACK_CMD_STATUS,    // show VM states
    FALLBACK_CMD_TRACE,     // show trace errors
    FALLBACK_CMD_STACK,     // show stack
    FALLBACK_CMD_RSTACK,    // show return stack
    FALLBACK_CMD_CFSTACK,   // show control flow stack
    FALLBACK_CMD_INPUT,     // show input states, where code came from 
    FALLBACK_CMD_CONTINUE,  // continue execution
    FALLBACK_CMD_QUIT,      // quit VM
    FALLBACK_CMD_ABORT,     // abort current operation & go back to interpreter
    FALLBACK_CMD_RESET,     // reset VM 
} AskForthFallbackCommands;

#define ASKF_FALLBACK_NUM_CMDS 11

void askforth_fallbackloop_run( AskForthVm* vm );

AskForthFallBackCmd* __get_fallback_cmds(void);

#endif
