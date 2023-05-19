#ifndef GB_H
#define GB_H

#include <stdio.h>
#include "mmu.h"
#include "cpu.h"
#include "cart.h"

#define GB_DEBUG_

#ifdef GB_DEBUG
    #define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
#endif

#define BOOT_CODE_SIZE    0x100
#define CART_MIN_SIZE_KB  0x20

typedef struct GB_struct 
{
    /* Game Boy components */
    CPU       cpu;
    MMU       mmu;
    Cartridge cart;

    /* Controller */
    uint8_t controller;
    uint8_t controllerState;

    /* Master clock */
    uint64_t clockCount;
}
GameBoy;

inline void gb_reset(GameBoy * const gb)
{
    mmu_reset (&gb->mmu);
    cpu_boot_reset (&gb->cpu);
}

void gb_load_cart   (GameBoy * const);
void gb_unload_cart (GameBoy * const);
void gb_print_logo  (GameBoy * const, const uint8_t);

extern GameBoy GB;
extern CPU * cpu;
extern MMU * mmu;

#endif