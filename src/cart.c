#include "cart.h"

/* Read/write implementations for different MBCs */

uint8_t none_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */
        if (addr <= 0x7FFF) return cart->romData[addr];
    /* No registers to write here */
    return 0xFF;
}

uint8_t mbc1_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */ 
    {
        if (addr <= 0x3FFF) return cart->romData[addr];
        if (addr <= 0x7FFF) {
            if (!cart->bankMode && cart->romSizeKB >= 1024)
                return cart->romData[((cart->bankHi << 5) + cart->bankLo) * 0x4000 + addr];
            else
                return cart->romData[(cart->bankLo - 1) * 0x4000 + addr];
        }
        if (addr >= 0xA000 && addr <= 0xBFFF) {
            if (cart->ram) {
                /* Select RAM bank and fetch data (if enabled) */ 
                if (!cart->ramEnabled) return 0xFF;
                const uint16_t ramOffset = ((cart->bankMode) ? (cart->bankHi * 0x2000) : 0);
                return cart->ramData[(addr % 0x2000) + ramOffset];
            }
        }
        return 0xFF;
    }
    else /* Write to registers */
    {
        if (addr <= 0x1FFF) { cart->ramEnabled = ((val & 0xF) == 0xA) ? 1 : 0; return 0; } /* Enable RAM  */
        if (addr <= 0x3FFF) {
            cart->bankLo = (val & 0x1F) | (cart->bankLo & 0x60);              /* Write lower 5 bank bits  */
            cart->bankLo += (!cart->bankLo) ? 1 : 0; return 0;
        }
        if (addr <= 0x5FFF) { cart->bankHi = val; return 0; }                 /* Write upper 2 bank bits  */
        if (addr <= 0x7FFF) { cart->bankMode = (val & 1); return 0; } 
    }
    return 0;
}

uint8_t mbc2_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */
    {
        if (addr <= 0x3FFF) return cart->romData[addr];
        if (addr <= 0x7FFF) return cart->romData[(cart->bankLo - 1) * 0x4000 + addr];
        if (addr >= 0xA000 && addr <= 0xBFFF)                                 /* Return only lower 4 bits  */
            return (cart->ramEnabled) ? (cart->ramData[(addr & 0x1FF)] & 0xF) : 0xF;
    }
    else /* Write to registers */
    {
        if (addr <= 0x3FFF) {
            if (addr & 0x10) { cart->bankLo = (val == 0) ? 1 : (val & 0xF); } /* LSB == 1, ROM bank select */
            if (!(addr & 0x10)) { cart->ramEnabled = (val == 0xA) ? 1 : 0;  } /* LSB == 0, RAM enable      */
        }
    }
    return 0;
}

uint8_t mbc3_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */
    {
        if (addr <= 0x3FFF) return cart->romData[addr];
        if (addr <= 0x7FFF) return cart->romData[(cart->bankLo - 1) * 0x4000 + addr];
    }
    else
    {
        if (addr <= 0x1FFF) { 
            cart->ramEnabled = ((val & 0xF) == 0xA) ? 1 : 0; return 0; }      /* Enable both RAM and RTC   */
        if (addr <= 0x3FFF) { 
            cart->bankLo = (val == 0) ? 1 : (val & 0x7F); return 0; }         /* Write lower 7 bank bits  */
        if (addr <= 0x5FFF) {
            if (val < 4) { cart->bankHi = val & 3; return 0; }                /* Lower 3 bits for RAM     */
        }
    }
    return 0;
}