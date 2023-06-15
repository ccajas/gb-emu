CFLAGS = -Wall -s -Os -std=gnu90 -MMD -MP
CFLAGS_LIN = $(CFLAGS) `pkg-config --cflags glfw3`
LDFLAGS = $(pkg-config --static --libs glfw3)

#`pkg-config --cflags glfw3 freetype2`
src = $(wildcard src/*.c)
src_min = src/main.c src/app.c src/gb.c src/cart.c
srcGL = $(wildcard src/api/glfw/*.c)
srcTIGR = $(wildcard src/api/tigr/*.c)

CC = i686-w64-mingw32-gcc
#"i686-w64-mingw32-gcc"

lib = $(csrc:.c=.a)
obj = $(csrc:.c=.o)

# Output
target = bin/gb-emu
target_linux = bin/linux/gb-emu
all: glfw

.PHONY: clean

LDFLAGS = `pkg-config --libs glfw3 freetype2`

# main build	
glfw: $(obj)
	$(CC) $(CFLAGS) -DUSE_GLFW $(src_min) $(srcGL) -o $(target) -I../_include -lm -lglfw3 -lgdi32

glfw-l: $(obj)
	gcc $(CFLAGS_LIN) -DUSE_GLFW $(src_min) $(srcGL) -o $(target_linux) -lm -ldl -lpthread $(LDFLAGS)

# no GLFW build
tigr: $(obj)
	$(CC) $(CFLAGS) -DUSE_TIGR $(src_min) $(srcTIGR) -o $(target) -I../_include -lm -lopengl32 -lgdi32

core: $(obj)
	$(CC) $(CFLAGS) $(src_min) -o $(target) -I../_include -lm -lgdi32

core-l: $(obj)
	gcc $(CFLAGS) $(src_min) -o $(target) -lm

clean:
	rm -f $(obj) $(target)