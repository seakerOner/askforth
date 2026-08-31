# For windows executable creation MSYS2 is needed with required toolchain
CC 	  		= gcc

BUILD 		= ./build

# native C compilation arquitecture 
# this controls GCC's -march and is independent from the Forth 
ARQ   		?= x86-64

# AskForth data-stack cell-width
# must match the arquitecture bits
CELL-BITS 	?= 64

EXECUTABLE_NAME = askforth
EXECUTABLE_EXT  = 

TARGET ?= -DTARGET_LINUX

FLAGS = -Wall -Wextra -x c -O2
FLAGS += -march=$(ARQ)
FLAGS += $(TARGET)
FLAGS += -DARQBITS$(CELL-BITS)

OBJECTS = 				\
	main.o 				\
	stack.o 			\
	mem_backend_blob.o 	\
	vm.o input.o 		\
	library.o 			\
	errors.o 			\
	tokenizer.o 		\
	core_words.o 		\
	blocks.o 			\
	fallback_loop.o		\

$(BUILD):
	mkdir -p $(BUILD)

askforth: $(BUILD) $(OBJECTS)
	$(CC) $(addprefix $(BUILD)/,$(OBJECTS)) -o $(BUILD)/$(EXECUTABLE_NAME)$(EXECUTABLE_EXT)	

main.o: ./main.c 
	$(CC) $(FLAGS)	-c ./main.c -o $(BUILD)/main.o

stack.o: ./stack/stack.c
	$(CC) $(FLAGS)	-c ./stack/stack.c -o $(BUILD)/stack.o

mem_backend_blob.o: ./memory/backend_blob.c
	$(CC) $(FLAGS)	-c ./memory/backend_blob.c -o $(BUILD)/mem_backend_blob.o

vm.o: ./vm/forth_vm.c 
	$(CC) $(FLAGS)	-c ./vm/forth_vm.c -o $(BUILD)/vm.o

input.o: ./input/input.c
	$(CC) $(FLAGS)	-c ./input/input.c -o $(BUILD)/input.o

tokenizer.o: ./input/tokenizer.c
	$(CC) $(FLAGS)	-c ./input/tokenizer.c -o $(BUILD)/tokenizer.o

library.o: ./library/library.c
	$(CC) $(FLAGS)	-c ./library/library.c -o $(BUILD)/library.o

core_words.o: ./words/askforth_words.c
	$(CC) $(FLAGS)	-c ./words/askforth_words.c -o $(BUILD)/core_words.o

errors.o: ./errors/error_thrower.c
	$(CC) $(FLAGS)	-c ./errors/error_thrower.c -o $(BUILD)/errors.o

blocks.o: ./memory/blocks.c
	$(CC) $(FLAGS)	-c ./memory/blocks.c -o $(BUILD)/blocks.o

fallback_loop.o: ./fallback_loop/fallback.h
	$(CC) $(FLAGS)	-c ./fallback_loop/fallback.c -o $(BUILD)/fallback_loop.o

clean:
	rm -f $(BUILD)/*.o
	rm -f $(BUILD)/$(EXECUTABLE_NAME)
	rm -f $(BUILD)/$(EXECUTABLE_NAME).exe
	@echo "Cleaned Build Files..."

x86-64-linux:
	$(MAKE) ARQ=x86-64 BUILD=./build/x86-64-linux CELL-BITS=64 TARGET=-DTARGET_LINUX askforth 

x86-64-windows:
	$(MAKE) ARQ=x86-64 BUILD=./build/x86-64-windows CELL-BITS=64 TARGET=-DTARGET_WINDOWS EXECUTABLE_EXT=.exe askforth 
