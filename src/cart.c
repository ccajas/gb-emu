#include <stdio.h>
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
        if (addr <= 0x3FFF) {
            if (!(cart->romSizeKB >= 1024 && cart->mode == 1))
                return cart->romData[addr];
            else
                return cart->romData[((cart->bank2nd << 5) & 0x60) * 0x4000 + addr];
        }
        if (addr <= 0x7FFF) {
            if (cart->romSizeKB >= 1024)
                return cart->romData[((cart->bank2nd << 5) + cart->bank1st - 1) * 0x4000 + addr];
            else
                return cart->romData[((cart->bank1st & cart->romMask) - 1) * 0x4000 + addr];
        }
        if (addr >= 0xA000 && addr <= 0xBFFF) {
            if (cart->ram) {                                /* Select RAM bank and fetch data (if enabled) */ 
                if (!cart->usingRAM) return 0xFF;
                const uint16_t ramOffset = (cart->mode == 1) ? (cart->bank2nd * 0x2000) : 0;
                return cart->ramData[(addr % 0x2000) + ramOffset];
            }
        }
        return 0xFF;
    }
    else /* Write to registers */
    {
        if (addr <= 0x1FFF) { cart->usingRAM = ((val & 0xF) == 0xA) ? 1 : 0; return 0; }    /* Enable RAM  */
        if (addr <= 0x3FFF) {
            cart->bank1st = (val & 0x1F);                                      /* Write lower 5 bank bits  */
            cart->bank1st += (val == 0 || (val & 0x60) != 0) ? 1 : 0; return 0;
        }
        if (addr <= 0x5FFF) { cart->bank2nd = val; return 0; }                 /* Write upper 2 bank bits  */
        if (addr <= 0x7FFF) { cart->mode = (val & 1); return 0; }              /* Simple/RAM banking mode  */
    }
    return 0;
}

uint8_t mbc2_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */
    {
        if (addr <= 0x3FFF) return cart->romData[addr];
        if (addr <= 0x7FFF) 
            return cart->romData[((cart->bank1st & cart->romMask) - 1) * 0x4000 + addr];
        if (addr <= 0xBFFF)                                                          /* Return only lower 4 bits  */
            return (cart->usingRAM == 1) ? (cart->ramData[(addr & 0x1FF)] & 0xF) : 0xF;
    }
    else /* Write to registers */
    {
        if (addr <= 0x3FFF) {
            if (addr & 0x100)    { cart->bank1st = ((val == 0) ? 1 : (val & 0xF)); } /* LSB == 1, ROM bank select */
            if (!(addr & 0x100)) { cart->usingRAM = (val == 0xA) ? 1 : 0; }          /* LSB == 0, RAM enable      */
        }
    }
    return 0;
}

uint8_t mbc3_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */
    {
        if (addr <= 0x3FFF) return cart->romData[addr];
        if (addr <= 0x7FFF) return cart->romData[((cart->bank1st & cart->romMask) - 1) * 0x4000 + addr];
    }
    else /* Write to registers */
    {
        if (addr <= 0x1FFF) { cart->usingRAM = ((val & 0xF) == 0xA) ? 1 : 0; return 0; }    /* Enable both RAM and RTC   */
        if (addr <= 0x3FFF) { cart->bank1st = (val == 0) ? 1 : (val & 0x7F); return 0; }    /* Write lower 7 bank bits  */
        if (addr <= 0x5FFF) { if (val < 4) { cart->bank2nd = val & 3; return 0; } }         /* Lower 3 bits for RAM     */
    }
    return 0;
}

uint8_t mbc5_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */
    {
        if (addr <= 0x3FFF) return cart->romData[addr];
        if (addr <= 0x7FFF) {                                          /* Combine 9th bit with lower 8 bits */
            if (cart->romSizeKB >= 4096)
                return cart->romData[(((cart->bank2nd & 1) << 8) + cart->bank1st - 1) * 0x4000 + addr];
            else
                return cart->romData[((cart->bank1st & cart->romMask) - 1) * 0x4000 + addr];
        }
        if (addr >= 0xA000 && addr <= 0xBFFF)
            return (cart->usingRAM == 1) ?
                cart->ramData[(cart->bank2nd >> 4) * 0x2000 + (addr % 0x2000)] : 0xFF;
    }
    else /* Write to registers */
    {    /* RAM banks are stored in upper 4 bits of bank2nd, and shifted down for selecting the bank       */
        if (addr <= 0x1FFF) { 
            cart->usingRAM = ((val & 0xF) == 0xA) ? 1 : 0; return 0; }         /* Enable RAM               */
        if (addr <= 0x2FFF) { cart->bank1st = val; return 0; }                 /* Write 8 lower bank bits  */
        if (addr <= 0x3FFF) { cart->bank2nd |= (val & 1); return 0; }          /* Write 9th bank bit       */
        if (addr <= 0x5FFF) { cart->bank2nd |= (val & 0xF) << 4; return 0; }   /* Upper 4 bits for RAM     */
    }
    return 0;
}

/* Array to select the read/write functions from */

uint8_t (* cart_rw[])(struct Cartridge *, const uint16_t, const uint8_t, const uint8_t) =
{
    none_rw, mbc1_rw, mbc2_rw, mbc3_rw, NULL, mbc5_rw
};

/* Read cart and load metadata and MBC info */

void cart_identify (struct Cartridge * cart)
{
    /* Read checksum and see if it matches data */
    uint8_t checksum = 0;
    uint16_t addr;
    for (addr = 0x134; addr <= 0x14C; addr++) {
        checksum = checksum - cart->romData[addr] - 1;
    }
    if (cart->romData[0x14D] != checksum)
        LOG_(" %c%c Invalid checksum!\n", 192,196);
    else
        LOG_(" %c%c Valid checksum '%02X'\n", 192,196, checksum);

    /* Get cartridge type and MBC from header */
    memcpy (cart->header, cart->romData + 0x100, 80 * sizeof(uint8_t));
    const uint8_t * header = cart->header;

    const uint8_t cartType = header[0x47];

    /* Select MBC for read/write */
    switch (cartType)
    {
        case 0:             cart->mbc = 0; break;
        case 0x1  ... 0x3:  cart->mbc = 1; break;
        case 0x5  ... 0x6:  cart->mbc = 2; break;
        case 0xF  ... 0x13: cart->mbc = 3; break;
        case 0x19 ... 0x1E: cart->mbc = 5; break;
        default: 
            LOG_("GB: MBC not supported.\n"); return;
    }
    cart->rw = cart_rw[cart->mbc];

    printf ("GB: ROM file size (KiB): %d\n", 32 * (1 << header[0x48]));
    printf ("GB: Cart type: %02X Mapper type: %d\n", header[0x47], cart->mbc);
    
    const uint8_t ramBanks[] = { 0, 0, 1, 4, 16, 8 };

    /* Add other metadata */
    cart->romSizeKB = 32 * (1 << header[0x48]);
    cart->ramSizeKB = 8 * ramBanks[header[0x49]];
    cart->romMask = (1 << (header[0x48] + 1)) - 1;
    cart->bank1st = 1;
    cart->bank2nd = 0;
    cart->romOffset = 0x4000;
    cart->ramOffset = 0;
}