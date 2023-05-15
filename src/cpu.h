#ifndef CPU_H
#define CPU_H

#include <stdint.h>

enum registers { A = 0, B, C, D, E, H, L };

typedef struct CPU_struct
{
    uint8_t r[7];     /* A-E, H, L - 8-bit registers */
    uint8_t rm, rt;
    uint8_t flags;
    uint16_t pc, sp;
    uint64_t clock_m, clock_t;

    uint8_t stop, halt;
    char reg_names[7];

    /* Testing */
    uint16_t ni;
}
CPU;

/* Forward declaration */
typedef struct GB_struct GameBoy;

/* Function definitions */
void cpu_init ();
void cpu_print_regs ();
void cpu_boot_reset ();
void cpu_clock ();
void cpu_exec (uint8_t const);

#endif