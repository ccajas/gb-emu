#ifndef GB_H
#define GB_H

#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include "cart.h"

#define GB_DEBUG__

#ifdef GB_DEBUG
    #define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
#endif

/* General functions that can be exposed to the frontend */

struct gb_func
{
    /* Reads one byte from ROM */
    uint8_t (*gb_rom_read)(void *, const uint_fast32_t addr);

    /* Draws one line from the display */
    void (*gb_draw_line)(void *, const uint8_t * pixels, const uint8_t line);
};

/* Debug functions for the frontend */

struct gb_debug
{
    void (*peek_vram)   (void *, const uint8_t *);
    void (*update_tiles)(void *, const uint8_t *);
};

typedef struct gb_struct 
{
    /* Game Boy components */
    CPU       cpu;
    MMU       mmu;
    PPU       ppu;
    Cartridge cart;

    /* Controller */
    uint8_t controller;
    uint8_t controllerState;

    /* Master clock */
    uint64_t clockCount;
    uint64_t stepCount;

    /* Time keeping */
    int32_t  frameClock;
    uint32_t frames;

    /* Direct access to internal data */
    struct Direct
	{
        /* Set status of the emulation */
        uint8_t paused;
        uint8_t debugMode;
        uint8_t shouldReset;

        /* Pointer to frontend data */
		void * ptr;
	}
    direct;

    /* Exposed functions */
    struct gb_func  * gb_func;
    struct gb_debug * gb_debug;
}
GameBoy;

void gb_init     (GameBoy * const, void *, struct gb_func  *, struct gb_debug *);
void gb_shutdown (GameBoy * const);

uint8_t  gb_step         (GameBoy * const);
uint32_t gb_frame        (GameBoy * const);
void     gb_print_logo   (GameBoy * const, const uint8_t);
void     gb_unload_cart  (GameBoy * const);

#endif