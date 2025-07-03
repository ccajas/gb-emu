#ifndef PALETTES_H
#define PALETTES_H

#include <stdint.h>

typedef struct 
{
    char name[32];
#if defined(COLOR_RGB_565)
    uint16_t colors[4];
#else
    uint8_t colors[4][3];
#endif
}
GB_Palette;

#if defined(COLOR_RGB_565)
    #define RGB_(X)  (uint16_t)(((X & 0xf80000) >> 8) | ((X & 0xfc00) >> 5) | ((X & 0xf8) >> 3))
#else
    #define RGB_(X)  { (X >> 16), ((X >> 8) & 255), (X & 255) }
#endif
#define RGB_4(P1, P2, P3, P4)  RGB_(P1), RGB_(P2), RGB_(P3), RGB_(P4)

static const GB_Palette palettes[] =
{
    { "Default",       { RGB_4 (0,        0x555555, 0xAAAAAA, 0xF0F0F0) }},
    { "DMG Original",  { RGB_4 (0x044140, 0x1B6D62, 0x32AB6A, 0x93C805) }},
    { "DMG Redux",     { RGB_4 (0x083908, 0x316331, 0x8CAD08, 0x9CBD08) }},
    { "Pocket",        { RGB_4 (0x0B0D08, 0x383E36, 0x626957, 0x8E9677) }},
    { "Super GameBoy", { RGB_4 (0x331e50, 0xA63725, 0xD68E49, 0xF7E7C6) }},
    { "SGB DK",        { RGB_4 (0x331e80, 0x8098A0, 0xD8B058, 0xF8D0B8) }},
    { "Cold 1",        { RGB_4 (0x262338, 0x56689D, 0x90ADBB, 0xDCEDEB) }},
    { "Hot 1",         { RGB_4 (0x15017A, 0xF60983, 0xFD9785, 0xFFF5DE) }},
    { "Hot 2",         { RGB_4 (0x842D72, 0xEE316B, 0xFFB037, 0xFBDFB7) }},
    { "Pocket HC",     { RGB_4 (0x121212, 0x383838, 0x646464, 0xB7B7B7) }},
    { "Lite",          { RGB_4 (0x102A23, 0x205643, 0x3A9175, 0x4BC298) }},
    { "Lite Amber",    { RGB_4 (0x2B1C00, 0x523500, 0x7E5300, 0xB68C17) }},
    { "Game.com1",     { RGB_4 (0x1E1C1C, 0x242E30, 0x3D4041, 0x757F7D) }},
    { "Game.com",      { RGB_4 (0x050800, 0x052B1E, 0x41503F, 0x707E64) }},
    { "Test RGB",      { RGB_4 (0x000000, 0x0000CC, 0xCC0000, 0x00CC00) }},
    { "BLK AQU4",      { RGB_4 (0x002B59, 0x005F8C, 0x00B9BE, 0x9FF4E5) }},
    { "BGB",           { RGB_4 (0x081820, 0x346856, 0x88C070, 0xE0F8D0) }},
    { "Velvet Cherry", { RGB_4 (0x2D162C, 0x412752, 0x683A68, 0x9775A6) }},
    { "Wish GB",       { RGB_4 (0x622E4C, 0x7550E8, 0x608FCF, 0x8BE5FF) }},
    { "Crimson",       { RGB_4 (0x1B0326, 0x7A1C4B, 0xBA5044, 0xEFF9D6) }},
    { "Hollow",        { RGB_4 (0x0F0F1B, 0x565A75, 0xC6B7BE, 0XFAFBF6) }}
};

static const GB_Palette manualPalettes[] =
{
    { "GBC_x16_BG",    { RGB_4 (0,        0x525252, 0xA5A5A5, 0xFFFFFF) }},
    { "GBC_x16_OBJ0",  { RGB_4 (0,        0x525252, 0xA5A5A5, 0xFFFFFF) }},
    { "GBC_x16_OBJ1",  { RGB_4 (0,        0x525252, 0xA5A5A5, 0xFFFFFF) }},

    { "GBC_x0D_BG",    { RGB_4 (0,        0x52528C, 0x8C8CDE, 0xFFFFFF) }},
    { "GBC_x0D_OBJ0",  { RGB_4 (0,        0x943A3A, 0xFF8484, 0xFFFFFF) }},
    { "GBC_x0D_OBJ1",  { RGB_4 (0,        0x843100, 0xFFAD63, 0xFFFFFF) }},

    { "GBC_x17_BG",    { RGB_4 (0,        0x856D90, 0xE1AB91, 0xE7E0CA) }},
    { "GBC_x17_OBJ0",  { RGB_4 (0,        0x856D90, 0xE1AB91, 0xE7E0CA) }},
    { "GBC_x17_OBJ1",  { RGB_4 (0,        0x856D90, 0xE1AB91, 0xE7E0CA) }},

    { "GBC_x1A_BG",    { RGB_4 (0,        0x7B4A00, 0xFFFF00, 0xFFFFFF) }},
    { "GBC_x1A_OBJ0",  { RGB_4 (0,        0x0000FF, 0x63A5FF, 0xFFFFFF) }},
    { "GBC_x1A_OBJ1",  { RGB_4 (0,        0x008400, 0x7BFF31, 0xFFFFFF) }},

    { "GBC_x1C_BG",    { RGB_4 (0,        0x0063C5, 0x7BFF31, 0xFFFFFF) }},
    { "GBC_x1C_OBJ0",  { RGB_4 (0,        0x943A3A, 0xFF8484, 0xFFFFFF) }},
    { "GBC_x1C_OBJ1",  { RGB_4 (0,        0x943A3A, 0xFF8484, 0xFFFFFF) }}
};

