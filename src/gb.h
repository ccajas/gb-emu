#ifndef GB_H
#define GB_H

#include <stdio.h>
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

#define BOOT_CODE_SIZE    0x100
#define CART_MIN_SIZE_KB  0x20

#define TEST_MAX_STEPS    70000

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

void gb_init     (GameBoy * const);
void gb_shutdown (GameBoy * const);

void gb_run         (GameBoy * const);
void gb_load_cart   (GameBoy * const);
void gb_unload_cart (GameBoy * const);
void gb_print_logo  (GameBoy * const, const uint8_t);

extern GameBoy GB;
extern CPU * cpu;
extern MMU * mmu;

#endif