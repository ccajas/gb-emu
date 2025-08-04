#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <stdio.h>

#include "../src/gb.h"
#include "../src/cart.h"
#include "../src/opcycles.h"

struct GB        dummyGB;
struct Cartridge dummyCart;

#define ROM_SIZE 32768

void setup(void) 
{
    cr_log_info("Run before the test");

    dummyCart.romData = calloc(ROM_SIZE + 1, sizeof(uint8_t));
    dummyGB.cart = dummyCart;
    gb_init(&dummyGB, NULL);
}

Test(instr_timing, passing) 
{
    setup();
    cr_log_info("Instruction timing...\n");

    int i;
    for (i = 0; i < 255; i++)
    {
        switch (i)
        {
            case 0x10:
            case 0x76:
            case 0xCB:
                cr_log_info("Skipping instruction 0x%02x\n", i);
                break;
            case 0xD3:
            case 0xDB:
            case 0xDD:
            case 0xE3:
            case 0xE4:
            case 0xEB:
            case 0xEC:
            case 0xED:
            case 0xF4:
            case 0xFC:
            case 0xFD:
                cr_log_info("Invalid instruction, skipping\n");
                break;
            default:
            {
                cr_log_info("Testing instruction 0x%02x\n", i);
                /* Simluate program counter prefetch */
                dummyGB.rm = 1;
                /* Then run the instruction */
                gb_cpu_exec(&dummyGB, i);
                cr_assert_geq(dummyGB.rm, opCycles[i]);

                dummyGB.rm = 0;
            }
        }
    }  
}
/*
Test(test1, passing) {
    cr_assert(1);
}*/