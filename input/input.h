#include "../vm/forth_vm.h"

void askf_read_input_blocking( AskForthVm* vm  );

void askf_reset_input_buffer( AskForthVm* vm );

void askf_print( ascii* buff, u32 len );
