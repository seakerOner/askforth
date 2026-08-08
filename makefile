# For windows executable creation MSYS2 is needed with required toolchain
CC 	  = gcc
BUILD = ./build

EXECUTABLE_NAME = askforth

TARGET ?= -DTARGET_LINUX

FLAGS = -Wall -Wextra -x c
FLAGS += $(TARGET)

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

$(BUILD):
	mkdir -p $(BUILD)

askforth: $(BUILD) $(OBJECTS)
	$(CC) $(addprefix $(BUILD)/,$(OBJECTS)) -o $(BUILD)/$(EXECUTABLE_NAME)

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

clean:
	rm -f $(BUILD)/*.o
	rm -f $(BUILD)/$(EXECUTABLE_NAME)
	rm -f $(BUILD)/$(EXECUTABLE_NAME).exe
	@echo "Cleaned Build Files..."

linux: 
	$(MAKE) TARGET=-DTARGET_LINUX askforth

windows:
	$(MAKE) TARGET=-DTARGET_WINDOWS askforth


run: askforth
	@echo " "
	ifeq ( $(TARGET), -DTARGET_WINDOWS )
		$(BUILD)/$(EXECUTABLE_NAME).exe
	else
		$(BUILD)/$(EXECUTABLE_NAME)
	endif
