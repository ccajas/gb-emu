CFLAGS     = -Wall -s -Ofast -std=gnu89 -MMD -MP
BUILDFLAGS = -DGBE_DEBUG -DENABLE_LCD -DENABLE_AUDIO
GLFW_PKG   = `pkg-config --static --libs glfw3`
CC         = gcc

flags     = $(CFLAGS) $(BUILDFLAGS) $(MA_FLAGS)

GLdir     = src/api/gl/

src       = src/gb.c src/cart.c
src_min   = src/main.c src/app.c $(src)
src_bench = src/bench/bench.c $(src)
src_tests = src/tests/test-cpu.c $(src)

srcGL     = $(wildcard src/api/gl/*.c)
srcTIGR   = $(wildcard src/api/tigr/*.c)

#CC = i686-w64-mingw32-gcc

INC = -I../_include  -L../_lib 

include Makefile.ma

lib = $(src:.c=.a)
obj = $(src:.c=.o)

# Output
target = bin/gb-emu.exe
target_linux = bin/gb-emu

all: glfw

OS_NAME := $(shell uname -o | tr A-Z a-z)

.PHONY: bench clean

# main build
ifeq ($(OS_NAME),gnu/linux)
glfw: $(obj)
	@echo $(OS_NAME)
	$(CC) $(flags) -DUSE_GLFW $(src_min) $(miniaudio) $(srcGL) $(GLFW_PKG) -o $(target_linux)
else
#-Wl,-subsystem,windows 
glfw: $(obj)
	@echo $(OS_NAME)
	$(CC) $(flags) -DUSE_GLFW $(src_min) $(miniaudio) $(srcGL) ../_lib/libglfw3.a -lgdi32 -o $(target)
endif

# no GLFW builds
min: $(obj)
	$(CC) -DTEST_APP_ -o $(target_linux) src/main.c $(GLdir)glad.c $(GLFW_PKG)

#tigr: $(obj)
#	$(CC) $(CFLAGS) -DUSE_TIGR -DGBE_DEBUG -DENABLE_LCD $(src_min) $(srcTIGR) -o $(target) -lm -lopengl32 -lgdi32

#minifb: $(obj)
#	$(CC) $(CFLAGS) $(src_min) -o $(target) -lgdi32

# headless
core: $(obj)
ifeq ($(OS_NAME),gnu/linux)
	gcc -Wall -s -O2 -std=gnu89 -DGBE_DEBUG $(src_min) -o $(target_linux) -lrt -lm
else
	gcc -Wall -s -O2 -std=gnu89 -DGBE_DEBUG $(src_min) -o $(target)
endif

clean:
	rm -f $(obj) $(target)

# extra
tests: $(obj)
	rm -f $(obj) tests/test-cpu
	gcc -Wall -s -O2 -std=gnu89 $(src_tests) -o tests/test-cpu

bench: $(obj)
	gcc -Wall -s -Ofast -std=gnu89 -mtune=native -DENABLE_LCD $(src_bench) -o bin/gb-bench-emu