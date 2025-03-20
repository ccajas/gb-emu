#ifndef APP_H
#define APP_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "gbdebug.h"
#include "gb.h"
#include "texture.h"

/* #define GB_DEBUG */
#define DEBUG_TEXTURE_W  320
#define DEBUG_TEXTURE_H  288
#define DEFAULT_SCALE    3

#define USE_GLFW

#ifdef USE_GLFW
    #define GLFW_INCLUDE_NONE
    #include "api/glfw/graphics.h"
    #include <GLFW/glfw3.h>
#endif

#ifdef USE_TIGR
    #include "api/tigr/tigr.h"
#endif

#ifdef GB_DEBUG
    #define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
#endif

struct App
{
    uint8_t draw, paused, step;
    uint8_t debug;

    /* Used for input */
    uint8_t  GBbuttons[8];
    uint16_t GBkeys[8];
    uint8_t  joystickID;

    /* Cosmetic options */
    uint8_t scale;
    uint8_t fullScreen : 1;

    char defaultFile[256];
    char debugString[64], fpsString[32];

    /* Container for GB emulation data */
    struct gb_data
    {
        /* Used for drawing the display and tilemap */
#ifdef USE_GLFW
        struct Texture tileMap;
        struct Texture frameBuffer;
#endif
#ifdef USE_TIGR
        Tigr * tileMap;
        Tigr * frameBuffer;
#endif        
        uint8_t paletteBG, paletteOBJ;
        uint8_t pixelTint;
    }
    gbData;

    /* Pointers to main and debug functions */
    struct GB gb;
    
#ifdef USE_GLFW
    /* Drawing elements */
    GLFWwindow     * window;
    Scene            display;
    struct Texture * image;
#endif
#ifdef USE_TIGR
    Tigr * screen;
#endif
};

uint8_t * app_load (struct GB *, const char * romName);

void app_config (struct App *, uint8_t const argc, char * const argv[]);
void app_init   (struct App *);
void app_run    (struct App *);

/* Functions that reference frontend app data from emulator */
void app_draw_line (void * dataPtr, const uint8_t * pixels, const uint8_t line);

/* Drawing functions */
static inline void app_imgPtr (struct Texture * texture, const uint32_t pos)
{
    texture->ptr = texture->imgData + pos;
}

static inline void app_imgPtr_XY (struct Texture * texture, const uint16_t x, const uint16_t y)
{
    app_imgPtr (texture, (texture->width * y + x) * 3);
}

void app_draw (struct App *);

#endif