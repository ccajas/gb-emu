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

#define CART_MIN_SIZE_KB  32

/* Functions that can be exposed to the frontend */

struct gb_func
{
    uint8_t (*gb_rom_read)(void *, const uint16_t addr);
};

/* Debug functions for the frontend */

struct gb_debug
{
    void (*peek_vram)(void *, uint8_t *);
    void (*update_tiles)(GameBoy * const, uint8_t * const);
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
    uint32_t frameClock;
    uint32_t frames;

    /* Used for debugging output */
    uint8_t tileSet[256][8][8];

    /* Direct access to frontend data */
    struct
	{
		void * ptr;
	}
    direct;

    /* Exposed functions */
    struct gb_func  * gb_func;
    struct gb_debug * gb_debug;
}
GameBoy;

inline void gb_reset(GameBoy * const gb)
{
    mmu_reset (&gb->mmu);
    cpu_boot_reset (&gb->cpu);
    
    /* Init PPU with default values */
    PPU defaultPPU = {
        .mode = 0,
        .ticks = 0
    };
    gb->ppu = defaultPPU;
}

void gb_init     (GameBoy * const, void *, 
                  struct gb_func  *,
                  struct gb_debug *);
void gb_shutdown (GameBoy * const);

uint8_t gb_step         (GameBoy * const);
void    gb_frame        (GameBoy * const);
void    gb_debug_update (GameBoy * const);
void    gb_print_logo   (GameBoy * const, const uint8_t);
void    gb_unload_cart  (GameBoy * const);

#endif