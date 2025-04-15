#ifndef APP_H
#define APP_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "gbdebug.h"
#include "gb.h"
#include "texture.h"
#include "palettes.h"

/* #define GB_DEBUG */
#define DEBUG_TEXTURE_W  320
#define DEBUG_TEXTURE_H  288
#define DEFAULT_SCALE    3

#ifdef ENABLE_AUDIO
    #define MINIAUDIO_IMPLEMENTATION
    #include "../deps/miniaudio/extras/miniaudio_split/miniaudio.h"
#endif

#ifdef GBE_DEBUG
    #define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
#endif

/* Common functions for different APIs */

#if defined(USE_GLFW)
    #define GLFW_INCLUDE_NONE
    #include "api/gl/graphics.h"
    #include <GLFW/glfw3.h>

    #define GBE_APP_CLOSED          glfwWindowShouldClose (app->window)
    #define GBE_GET_TIME()          glfwGetTime()
    #define GBE_DRAW_BEGIN()        draw_begin (&app->display)
    #define GBE_WINDOW_TITLE(s)     glfwSetWindowTitle (app->window, s)
    #define GBE_POLL_EVENTS()       glfwPollEvents()
    #define GBE_APP_CLEANUP()\
        free (app->gbData.tileMap.imgData);\
        free (app->gbData.frameBuffer.imgData);\
        glfwDestroyWindow (app->window);\
        glfwTerminate();\

    #define GBE_DRAW_SWAP_BUFFERS()\
        app_draw (app);\
        glfwSwapBuffers (app->window);\

#elif defined(USE_TIGR)
    #include "api/tigr/tigr.h"

    #define GBE_APP_CLOSED          tigrClosed(screen)
    #define GBE_GET_TIME()          (double)(clock()) / CLOCKS_PER_SEC
    #define GBE_DRAW_BEGIN()        draw_begin (&app->display)
    #define GBE_WINDOW_TITLE(s)
    #define GBE_POLL_EVENTS()
    #define GBE_APP_CLEANUP()       tigrFree(app->window)
    #define GBE_DRAW_SWAP_BUFFERS()\
        app_draw (app);\
        tigrUpdate (app->window);\

#else
    #define GBE_APP_CLOSED          0
    #define GBE_GET_TIME()          (double)(clock()) / CLOCKS_PER_SEC
    #define GBE_DRAW_BEGIN()
    #define GBE_WINDOW_TITLE(s)
    #define GBE_POLL_EVENTS()
    #define GBE_APP_CLEANUP()
    #define GBE_DRAW_SWAP_BUFFERS()

#endif

#define USE_BOOT_ROM

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
        struct Texture tileMap;
        struct Texture frameBuffer;

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

#ifdef ENABLE_AUDIO
    ma_device audioDevice;
#endif
    
#ifdef USE_GLFW
    /* Drawing elements */
    GLFWwindow     * window;
    Scene            display;
#endif
    struct Texture * image;

#ifdef USE_TIGR
    Tigr * screen;
#endif
};

uint8_t * app_load (struct GB *, const char * romName);

void app_config (struct App *, uint8_t const argc, char * const argv[]);
void app_init   (struct App *);
void app_run    (struct App *);

#ifdef ENABLE_AUDIO
void app_audio_init (struct App *);
#endif

/* Functions that reference frontend app data from emulator */
void app_draw_line     (void * dataPtr, const uint8_t * pixels, const uint8_t line);

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