CC 	  = gcc
BUILD = ./build/

TARGET = -DTARGET_LINUX

EXECUTABLE_NAME = askforth

FLAGS = -std=c11 -Wall -Wextra 

FLAGS += TARGET

askforth: main.o stack.o
	$(CC) $(BUILD)/*.o -o $(BUILD)/$(EXECUTABLE_NAME)

main.o: ./main.c 
	$(CC) $(FLAGS)	-c ./main.c -o $(BUILD)/main.o

stack.o: ./stack/stack.c
	$(CC) $(FLAGS)	-c ./stack/stack.c -o $(BUILD)/stack.o

clean:
	rm -f $(BUILD)/*
	@echo "Cleaned Build Files..."

run:
	make
	@echo " "
	$(BUILD)/$(EXECUTABLE_NAME)
