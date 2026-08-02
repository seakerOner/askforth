#ifndef ASKF_INPUT_H
#define ASKF_INPUT_H
#include "../vm/forth_vm.h"
//#include "../stack/stack.h"

void askf_read_input_blocking( AskForthVm* vm  );

void askf_reset_input_buffer( AskForthVm* vm );

void askf_print( ascii* buff, u32 len );
void askf_print_char( ascii _char );

void askf_print_cell( AskForth_Cell* cell );
#endif
