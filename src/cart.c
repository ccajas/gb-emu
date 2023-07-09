#include <stdio.h>
#include <stdlib.h>
#include "cart.h"

#define LOW_BANK       (addr <= 0x3FFF)
#define HIGH_BANK      (addr >= 0x4000 && addr <= 0x7FFF)
#define RAM_BANK       (addr >= 0xA000 && addr <= 0xBFFF)
#define RAM_ENABLE_REG (addr <= 0x1FFF)
#define BANK_SELECT    (addr >= 0x2000 && addr <= 0x3FFF)
#define BANK_SELECT_2  (addr >= 0x4000 && addr <= 0x5FFF)
#define BANK_SELECT_L  (addr >= 0x2000 && addr <= 0x2FFF)
#define BANK_SELECT_H  (addr >= 0x3000 && addr <= 0x3FFF)
#define MODE_SELECT    (addr >= 0x6000 && addr <= 0x7FFF)

/* Read/write implementations for different MBCs */

uint8_t none_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */
        if (addr <= 0x7FFF) return cart->romData[addr];
    /* Nothing to write here */
    return 0xFF;
}

uint8_t mbc1_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    switch (write) {
        case 0: /* Read from cartridge */
            if LOW_BANK {                                                  /* User upper 2 bank bits in mode 1 */
                const uint8_t selectedBank = (cart->romBank2 << 5) & cart->romMask;
                return cart->romData[addr + ((cart->mode == 1) ? (selectedBank * 0x4000) : 0)];
            }
            if HIGH_BANK {
                const uint8_t bank = (cart->romBank1 == 0) ? 1 : cart->romBank1;
                const uint8_t selectedBank = ((cart->romBank2 << 5) + bank) & cart->romMask;
                return cart->romData[(selectedBank - 1) * 0x4000 + addr];
            }
            if (RAM_BANK && cart->ram) {                        /* Select RAM bank and fetch data (if enabled) */
                if (!cart->usingRAM) return 0xFF;
                if (cart->ramSizeKB == 8) return cart->ramData[addr % 0x2000];     /* Fetch only lower 8KB     */
                const uint16_t ramOffset = (cart->mode == 1) ? (cart->romBank2 * 0x2000) : 0;
                return cart->ramData[(addr % 0x2000) + ramOffset];
            }
            return 0xFF;
        break;
        case 1: /* Write to registers */
            if RAM_ENABLE_REG  cart->usingRAM = ((val & 0xF) == 0xA);              /* Enable RAM  */
            if BANK_SELECT     cart->romBank1 = val & 0x1F;                        /* Write lower 5 bank bits  */
            if BANK_SELECT_2   cart->romBank2 = (val & 3);                         /* Write upper 2 bank bits  */
            if MODE_SELECT     cart->mode = (val & 1);                             /* Simple/RAM banking mode  */
            if (RAM_BANK && cart->ram) {                                           /* Write to RAM if enabled  */
                if (!cart->usingRAM) return 0;           /* Write only to lower 8KB if mode 0 or smaller bank  */
                if (cart->ramSizeKB == 8) { cart->ramData[addr % 0x2000] = val; return 0; }
                const uint16_t ramOffset = (cart->mode == 1) ? (cart->romBank2 * 0x2000) : 0;
                cart->ramData[(addr % 0x2000) + ramOffset] = val;
            }
        break;
    }
    return 0;
}

uint8_t mbc2_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */
    {
        if LOW_BANK   return cart->romData[addr];
        if HIGH_BANK {
            const uint8_t selectedBank = (cart->romBank1 == 0 ? 1 : cart->romBank1) & cart->romMask;
            return cart->romData[(selectedBank - 1) * 0x4000 + addr];
        }
        if RAM_BANK {
            if (!cart->usingRAM) return 0;
            uint8_t v = (cart->ramData[addr & 0x1FF] & 0xF);     /* Return only lower 4 bits  */
            return v;
        }
    }
    else /* Write to registers */
    {
        if LOW_BANK {
            const uint8_t setRomBank = (addr >> 8) & 1;
            if (setRomBank)    cart->romBank1 = val & 0xF;                            /* LSB == 1, ROM bank select */
            else               cart->usingRAM = ((val & 0xF) == 0xA);                 /* LSB == 0, RAM switch      */
        }
        if RAM_BANK {
            if (!cart->usingRAM) return 0xFF; 
            cart->ramData[addr & 0x1FFF] = (val & 0xF) | 0xF0;                        /* Write only lower 4 bits   */
        }
    }
    return 0;
}

