#ifndef PALETTES_H
#define PALETTES_H

#include <stdint.h>

typedef struct 
{
    char name[32];
    uint8_t colors[4][3];
}
GB_Palette;

#define HEX24_(X)  { (X >> 16), ((X >> 8) & 255), (X & 255) }

static const GB_Palette palettes[] =
{
    { "Default",       {{ 0, 0, 0 }, { 85, 85, 85 }, { 170, 170, 170 }, { 240, 240, 240 }}},
    { "DMG Original",  { HEX24_(0x173620), HEX24_(0x306230), HEX24_(0x7B9C0F), HEX24_(0x9BBC0F) }},
    { "BLK AQU4",      { HEX24_(0x002B59), HEX24_(0x005F8C), HEX24_(0x00B9BE), HEX24_(0x9FF4E5) }},
    { "BGB",           { HEX24_(0x081820), HEX24_(0x346856), HEX24_(0x88C070), HEX24_(0xE0F8D0) }},
    { "Velvet Cherry", { HEX24_(0x2D162C), HEX24_(0x412752), HEX24_(0x683A68), HEX24_(0x9775A6) }},
    { "Wish GB",       { HEX24_(0x622E4C), HEX24_(0x7550E8), HEX24_(0x608FCF), HEX24_(0x8BE5FF) }},
    { "Crimson",       { HEX24_(0x1B0326), HEX24_(0x7A1C4B), HEX24_(0xBA5044), HEX24_(0xEFF9D6) }},
    { "Hollow",        { HEX24_(0x0F0F1B), HEX24_(0x565A75), HEX24_(0xC6B7BE), HEX24_(0XFAFBF6) }}
};

#endif