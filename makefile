CFLAGS = -Wall -s -O1 -std=c99 -MMD -MP
LDFLAGS = $(pkg-config --static --libs glfw3)

#`pkg-config --cflags glfw3 freetype2`
src = $(wildcard src/*.c) 
srcGL = $(wildcard src/api/glfw/*.c)

CC = "i686-w64-mingw32-gcc"

lib = $(csrc:.c=.a)
obj = $(csrc:.c=.o)

# Output
target = bin/gb-emu
#all: glfw

.PHONY: clean

# main build	
glfw: $(obj)
	$(CC) $(CFLAGS) src/main.c src/app.c src/gb.c -o $(target) -I../_include -lm -lglfw3 -lgdi32

clean:
	rm -f $(obj) $(target)