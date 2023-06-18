#ifndef PALETTES_H
#define PALETTES_H

#include <stdint.h>

typedef struct 
{
    char name[32];
    uint8_t colors[4][3];
}
GB_Palette;

#define RGB_(X)  { (X >> 16), ((X >> 8) & 255), (X & 255) }
#define RGB_4(P1, P2, P3, P4)  RGB_(P1), RGB_(P2), RGB_(P3), RGB_(P4)

static const GB_Palette palettes[] =
{
    { "Default",       { RGB_4 (0,        0x555555, 0xAAAAAA, 0xF0F0F0) }},
    { "DMG Original",  { RGB_4 (0x1C342D, 0x30584B, 0x527542, 0x959501) }},
    { "DMG Redux",     { RGB_4 (0x173620, 0x306230, 0x7B9C0F, 0x95A501) }},
    { "Pocket",        { RGB_4 (0x181B19, 0x2D322C, 0x4E5446, 0x8E9676) }},
    { "Super GameBoy", { RGB_4 (0x331e50, 0xA63725, 0xD68E49, 0xF7E7C6) }},
    { "Pocket HC",     { RGB_4 (0x121212, 0x383838, 0x646464, 0xB7B7B7) }},
    { "BLK AQU4",      { RGB_4 (0x002B59, 0x005F8C, 0x00B9BE, 0x9FF4E5) }},
    { "BGB",           { RGB_4 (0x081820, 0x346856, 0x88C070, 0xE0F8D0) }},
    { "Velvet Cherry", { RGB_4 (0x2D162C, 0x412752, 0x683A68, 0x9775A6) }},
    { "Wish GB",       { RGB_4 (0x622E4C, 0x7550E8, 0x608FCF, 0x8BE5FF) }},
    { "Crimson",       { RGB_4 (0x1B0326, 0x7A1C4B, 0xBA5044, 0xEFF9D6) }},
    { "Hollow",        { RGB_4 (0x0F0F1B, 0x565A75, 0xC6B7BE, 0XFAFBF6) }}
};

#endif