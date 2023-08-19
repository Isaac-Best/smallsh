# Compiler to use
CC = gcc

# Compiler flags
CFLAGS = -std=c99 -Wall

# Object files
OBJS = command_handling.o flags.o signal_handling.o string_processing.o smallsh.o

# Executable name
EXEC = smallsh

# Default target
all: $(EXEC)

# Linking the final executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

# Compiling the object files
command_handling.o: command_handling.c
	$(CC) $(CFLAGS) -c command_handling.c -o command_handling.o

flags.o: flags.c
	$(CC) $(CFLAGS) -c flags.c -o flags.o

signal_handling.o: signal_handling.c
	$(CC) $(CFLAGS) -c signal_handling.c -o signal_handling.o

string_processing.o: string_processing.c
	$(CC) $(CFLAGS) -c string_processing.c -o string_processing.o

smallsh.o: smallsh.c
	$(CC) $(CFLAGS) -c smallsh.c -o smallsh.o

# Clean target
clean:
	rm -f $(OBJS) $(EXEC)

# Phony targets
.PHONY: all clean
