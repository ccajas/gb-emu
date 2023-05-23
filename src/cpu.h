#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define CPU_FREQ  (2 << 10) * 4

typedef struct CPU_struct
{
    enum { A = 0, B, C, D, E, H, L, F = 10 } registers;

    uint8_t r[7];     /* A-E, H, L - 8-bit registers */
    uint8_t rm, rt;
    union
    {
        /* data */
        struct 
        {
            uint8_t f_lb : 4; /* Unused */
            uint8_t f_c  : 1;
            uint8_t f_h  : 1;
            uint8_t f_n  : 1;
            uint8_t f_z  : 1;
        };
        uint8_t flags;
    };
    
    uint16_t pc, sp;
    uint64_t clock_m, clock_t;

    uint8_t stop, halt;
    uint8_t invalid;

    /* Interrupt */
    uint8_t ime;

#ifdef GB_DEBUG
    /* For testing purposes */
    uint16_t ni;
    char reg_names[7];
#endif
}
CPU;

/* Forward declaration */
typedef struct GB_struct GameBoy;

/* Function definitions */
void cpu_init ();
void cpu_boot_reset ();
void cpu_state ();

uint8_t cpu_step    ();
void    cpu_exec    (uint8_t const op);
void    cpu_exec_cb (uint8_t const op);

#endif