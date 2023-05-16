#ifndef GB_H
#define GB_H

#include <stdio.h>
#include "mmu.h"
#include "cpu.h"
#include "cart.h"

#define GB_TESTING

#ifdef GB_TESTING
    #define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
#endif

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

extern GameBoy GB;
extern CPU * cpu;
extern MMU * mmu;

#endif