static const GB_Palette gbcPalettes[] =
{
    /* Default palette */
    { "GBC_x16_BG",    { RGB_4 (0x202020, 0x606060, 0xAAAAAA, 0xEFEFEF) }},
    { "GBC_x16_OBJ0",  { RGB_4 (0x202020, 0x606060, 0xAAAAAA, 0xEFEFEF) }},
    { "GBC_x16_OBJ1",  { RGB_4 (0x202020, 0x606060, 0xAAAAAA, 0xEFEFEF) }},

    { "GBC_x16_BG",    { RGB_4 (0x131614, 0x515652, 0x8F9791, 0xB9C3BC) }},
    { "GBC_x16_OBJ0",  { RGB_4 (0x131614, 0x515652, 0x8F9791, 0xB9C3BC) }},
    { "GBC_x16_OBJ1",  { RGB_4 (0x131614, 0x515652, 0x8F9791, 0xB9C3BC) }},

/*  { "GBC_x16_BG",    { RGB_4 (0x131616, 0x374242, 0x5E6C61, 0x95A694) }},
    { "GBC_x16_OBJ0",  { RGB_4 (0x131616, 0x374242, 0x5E6C61, 0x95A694) }},
    { "GBC_x16_OBJ1",  { RGB_4 (0x131616, 0x374242, 0x5E6C61, 0x95A694) }},*/

    { "DMG Original",  { RGB_4 (0x00261C, 0x177569, 0x5BB734, 0xA6D91F) }},
    { "DMG Original",  { RGB_4 (0x00261C, 0x177569, 0x5BB734, 0xA6D91F) }},
    { "DMG Original",  { RGB_4 (0x00261C, 0x177569, 0x5BB734, 0xA6D91F) }},

    { "DMG 2",         { RGB_4 (0x19311F, 0x5D7C3B, 0xA09E35, 0xF8F49F) }},
    { "DMG 2",         { RGB_4 (0x19311F, 0x5D7C3B, 0xA09E35, 0xF8F49F) }},
    { "DMG 2",         { RGB_4 (0x19311F, 0x5D7C3B, 0xA09E35, 0xF8F49F) }},
    /* Kirby */
    { "GB_x0805_BG",   { RGB_4 (0,        0x006300, 0xFFFF00, 0xA59CFF) }},
    { "GB_x0805_OBJ0", { RGB_4 (0,        0x630000, 0xD60000, 0xFF6352) }},
    { "GB_x0805_OBJ1", { RGB_4 (0x0084FF, 0xFFFF7B, 0xF5F5F5, 0x0000FF) }},
    /* Super Mario Land 2 */
    { "GB_x0905_BG",   { RGB_4 (0x5A5A5A, 0x9C8431, 0x63EFEF, 0xFFFFCE) }},
    { "GB_x0905_OBJ0", { RGB_4 (0,        0x944200, 0xFF7300, 0xF9F9F9) }},
    { "GB_x0905_OBJ1", { RGB_4 (0,        0x0000FF, 0x63A5FF, 0xF9F9F9) }},
    /* Dr. Mario */
    { "GB_x0B02_BG",   { RGB_4 (0,        0x0000FF, 0x63A5FF, 0xF9F9F9) }},
    { "GB_x0B02_OBJ0", { RGB_4 (0,        0x0000FF, 0x63A5FF, 0xF9F9F9) }},
    { "GB_x0B02_OBJ1", { RGB_4 (0,        0x943A3A, 0xFF8484, 0xF9F9F9) }},
    /* Link's Awakening */
    { "GB_x1105_BG",   { RGB_4 (0,        0x943A3A, 0xFF8484, 0xF9F9F9) }},
    { "GB_x1105_OBJ0", { RGB_4 (0x004A00, 0x318400, 0x00FF00, 0xF9F9F9) }},
    { "GB_x1105_OBJ1", { RGB_4 (0,        0x0000FF, 0x63A5FF, 0xF9F9F9) }},
    /* Wario Land II */
    { "GB_x1505_BG",   { RGB_4 (0,        0x42737B, 0xADAD84, 0xF9F9F9) }}, 
    { "GB_x1505_OBJ0", { RGB_4 (0,        0x843100, 0xFFAD63, 0xF9F9F9) }},
    { "GB_x1505_OBJ1", { RGB_4 (0,        0x0000FF, 0x63A5FF, 0xF9F9F9) }},
    /* Arcade Classic No. 2, MLB, Tetris Plus, Wario Blast 
       (also default if not found) */
    { "GB_x1C03_BG",   { RGB_4 (0,        0x0063C5, 0x7BFF31, 0xF9F9F9) }}, 
    { "GB_x1C03_OBJ0", { RGB_4 (0,        0x943A3A, 0xFF8484, 0xF9F9F9) }},
    { "GB_x1C03_OBJ1", { RGB_4 (0,        0x943A3A, 0xFF8484, 0xF9F9F9) }},
};

static const uint8_t gbcChecksumPalettes[0x100] = {
    [0x70] = 6,
    [0xD3] = 7
}; 

#endif