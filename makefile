CFLAGS = -Wall -s -Os -std=c99 -MMD -MP
CFLAGS_LIN = $(CFLAGS) `pkg-config --cflags glfw3 freetype2`
LDFLAGS = $(pkg-config --static --libs glfw3)

#`pkg-config --cflags glfw3 freetype2`
src = $(wildcard src/*.c)
src_min = src/main.c src/app.c src/gb.c
srcGL = $(wildcard src/api/glfw/*.c)
srcTIGR = $(wildcard src/api/tigr/*.c)

CC = gcc
#"i686-w64-mingw32-gcc"

lib = $(csrc:.c=.a)
obj = $(csrc:.c=.o)

# Output
target = bin/gb-emu
all: glfw

.PHONY: clean

LDFLAGS = `pkg-config --libs glfw3 freetype2`

# main build	
glfw: $(obj)
	$(CC) $(CFLAGS) -DUSE_GLFW $(src_min) $(srcGL) -o $(target) -I../_include -lm -lglfw3

glfw-l: $(obj)
	$(CC) $(CFLAGS_LIN) -DUSE_GLFW $(src_min) $(srcGL) -o $(target) -lm -ldl -lpthread $(LDFLAGS)

# no GLFW build
tigr: $(obj)
	$(CC) $(CFLAGS) -DUSE_TIGR $(src_min) $(srcTIGR) -o $(target) -I../_include -lm -lopengl32 -lgdi32

min: $(obj)
	$(CC) $(CFLAGS) $(src_min) -o $(target) -I../_include -lm -lgdi32

min-l: $(obj)
	$(CC) $(CFLAGS) $(src_min) -o $(target) -lm

clean:
	rm -f $(obj) $(target)