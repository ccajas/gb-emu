CFLAGS = -Wall -s -Os -std=gnu89
src = g_hello.c glad.c ../../gb.c ../../cart.c
CC = i686-w64-mingw32-gcc
#"i686-w64-mingw32-gcc"
# Output
target = g_hello
all: glfw

.PHONY: clean
LDFLAGS = `pkg-config --libs glfw3`

# main build
glfw: $(obj)
	i686-w64-mingw32-gcc $(CFLAGS) $(src) -o $(target) -lm -lglfw3 -lgdi32

glfw-l: $(obj)
	cc $(CFLAGS) $(src) -o $(target) -lm -ldl  $(LDFLAGS)

clean:
	rm -f $(obj) $(target)