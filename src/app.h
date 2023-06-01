#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "cpu.h"

#define GB_APP_DRAW
#define GB_DEBUG

#ifdef GB_APP_DRAW
    #include "api/glfw/graphics.h"
#endif

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
#ifdef GB_APP_DRAW
        struct Texture tileMap;
        struct Texture frameBuffer;
#endif
    };
    struct gb_data gbData;
    
#ifdef GB_APP_DRAW
    /* Drawing elements */
    GLFWwindow     * window;
    Scene            display;
    struct Texture * image;
#endif
};

uint8_t * gb_load (const char *);

void app_config (struct App * app, uint8_t const argc, char * const argv[]);
void app_init   (struct App * app);
void app_run    (struct App * app);