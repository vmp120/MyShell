#compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -g

TARGET = mysh

#source files
SRC = mysh.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

#run the shell in interactive mode
run: $(TARGET)
	./$(TARGET)

#run in batch mode
runbatch: $(TARGET)
	./$(TARGET) $(FILE)

.PHONY: all clean run runbatch