CFLAGS = -Wall -Wextra -s -O1 -std=c99 -MMD -MP 
#`pkg-config --cflags glfw3 freetype2`
src = $(wildcard src/*.c) 
CC = "i686-w64-mingw32-gcc"

lib = $(csrc:.c=.a)
obj = $(csrc:.c=.o)

# Output
target = bin/gb-emu
#all: glfw

.PHONY: clean

# main build	
glfw: $(obj)
	$(CC) $(CFLAGS) $(src) -o $(target) -lm $(LDFLAGS)