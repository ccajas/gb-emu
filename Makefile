CFLAGS = -Wall -s -Os -std=gnu89 -MMD -MP
CFLAGS_LIN = $(CFLAGS) `pkg-config --cflags glfw3`
LDFLAGS = $(pkg-config --static --libs glfw3)

#`pkg-config --cflags glfw3 freetype2`
GLdir = src/api/glfw/
src = $(wildcard src/*.c)
src_min = src/main.c src/app.c src/gb.c src/cart.c
srcGL = $(GLdir)graphics.c $(GLdir)glad.c $(GLdir)shader.c
srcTIGR = $(wildcard src/api/tigr/*.c)

CC = i686-w64-mingw32-gcc
#"i686-w64-mingw32-gcc"

lib = $(csrc:.c=.a)
obj = $(csrc:.c=.o)

# Output
target = bin/win32/gb-emu
target_linux = bin/linux/gb-emu
all: glfw
includes = -I../_include

.PHONY: clean

LDFLAGS = `pkg-config --static --libs glfw3`

# main builds
glfw: $(obj)
	$(CC) $(CFLAGS) -DUSE_GLFW $(src_min) $(srcGL) -o $(target) $(includes) -lm -lglfw3 -lgdi32

glfw-l: $(obj)
	gcc $(CFLAGS_LIN) -DUSE_GLFW $(src_min) $(srcGL) -o $(target_linux) -lm -ldl $(LDFLAGS)

# no GLFW build
tigr: $(obj)
	$(CC) $(CFLAGS) -DUSE_TIGR $(src_min) $(srcTIGR) -o $(target) $(includes) -lm -lopengl32 -lgdi32

minifb: $(obj)
	$(CC) $(CFLAGS) src/main.c src/app.c src/gb.c src/cart.c -o $(target) -lgdi32

core: $(obj)
	$(CC) $(CFLAGS) $(src_min) -o $(target) -I../_include -lm -lgdi32

core-l: $(obj)
	gcc $(CFLAGS) $(src_min) -o $(target_linux) -lm

clean:
	rm -f $(obj) $(target)