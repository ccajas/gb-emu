#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "api/glfw/graphics.h"

#define DISPLAY_WIDTH   160
#define DISPLAY_HEIGHT  144

#define GB_DEBUG

#ifdef GB_DEBUG
    #define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
#endif

/* Container for relevant emulation data */

struct gb_data
{
    /* Store ROM data */
    uint8_t * bootRom;
    uint8_t * rom;

    /* Used in drawing VRAM */
    //uint8_t vram_raw[VRAM_SIZE * 2 * 3];

    /* Used for drawing the display and tilemap */
    struct Texture tileMap;
    struct Texture frameBuffer;
};

struct App
{
    uint8_t draw;
    uint8_t scale;
    clock_t time;
    uint8_t paused;

    struct gb_data gbData;
    char defaultFile[256];

    /* Drawing elements */
    GLFWwindow     * window;
    Scene            display;
    struct Texture * image;
};

uint8_t * app_load (struct App const * app);

void app_config (struct App * app, uint8_t const argc, char * const argv[]);
void app_init   (struct App * app);
void app_run    (struct App * app);