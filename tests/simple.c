#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <stdio.h>

#include "../src/gb.h"
#include "../src/cart.h"

struct GB        dummyGB;
struct Cartridge dummyCart;

void setup(void) {
    printf("Runs before the test");
    dummyGB.cart = dummyCart;
    dummyGB.cart.romData = {0};
}

Test(no_rom, passing) {
    cr_log_info("No rom...\n");
    cr_assert(eq(int, 0, gb_rom_loaded (&dummyGB)));
}
/*
Test(test1, passing) {
    cr_assert(1);
}*/