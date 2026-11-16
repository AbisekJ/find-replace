CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = find_replace

SRC = main.c find_replace.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c find_replace.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
