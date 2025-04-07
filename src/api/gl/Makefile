CFLAGS = -Wall -s -O2 -std=gnu89 -DGBE_DEBUG -DUSE_GLFW
PKG = `pkg-config --static --libs glfw3`
src = $(wildcard ../../*.c) $(wildcard *.c)

include Makefile.ma

CFLAGS += $(MA_FLAGS)
src += $(miniaudio)

CC = i686-w64-mingw32-gcc
build_dir = ../../../bin

# Output
target = $(build_dir)/gb-emu
all: glfw

.PHONY: clean
LDFLAGS = `pkg-config --libs glfw3`

# main build
glfw: $(obj)
	i686-w64-mingw32-gcc $(CFLAGS) -o $(target) $(src) -lm -lglfw3 -lgdi32

gb-emu: $(obj)
	gcc $(CFLAGS) -o $(target) $(src) $(PKG)

clean:
	rm -f $(obj) $(target)