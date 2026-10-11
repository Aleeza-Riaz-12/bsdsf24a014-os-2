CC := gcc
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Wpedantic
SRC := src/ls-v1.0.0.c
OBJ := obj/ls.o
TARGET := bin/ls
.PHONY: all clean test
all: $(TARGET)
$(TARGET): $(OBJ) | bin
	$(CC) $(CFLAGS) $< -o $@
$(OBJ): $(SRC) | obj
	$(CC) $(CFLAGS) -c $< -o $@
obj bin:
	mkdir -p $@
test: all
	python3 tests/test_ls.py
clean:
	rm -f $(OBJ) $(TARGET)
