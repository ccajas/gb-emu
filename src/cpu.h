#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "mbc.h"
#include "io.h"

#define GB_DEBUG

#define FRAME_CYCLES  70224

struct CPU
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
    uint32_t frameClock;

    uint8_t stop, halted;
    uint8_t invalid;

    /* Memory and I/O registers */
    uint8_t ram[0x2000]; /* Work RAM  */
    uint8_t io[0x80];
    uint8_t hram[0x80];  /* High RAM  */

    /* Interrupt master enable */
    uint8_t ime;

#ifdef GB_DEBUG
    /* For testing purposes */
    uint16_t ni;
    char reg_names[7];
#endif
};

/* Function definitions */
void cpu_init       ();
void cpu_boot_reset ();
void cpu_state      ();

uint8_t cpu_step  ();
void    cpu_frame ();
uint8_t cpu_mem_access(const uint16_t addr, const uint8_t val, const uint8_t);
uint8_t cpu_exec    (const uint8_t op);
void    cpu_exec_cb (const uint8_t op);

#endif