#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define GB_DEBUG

typedef struct CPU_struct
{
    enum { A = 0, B, C, D, E, H, L, F = 10 } registers;

    union
    {
        /* Only works for little endian */
        struct {
            uint8_t  _a;
            uint16_t _bc;
            uint16_t _de;
            uint16_t _hl;
        };
        uint8_t r[7];     /* A-E, H, L - 8-bit registers */
    };
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

    uint8_t stop, halted;
    uint8_t invalid;

    /* Interrupt master enable */
    uint8_t ime;

#ifdef GB_DEBUG
    /* For testing purposes */
    uint16_t ni;
    char reg_names[7];
#endif
}
CPU;

/* Forward declarations */
typedef struct gb_struct  GameBoy;
typedef struct MMU_struct MMU;

/* Function definitions */
void cpu_init       (CPU * const);
void cpu_boot_reset (CPU * const);
void cpu_state      (CPU * const, MMU * const);

uint8_t cpu_step    (CPU * const, MMU * const);
void    cpu_exec_cb (CPU * const, MMU * const, uint8_t const op);

#endif