CFLAGS = -Wall -s -O2 -std=gnu89 -MMD -MP
CFLAGS_WIN = $(CFLAGS) -Wl,-subsystem,windows
GLFW_PKG = `pkg-config --static --libs glfw3`

#`pkg-config --cflags glfw3 freetype2`
GLdir = src/api/gl/
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

.PHONY: clean

# main builds
glfw: $(obj)
	$(CC) -I../_include  -L../_lib -DGBE_DEBUG -DUSE_GLFW $(CFLAGS_WIN) -o $(target) $(src_min) $(srcGL) -lm -lglfw3 -lgdi32

glfw-l: $(obj)
	gcc -DGBE_DEBUG -DUSE_GLFW $(CFLAGS) -o $(target_linux) $(src_min) $(srcGL) $(GLFW_PKG)

# no GLFW build
min: $(obj)
	gcc -DTEST_APP_ -o $(target_linux) src/main.c $(GLdir)glad.c $(GLFW_PKG)

tigr: $(obj)
	$(CC) $(CFLAGS) -DUSE_TIGR $(src_min) $(srcTIGR) -o $(target) $(includes) -lm -lopengl32 -lgdi32

minifb: $(obj)
	$(CC) $(CFLAGS) src/main.c src/app.c src/gb.c src/cart.c -o $(target) -lgdi32

# headless
core: $(obj)
	gcc -DGBE_DEBUG $(CFLAGS) $(src_min) -o $(target_linux) -lrt -lm

clean:
	rm -f $(obj) $(target)