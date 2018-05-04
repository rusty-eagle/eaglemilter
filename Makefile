# the compiler: gcc for C program, define as g++ for C++
CC = clang

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall

# the build target executable:
BIN_NAME = EagleMilter
TARGET = milter

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o dist/$(BIN_NAME) $(TARGET).c -Ijson-parse json-parser/json.c -lmilter -lpthread -lm

clean:
	$(RM) dist/$(TARGET)
