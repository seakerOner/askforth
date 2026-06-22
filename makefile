CC 	  = gcc
BUILD = ./build/

TARGET = -DTARGET_LINUX

EXECUTABLE_NAME = askforth

FLAGS = -std=c11 -Wall -Wextra 

FLAGS += $(TARGET)

askforth: main.o stack.o mem_backend_blob.o vm.o
	$(CC) $(BUILD)/*.o -o $(BUILD)/$(EXECUTABLE_NAME)

main.o: ./main.c 
	$(CC) $(FLAGS)	-c ./main.c -o $(BUILD)/main.o

stack.o: ./stack/stack.c
	$(CC) $(FLAGS)	-c ./stack/stack.c -o $(BUILD)/stack.o

mem_backend_blob.o: ./memory/backend_blob.c
	$(CC) $(FLAGS)	-c ./memory/backend_blob.c -o $(BUILD)/mem_backend_blob.o

vm.o: ./vm/forth_vm.c 
	$(CC) $(FLAGS)	-c ./vm/forth_vm.c -o $(BUILD)/vm.o

clean:
	rm -f $(BUILD)/*
	@echo "Cleaned Build Files..."

run:
	make
	@echo " "
	$(BUILD)/$(EXECUTABLE_NAME)
