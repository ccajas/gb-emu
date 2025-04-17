CFLAGS   = -Wall -s -O2 -std=gnu89 -DGBE_DEBUG -DUSE_GLFW -DENABLE_AUDIO -MMD -MP
GLFW_PKG = `pkg-config --static --libs glfw3`
CC       = gcc

GLdir   = src/api/gl/
src     = $(wildcard src/*.c)
src_min = src/main.c src/app.c src/gb.c src/cart.c
srcGL   = $(wildcard src/api/gl/*.c)
srcTIGR = $(wildcard src/api/tigr/*.c)

#CC = i686-w64-mingw32-gcc

INC = -I../_include  -L../_lib 

include Makefile.ma

lib = $(src:.c=.a)
obj = $(src:.c=.o)

# Output
target = bin/gb-emu.exe
target_linux = bin/gb-emu

all: glfw

OS_NAME := $(shell uname -s | tr A-Z a-z)

.PHONY: clean

# main build
ifeq ($(OS_NAME),linux)
glfw: $(obj)
	@echo $(OS_NAME)
	$(CC) $(CFLAGS) $(MA_FLAGS) $(src) $(miniaudio) $(srcGL) $(GLFW_PKG) -o $(target_linux)
else
glfw: $(obj)
	@echo $(OS_NAME)
	$(CC) $(CFLAGS) $(MA_FLAGS) $(src) $(miniaudio) $(srcGL) ../_lib/libglfw3.a -lgdi32 -o $(target)
endif

# no GLFW build
min: $(obj)
	$(CC) -DTEST_APP_ -o $(target_linux) src/main.c $(GLdir)glad.c $(GLFW_PKG)

tigr: $(obj)
	$(CC) $(CFLAGS) -DUSE_TIGR $(src_min) $(srcTIGR) -o $(target) $(includes) -lm -lopengl32 -lgdi32

minifb: $(obj)
	$(CC) $(CFLAGS) src/main.c src/app.c src/gb.c src/cart.c -o $(target) -lgdi32

# headless
core: $(obj)
	gcc -Wall -s -O2 -std=gnu89 -DGBE_DEBUG $(src_min) -o $(target_linux) -lrt -lm

corew: $(obj)
	gcc -Wall -s -O2 -std=gnu89 -DGBE_DEBUG $(src_min) -o $(target)

clean:
	rm -f $(obj) $(target)