#ifndef APP_H
#define APP_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "gbdebug.h"
#include "gb.h"

#define GB_APP_DRAW
#define GB_DEBUG

#include "api/glfw/graphics.h"

#ifdef GB_DEBUG
    #define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
#endif

struct App
{
    uint8_t draw;
    uint8_t scale;
    uint8_t paused;

    char defaultFile[256];

    /* Container for GB emulation data */
    struct gb_data
    {
        uint8_t * bootRom;
        uint8_t * rom;

        /* Used for drawing the display and tilemap */
        struct Texture tileMap;
        struct Texture frameBuffer;
    };
    struct gb_data gbData;

    /* Pointers to main and debug functions */
    struct GB gb;
    
#ifdef GB_APP_DRAW
    /* Drawing elements */
    GLFWwindow     * window;
    Scene            display;
    struct Texture * image;
#endif
};

uint8_t * app_load (const char *);

void app_config (struct App *, uint8_t const argc, char * const argv[]);
void app_init   (struct App *);
void app_run    (struct App *);

#endif