uint8_t mbc3_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */
    {
        if LOW_BANK   return cart->romData[addr];
        if HIGH_BANK  return cart->romData[((cart->romBank1 & cart->romMask) - 1) * 0x4000 + addr];
        if (RAM_BANK && cart->ram) {                                     /* Select RAM bank and fetch data (if enabled) */
            if (!cart->usingRAM) return 0xFF;
            if (cart->ramSizeKB == 8) return cart->ramData[addr % 0x2000];                  /* Fetch only lower 8KB     */
            const uint16_t ramOffset = cart->romBank2 * 0x2000;
            return cart->ramData[(addr % 0x2000) + ramOffset];
        }
    }
    else /* Write to registers */
    {
        if RAM_ENABLE_REG  cart->usingRAM = ((val & 0xF) == 0xA);                           /* Enable both RAM and RTC  */
        if BANK_SELECT     cart->romBank1 = ((val == 0) ? 1 : (val & 0x7F));                 /* Write lower 7 bank bits  */
        if BANK_SELECT_2   {                                                                /* Lower 3 bits for RAM     */ 
            cart->ramBank = val & 3; 
            if (cart->ram) cart->bkRamData = cart->ramData + (cart->ramBank * 0x2000);
        }
        if (RAM_BANK && cart->ram) {                                     /* Select RAM bank and fetch data (if enabled) */
            if (!cart->usingRAM) return 0xFF;                                  /* Write only to lower 8KB if no banking */
            const uint16_t ramOffset = (cart->ramSizeKB == 8) ? 0 : cart->ramBank * 0x2000;
            cart->ramData[(addr % 0x2000) + ramOffset] = val;
        }
    }
    return 0xFF;
}

uint8_t mbc5_rw (struct Cartridge * cart, const uint16_t addr, const uint8_t val, const uint8_t write)
{
    if (!write) /* Read from cartridge */
    {
        if LOW_BANK return cart->romData[addr];
        if HIGH_BANK {
            const uint16_t selectedBank =                              /* Combine 9th bit with lower 8 bits */
                ((cart->romBank2 << 8) + cart->romBank1 & cart->romMask);
            return cart->romData[(selectedBank - 1) * 0x4000 + addr];
        }
        if (RAM_BANK && cart->ram)
            return (cart->usingRAM) ? cart->ramData[(cart->ramBank) * 0x2000 + (addr % 0x2000)] : 0xFF;
    }
    else /* Write to registers */
    {
        if RAM_ENABLE_REG  cart->usingRAM = ((val & 0xF) == 0xA);               /* Enable RAM               */
        if BANK_SELECT_L   cart->romBank1 = val;                                /* Write 8 lower bank bits  */
        if BANK_SELECT_H   cart->romBank2 = val & 1;                            /* Write 9th bank bit       */
        if BANK_SELECT_2   cart->ramBank = (val & 0xF);                         /* Lower 4 bits for RAM     */
    }
    return 0;
}

/* Array to select the MBC read/write functions from */

uint8_t (* cart_rw[])(struct Cartridge *, const uint16_t, const uint8_t, const uint8_t) =
{
    none_rw, mbc1_rw, mbc2_rw, mbc3_rw, NULL, mbc5_rw
};

/* Array to select whether or not the cart has a battery */

const uint8_t cartBattery[0x100] = 
{
    [3] = 1, [6] = 1, [9] = 1, [0xD] = 1, [0xF] = 1,
    [0x10] = 1, [0x13] = 1, [0x1B] = 1, [0x1E] = 1,
    [0x22] = 1, [0xFF] = 1
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
    
    const uint8_t ramBanks[] = { 0, 0, 8, 32, 128, 64 };

    /* Add other metadata and init values */
    cart->romSizeKB = 32 * (1 << header[0x48]);
    /* MBC2 has built-in RAM */
    cart->ramSizeKB = (cart->mbc == 2) ? 1 : ramBanks[header[0x49]];
    cart->ram     = (cart->ramSizeKB > 0);
    cart->battery = cartBattery[cartType];
    cart->romMask = (1 << (header[0x48] + 1)) - 1;

    printf ("GB: RAM file size (KiB): %d\n", cart->ramSizeKB);
    printf ("GB: Cart type: %02X Mapper type: %d\n", header[0x47], cart->mbc);
    printf ("GB: This is a %s cart\n", (header[0x43] & 0x80) ? "CGB" : "DMG");
    printf ("GB: Cart has battery: %s\n", cart->battery ? "Yes" : "No");

    if (cart->ramSizeKB) {
        cart->ramData = calloc(cart->ramSizeKB * 1024, sizeof (uint8_t));
        cart->bkRamData = cart->ramData;
    }

    cart->usingRAM = 0;
    cart->romBank1 = (cart->mbc == 5) ? 1 : 0;
    cart->romBank2 = 0;
    cart->ramBank = 0;
    cart->mode = 0;
}