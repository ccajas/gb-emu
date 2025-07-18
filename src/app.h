#ifndef APP_H
#define APP_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "app_settings.h"
#include "gb.h"
#include "palettes.h"

#define USE_BOOT_ROM__

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
#ifdef USE_GLFW
        /* Used for drawing the display and tilemap */
        struct Texture tileMap;
        struct Texture frameBuffer;
#endif
#ifdef USE_TIGR
        Tigr * tileMap;
        Tigr * frameBuffer;
#endif        
        uint8_t palette;
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

#ifdef ENABLE_AUDIO
void app_audio_init (struct App *);
#endif

/* Functions that reference frontend app data from emulator */
uint8_t app_cart_rom_read (void * dataPtr, const uint32_t addr);
void    app_draw_line     (void * dataPtr, const uint8_t * pixels, const uint8_t line);

#if defined(USE_GLFW)
/* Drawing functions */
static inline void app_imgPtr (struct Texture * texture, const uint32_t pos)
{
    texture->ptr = texture->imgData + pos;
}

static inline void app_imgPtr_XY (struct Texture * texture, const uint16_t x, const uint16_t y)
{
    app_imgPtr (texture, (texture->width * y + x) * 3);
}
#endif

void app_draw (struct App *);

#endif