#ifndef GB_H
#define GB_H

#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include "cart.h"

#define GB_DEBUG_

#ifdef GB_DEBUG
    #define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
#endif

#define CART_MIN_SIZE_KB  0x20

typedef struct GB_struct 
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

    /* Emulation status */
    union 
    {
        struct
        {
            uint8_t running : 1;
            uint8_t romLoaded : 1;
        };
        uint8_t status;
    };
}
GameBoy;

inline void gb_reset(GameBoy * const gb)
{
    mmu_reset (&gb->mmu);
    cpu_boot_reset (&gb->cpu);
}

void gb_init     (GameBoy * const, const char *);
void gb_shutdown (GameBoy * const);

uint8_t gb_step        (GameBoy * const);
void    gb_frame       (GameBoy * const);
void    gb_print_logo  (GameBoy * const, const uint8_t);
void    gb_load_cart   (GameBoy * const, const char *);
void    gb_unload_cart (GameBoy * const);

extern GameBoy GB;
extern CPU * cpu;
extern MMU * mmu;

#endif