#ifndef GB_DEBUG_H
#define GB_DEBUG_H

#include <stdint.h>
#include "gb.h"

/* Constant: font8x8_basic
   Contains an 8x8 font map for unicode points U+0000 - U+007F (basic latin) */
static const unsigned char font8x8_basic[96][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0020 (space)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},   // U+0021 (!)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0022 (")
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},   // U+0023 (#)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00},   // U+0024 ($)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},   // U+0025 (%)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00},   // U+0026 (&)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0027 (')
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00},   // U+0028 (()
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00},   // U+0029 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},   // U+002A (*)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00},   // U+002B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+002C (,)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},   // U+002D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+002E (.)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},   // U+002F (/)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},   // U+0030 (0)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},   // U+0031 (1)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},   // U+0032 (2)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},   // U+0033 (3)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},   // U+0034 (4)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},   // U+0035 (5)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},   // U+0036 (6)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},   // U+0037 (7)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},   // U+0038 (8)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},   // U+0039 (9)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+003A (:)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+003B (;)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},   // U+003C (<)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00},   // U+003D (=)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00},   // U+003E (>)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},   // U+003F (?)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},   // U+0041 (A)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},   // U+0042 (B)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},   // U+0043 (C)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},   // U+0044 (D)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},   // U+0045 (E)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},   // U+0046 (F)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},   // U+0047 (G)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},   // U+0048 (H)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0049 (I)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},   // U+004A (J)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},   // U+004B (K)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},   // U+004C (L)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},   // U+004D (M)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},   // U+004E (N)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},   // U+004F (O)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},   // U+0050 (P)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},   // U+0051 (Q)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},   // U+0052 (R)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},   // U+0053 (S)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0054 (T)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},   // U+0055 (U)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0056 (V)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},   // U+0057 (W)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},   // U+0058 (X)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},   // U+0059 (Y)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},   // U+005A (Z)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00},   // U+005B ([)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},   // U+005C (\)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},   // U+005D (])
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00},   // U+005E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},   // U+005F (_)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0060 (`)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00},   // U+0061 (a)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00},   // U+0062 (b)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},   // U+0063 (c)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00},   // U+0064 (d)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00},   // U+0065 (e)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00},   // U+0066 (f)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0067 (g)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00},   // U+0068 (h)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0069 (i)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E},   // U+006A (j)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00},   // U+006B (k)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+006C (l)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00},   // U+006D (m)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00},   // U+006E (n)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},   // U+006F (o)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F},   // U+0070 (p)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78},   // U+0071 (q)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00},   // U+0072 (r)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00},   // U+0073 (s)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00},   // U+0074 (t)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00},   // U+0075 (u)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0076 (v)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00},   // U+0077 (w)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00},   // U+0078 (x)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0079 (y)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00},   // U+007A (z)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00},   // U+007B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},   // U+007C (|)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00},   // U+007D (})
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+007E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}    // U+007F
};

static const unsigned char font_bmspa[96][8] = {
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, //  
	{0x00,0x5f,0x00,0x00,0x00,0x00,0x00,0x00}, // !
	{0x00,0x03,0x00,0x03,0x00,0x00,0x00,0x00}, // "
	{0x0a,0x1f,0x0a,0x1f,0x0a,0x00,0x00,0x00}, // #
	{0x24,0x2a,0x2a,0x7f,0x2a,0x2a,0x12,0x00}, // $
	{0x00,0x47,0x25,0x17,0x08,0x74,0x52,0x71}, // %
	{0x00,0x36,0x49,0x49,0x49,0x41,0x41,0x38}, // &
	{0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00}, // '
	{0x00,0x3e,0x41,0x00,0x00,0x00,0x00,0x00}, // (
	{0x41,0x3e,0x00,0x00,0x00,0x00,0x00,0x00}, // )
	{0x04,0x15,0x0e,0x15,0x04,0x00,0x00,0x00}, // *
	{0x08,0x08,0x3e,0x08,0x08,0x00,0x00,0x00}, // +
	{0x00,0xc0,0x00,0x00,0x00,0x00,0x00,0x00}, // ,
	{0x08,0x08,0x08,0x08,0x08,0x00,0x00,0x00}, // -
	{0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // .
	{0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x00}, // /
	{0x00,0x3e,0x61,0x51,0x49,0x45,0x43,0x3e}, // 0
	{0x01,0x01,0x7e,0x00,0x00,0x00,0x00,0x00}, // 1
	{0x00,0x71,0x49,0x49,0x49,0x49,0x49,0x46}, // 2
	{0x41,0x49,0x49,0x49,0x49,0x49,0x36,0x00}, // 3
	{0x00,0x0f,0x10,0x10,0x10,0x10,0x10,0x7f}, // 4
	{0x00,0x4f,0x49,0x49,0x49,0x49,0x49,0x31}, // 5
	{0x00,0x3e,0x49,0x49,0x49,0x49,0x49,0x30}, // 6
	{0x01,0x01,0x01,0x01,0x01,0x01,0x7e,0x00}, // 7
	{0x00,0x36,0x49,0x49,0x49,0x49,0x49,0x36}, // 8
	{0x00,0x06,0x49,0x49,0x49,0x49,0x49,0x3e}, // 9
	{0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // :
	{0x40,0x34,0x00,0x00,0x00,0x00,0x00,0x00}, // ;
	{0x08,0x14,0x22,0x00,0x00,0x00,0x00,0x00}, // <
	{0x14,0x14,0x14,0x14,0x14,0x00,0x00,0x00}, // =
	{0x22,0x14,0x08,0x00,0x00,0x00,0x00,0x00}, // >
	{0x00,0x06,0x01,0x01,0x59,0x09,0x09,0x06}, // ?
	{0x00,0x3e,0x41,0x5d,0x55,0x5d,0x51,0x5e}, // @
	{0x00,0x7e,0x01,0x09,0x09,0x09,0x09,0x7e}, // A
	{0x00,0x7f,0x41,0x49,0x49,0x49,0x49,0x36}, // B
	{0x00,0x3e,0x41,0x41,0x41,0x41,0x41,0x22}, // C
	{0x00,0x7f,0x41,0x41,0x41,0x41,0x41,0x3e}, // D
	{0x00,0x3e,0x49,0x49,0x49,0x49,0x49,0x41}, // E
	{0x00,0x7e,0x09,0x09,0x09,0x09,0x09,0x01}, // F
	{0x00,0x3e,0x41,0x49,0x49,0x49,0x49,0x79}, // G
	{0x00,0x7f,0x08,0x08,0x08,0x08,0x08,0x7f}, // H
	{0x00,0x7f,0x00,0x00,0x00,0x00,0x00,0x00}, // I
	{0x00,0x38,0x40,0x40,0x41,0x41,0x41,0x3f}, // J
	{0x00,0x7f,0x08,0x08,0x08,0x0c,0x0a,0x71}, // K
	{0x00,0x3f,0x40,0x40,0x40,0x40,0x40,0x40}, // L
	{0x00,0x7e,0x01,0x01,0x7e,0x01,0x01,0x7e}, // M
	{0x00,0x7e,0x01,0x01,0x3e,0x40,0x40,0x3f}, // N
	{0x00,0x3e,0x41,0x41,0x41,0x41,0x41,0x3e}, // O
	{0x00,0x7e,0x09,0x09,0x09,0x09,0x09,0x06}, // P
	{0x00,0x3e,0x41,0x41,0x71,0x51,0x51,0x7e}, // Q
	{0x00,0x7e,0x01,0x31,0x49,0x49,0x49,0x46}, // R
	{0x00,0x46,0x49,0x49,0x49,0x49,0x49,0x31}, // S
	{0x01,0x01,0x01,0x7f,0x01,0x01,0x01,0x00}, // T
	{0x00,0x3f,0x40,0x40,0x40,0x40,0x40,0x3f}, // U
	{0x00,0x0f,0x10,0x20,0x40,0x20,0x10,0x0f}, // V
	{0x00,0x3f,0x40,0x40,0x3f,0x40,0x40,0x3f}, // W
	{0x00,0x63,0x14,0x08,0x08,0x08,0x14,0x63}, // X
	{0x00,0x07,0x08,0x08,0x78,0x08,0x08,0x07}, // Y
	{0x00,0x71,0x49,0x49,0x49,0x49,0x49,0x47}, // Z
	{0x00,0x7f,0x41,0x00,0x00,0x00,0x00,0x00}, // [
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // "\"
	{0x41,0x7f,0x00,0x00,0x00,0x00,0x00,0x00}, // ]
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // ^
	{0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00}, // _
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // `
	{0x00,0x7e,0x01,0x09,0x09,0x09,0x09,0x7e}, // A
	{0x00,0x7f,0x41,0x49,0x49,0x49,0x49,0x36}, // B
	{0x00,0x3e,0x41,0x41,0x41,0x41,0x41,0x22}, // C
	{0x00,0x7f,0x41,0x41,0x41,0x41,0x41,0x3e}, // D
	{0x00,0x3e,0x49,0x49,0x49,0x49,0x49,0x41}, // E
	{0x00,0x7e,0x09,0x09,0x09,0x09,0x09,0x01}, // F
	{0x00,0x3e,0x41,0x49,0x49,0x49,0x49,0x79}, // G
	{0x00,0x7f,0x08,0x08,0x08,0x08,0x08,0x7f}, // H
	{0x00,0x7f,0x00,0x00,0x00,0x00,0x00,0x00}, // I
	{0x00,0x38,0x40,0x40,0x41,0x41,0x41,0x3f}, // J
	{0x00,0x7f,0x08,0x08,0x08,0x0c,0x0a,0x71}, // K
	{0x00,0x3f,0x40,0x40,0x40,0x40,0x40,0x40}, // L
	{0x00,0x7e,0x01,0x01,0x7e,0x01,0x01,0x7e}, // M
	{0x00,0x7e,0x01,0x01,0x3e,0x40,0x40,0x3f}, // N
	{0x00,0x3e,0x41,0x41,0x41,0x41,0x41,0x3e}, // O
	{0x00,0x7e,0x09,0x09,0x09,0x09,0x09,0x06}, // P
	{0x00,0x3e,0x41,0x41,0x71,0x51,0x51,0x7e}, // Q
	{0x00,0x7e,0x01,0x31,0x49,0x49,0x49,0x46}, // R
	{0x00,0x46,0x49,0x49,0x49,0x49,0x49,0x31}, // S
	{0x01,0x01,0x01,0x7f,0x01,0x01,0x01,0x00}, // T
	{0x00,0x3f,0x40,0x40,0x40,0x40,0x40,0x3f}, // U
	{0x00,0x0f,0x10,0x20,0x40,0x20,0x10,0x0f}, // V
	{0x00,0x3f,0x40,0x40,0x3f,0x40,0x40,0x3f}, // W
	{0x00,0x63,0x14,0x08,0x08,0x08,0x14,0x63}, // X
	{0x00,0x07,0x08,0x08,0x78,0x08,0x08,0x07}, // Y
	{0x00,0x71,0x49,0x49,0x49,0x49,0x49,0x47}, // Z
	{0x08,0x36,0x41,0x00,0x00,0x00,0x00,0x00}, // {
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // |
	{0x41,0x36,0x08,0x00,0x00,0x00,0x00,0x00}, // }
	{0x02,0x01,0x01,0x02,0x02,0x01,0x00,0x00}, // ~
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

static const unsigned char font_zxpix[96][6] = {
	{0x00,0x00,0x00,0x00,0x00,0x00}, //  
	{0x2f,0x00,0x00,0x00,0x00,0x00}, // !
	{0x03,0x00,0x03,0x00,0x00,0x00}, // "
	{0x12,0x3f,0x12,0x12,0x3f,0x12}, // #
	{0x2e,0x2a,0x7f,0x2a,0x3a,0x00}, // $
	{0x23,0x13,0x08,0x04,0x32,0x31}, // %
	{0x10,0x2a,0x25,0x2a,0x10,0x20}, // &
	{0x02,0x01,0x00,0x00,0x00,0x00}, // '
	{0x1e,0x21,0x00,0x00,0x00,0x00}, // (
	{0x21,0x1e,0x00,0x00,0x00,0x00}, // )
	{0x08,0x2a,0x1c,0x2a,0x08,0x08}, // *
	{0x08,0x08,0x3e,0x08,0x08,0x08}, // +
	{0x80,0x60,0x00,0x00,0x00,0x00}, // ,
	{0x08,0x08,0x08,0x08,0x08,0x00}, // -
	{0x30,0x30,0x00,0x00,0x00,0x00}, // .
	{0x20,0x10,0x08,0x04,0x02,0x00}, // /
	{0x1e,0x31,0x29,0x25,0x23,0x1e}, // 0
	{0x22,0x21,0x3f,0x20,0x20,0x20}, // 1
	{0x32,0x29,0x29,0x29,0x29,0x26}, // 2
	{0x12,0x21,0x21,0x25,0x25,0x1a}, // 3
	{0x18,0x14,0x12,0x3f,0x10,0x10}, // 4
	{0x17,0x25,0x25,0x25,0x25,0x19}, // 5
	{0x1e,0x25,0x25,0x25,0x25,0x18}, // 6
	{0x01,0x01,0x31,0x09,0x05,0x03}, // 7
	{0x1a,0x25,0x25,0x25,0x25,0x1a}, // 8
	{0x06,0x29,0x29,0x29,0x29,0x1e}, // 9
	{0x24,0x00,0x00,0x00,0x00,0x00}, // :
	{0x80,0x64,0x00,0x00,0x00,0x00}, // ;
	{0x08,0x14,0x22,0x00,0x00,0x00}, // <
	{0x14,0x14,0x14,0x14,0x14,0x00}, // =
	{0x22,0x14,0x08,0x00,0x00,0x00}, // >
	{0x02,0x01,0x01,0x29,0x05,0x02}, // ?
	{0x1e,0x21,0x2d,0x2b,0x2d,0x0e}, // @
	{0x3e,0x09,0x09,0x09,0x09,0x3e}, // A
	{0x3f,0x25,0x25,0x25,0x25,0x1a}, // B
	{0x1e,0x21,0x21,0x21,0x21,0x12}, // C
	{0x3f,0x21,0x21,0x21,0x12,0x0c}, // D
	{0x3f,0x25,0x25,0x25,0x25,0x21}, // E
	{0x3f,0x05,0x05,0x05,0x05,0x01}, // F
	{0x1e,0x21,0x21,0x21,0x29,0x1a}, // G
	{0x3f,0x04,0x04,0x04,0x04,0x3f}, // H
	{0x21,0x21,0x3f,0x21,0x21,0x21}, // I
	{0x10,0x20,0x20,0x20,0x20,0x1f}, // J
	{0x3f,0x04,0x0c,0x0a,0x11,0x20}, // K
	{0x3f,0x20,0x20,0x20,0x20,0x20}, // L
	{0x3f,0x02,0x04,0x04,0x02,0x3f}, // M
	{0x3f,0x02,0x04,0x08,0x10,0x3f}, // N
	{0x1e,0x21,0x21,0x21,0x21,0x1e}, // O
	{0x3f,0x09,0x09,0x09,0x09,0x06}, // P
	{0x1e,0x21,0x29,0x31,0x21,0x1e}, // Q
	{0x3f,0x09,0x09,0x09,0x19,0x26}, // R
	{0x12,0x25,0x25,0x25,0x25,0x18}, // S
	{0x01,0x01,0x01,0x3f,0x01,0x01}, // T
	{0x1f,0x20,0x20,0x20,0x20,0x1f}, // U
	{0x0f,0x10,0x20,0x20,0x10,0x0f}, // V
	{0x1f,0x20,0x10,0x10,0x20,0x1f}, // W
	{0x21,0x12,0x0c,0x0c,0x12,0x21}, // X
	{0x01,0x02,0x3c,0x02,0x01,0x00}, // Y
	{0x21,0x31,0x29,0x25,0x23,0x21}, // Z
	{0x3f,0x21,0x00,0x00,0x00,0x00}, // [
	{0x02,0x04,0x08,0x10,0x20,0x00}, // "\"
	{0x21,0x3f,0x00,0x00,0x00,0x00}, // ]
	{0x04,0x02,0x3f,0x02,0x04,0x00}, // ^
	{0x40,0x40,0x40,0x40,0x40,0x40}, // _
	{0x01,0x02,0x00,0x00,0x00,0x00}, // `
	{0x10,0x2a,0x2a,0x2a,0x3c,0x00}, // a
	{0x3f,0x24,0x24,0x24,0x18,0x00}, // b
	{0x1c,0x22,0x22,0x22,0x00,0x00}, // c
	{0x18,0x24,0x24,0x24,0x3f,0x00}, // d
	{0x1c,0x2a,0x2a,0x2a,0x24,0x00}, // e
	{0x3e,0x05,0x01,0x00,0x00,0x00}, // f
	{0x18,0x28,0xa4,0xa4,0x7c,0x00}, // g
	{0x3f,0x04,0x04,0x0c,0x30,0x00}, // h
	{0x24,0x3d,0x20,0x00,0x00,0x00}, // i
	{0x20,0x40,0x40,0x3d,0x00,0x00}, // j
	{0x3f,0x0c,0x12,0x20,0x00,0x00}, // k
	{0x1f,0x20,0x20,0x00,0x00,0x00}, // l
	{0x3e,0x02,0x3c,0x02,0x3c,0x00}, // m
	{0x3e,0x02,0x02,0x02,0x3c,0x00}, // n
	{0x0c,0x14,0x22,0x32,0x0c,0x00}, // o
	{0xfc,0x24,0x24,0x24,0x18,0x00}, // p
	{0x18,0x24,0x24,0x24,0xfc,0x80}, // q
	{0x3c,0x02,0x02,0x02,0x00,0x00}, // r
	{0x24,0x2c,0x2a,0x2a,0x10,0x00}, // s
	{0x02,0x1f,0x22,0x20,0x00,0x00}, // t
	{0x1e,0x20,0x20,0x20,0x1e,0x00}, // u
	{0x06,0x18,0x20,0x18,0x06,0x00}, // v
	{0x1e,0x30,0x1c,0x30,0x0e,0x00}, // w
	{0x22,0x14,0x08,0x14,0x22,0x00}, // x
	{0x0c,0x10,0xa0,0xa0,0x7c,0x00}, // y
	{0x22,0x32,0x2a,0x26,0x22,0x22}, // z
	{0x0c,0x3f,0x21,0x00,0x00,0x00}, // {
	{0x3f,0x00,0x00,0x00,0x00,0x00}, // |
	{0x21,0x3f,0x0c,0x00,0x00,0x00}, // }
	{0x02,0x01,0x02,0x01,0x00,0x00}, // ~
	{0x00,0x00,0x00,0x00,0x00,0x00}
};

static const unsigned char font_tamaMini[96][5] = {
	{0x00,0x00,0x00,0x00,0x00}, //  
	{0x2f,0x00,0x00,0x00,0x00}, // !
	{0x03,0x00,0x03,0x00,0x00}, // "
	{0x14,0x3e,0x14,0x3e,0x14}, // #
	{0x2e,0x6a,0x2b,0x3a,0x00}, // $
	{0x26,0x12,0x08,0x24,0x32}, // %
	{0x1c,0x17,0x15,0x34,0x00}, // &
	{0x03,0x00,0x00,0x00,0x00}, // '
	{0x1e,0x21,0x00,0x00,0x00}, // (
	{0x21,0x1e,0x00,0x00,0x00}, // )
	{0x22,0x08,0x1c,0x08,0x22}, // *
	{0x08,0x1c,0x08,0x00,0x00}, // +
	{0x40,0x20,0x00,0x00,0x00}, // ,
	{0x08,0x08,0x00,0x00,0x00}, // -
	{0x20,0x00,0x00,0x00,0x00}, // .
	{0x20,0x10,0x08,0x04,0x02}, // /
	{0x3f,0x21,0x21,0x3f,0x00}, // 0
	{0x01,0x3f,0x00,0x00,0x00}, // 1
	{0x3d,0x25,0x25,0x27,0x00}, // 2
	{0x25,0x25,0x25,0x3f,0x00}, // 3
	{0x07,0x04,0x04,0x3f,0x00}, // 4
	{0x27,0x25,0x25,0x3d,0x00}, // 5
	{0x3f,0x25,0x25,0x3d,0x00}, // 6
	{0x01,0x39,0x05,0x03,0x00}, // 7
	{0x3f,0x25,0x25,0x3f,0x00}, // 8
	{0x27,0x25,0x25,0x3f,0x00}, // 9
	{0x28,0x00,0x00,0x00,0x00}, // :
	{0x40,0x28,0x00,0x00,0x00}, // ;
	{0x04,0x0a,0x11,0x00,0x00}, // <
	{0x14,0x14,0x00,0x00,0x00}, // =
	{0x11,0x0a,0x04,0x00,0x00}, // >
	{0x01,0x2d,0x05,0x07,0x00}, // ?
	{0x3f,0x21,0x3d,0x25,0x1f}, // @
	{0x3f,0x09,0x09,0x3f,0x00}, // A
	{0x3f,0x25,0x27,0x3c,0x00}, // B
	{0x3f,0x21,0x21,0x21,0x00}, // C
	{0x3f,0x21,0x21,0x1e,0x00}, // D
	{0x3f,0x25,0x25,0x25,0x00}, // E
	{0x3f,0x05,0x05,0x05,0x00}, // F
	{0x3f,0x21,0x25,0x3d,0x00}, // G
	{0x3f,0x04,0x04,0x3f,0x00}, // H
	{0x21,0x3f,0x21,0x00,0x00}, // I
	{0x38,0x20,0x21,0x3f,0x01}, // J
	{0x3f,0x04,0x04,0x3b,0x00}, // K
	{0x3f,0x20,0x20,0x20,0x00}, // L
	{0x3f,0x01,0x3f,0x01,0x3f}, // M
	{0x3f,0x02,0x04,0x3f,0x00}, // N
	{0x3f,0x21,0x21,0x3f,0x00}, // O
	{0x3f,0x09,0x09,0x0f,0x00}, // P
	{0x3f,0x21,0x31,0x3f,0x00}, // Q
	{0x3f,0x09,0x39,0x2f,0x00}, // R
	{0x27,0x25,0x25,0x3d,0x00}, // S
	{0x01,0x01,0x3f,0x01,0x01}, // T
	{0x3f,0x20,0x20,0x3f,0x00}, // U
	{0x0f,0x10,0x30,0x1f,0x00}, // V
	{0x3f,0x20,0x3f,0x20,0x3f}, // W
	{0x3b,0x04,0x04,0x3b,0x00}, // X
	{0x0f,0x08,0x38,0x0f,0x00}, // Y
	{0x31,0x29,0x25,0x23,0x00}, // Z
	{0x3f,0x21,0x00,0x00,0x00}, // [
	{0x20,0x10,0x08,0x04,0x02}, // "\"
	{0x21,0x3f,0x00,0x00,0x00}, // ]
	{0x02,0x01,0x01,0x02,0x00}, // ^
	{0x20,0x20,0x00,0x00,0x00}, // _
	{0x01,0x02,0x00,0x00,0x00}, // `
	{0x38,0x24,0x24,0x3c,0x00}, // a
	{0x3f,0x24,0x24,0x3c,0x00}, // b
	{0x3c,0x24,0x24,0x24,0x00}, // c
	{0x3c,0x24,0x24,0x3f,0x00}, // d
	{0x3c,0x2c,0x2c,0x2c,0x00}, // e
	{0x04,0x3f,0x05,0x00,0x00}, // f
	{0xbc,0xa4,0xa4,0xfc,0x00}, // g
	{0x3f,0x04,0x04,0x3c,0x00}, // h
	{0x3d,0x00,0x00,0x00,0x00}, // i
	{0x80,0xfd,0x00,0x00,0x00}, // j
	{0x3f,0x08,0x08,0x34,0x00}, // k
	{0x3f,0x00,0x00,0x00,0x00}, // l
	{0x3c,0x04,0x3c,0x04,0x3c}, // m
	{0x3c,0x04,0x04,0x3c,0x00}, // n
	{0x3c,0x24,0x24,0x3c,0x00}, // o
	{0xfc,0x24,0x24,0x3c,0x00}, // p
	{0x3c,0x24,0x24,0xfc,0x00}, // q
	{0x3c,0x08,0x04,0x00,0x00}, // r
	{0x2c,0x2c,0x2c,0x3c,0x00}, // s
	{0x04,0x3f,0x24,0x00,0x00}, // t
	{0x3c,0x20,0x20,0x3c,0x00}, // u
	{0x0c,0x10,0x30,0x1c,0x00}, // v
	{0x3c,0x20,0x3c,0x20,0x3c}, // w
	{0x34,0x08,0x08,0x34,0x00}, // x
	{0xbc,0xa0,0xa0,0xfc,0x00}, // y
	{0x24,0x34,0x2c,0x24,0x00}, // z
	{0x04,0x3f,0x21,0x00,0x00}, // {
	{0x3f,0x00,0x00,0x00,0x00}, // |
	{0x21,0x3f,0x04,0x00,0x00}, // }
	{0x01,0x02,0x02,0x01,0x00}, // ~
	{0x00,0x00,0x00,0x00,0x00}
};

static inline void render_text (
    const unsigned char textmap[][6],
    const char * text, const uint16_t width,
    const uint32_t color, const uint32_t bgColor,
    uint8_t * pixelData) 
{
    uint16_t x,y;
    uint16_t set;

    const uint16_t FONT_SPACING = 7;

    uint8_t t = 0;
    while (*(text + t) != '\0')
    {
        const uint8_t ord = ((uint8_t) *(text + t)) - 32;
        const unsigned char * bitmap = textmap[ord];

        const uint16_t tileXoffset = t * FONT_SPACING;
        t++;
        
        for (y = 0; y < 8; y++) 
        {
            const uint16_t yOffset = y * width;

            for (x = 0; x < FONT_SPACING; x++)
            {
                set = (x >= 6) ? 0 : bitmap[x] & 1 << y;
                const uint32_t idx = (yOffset + tileXoffset + x) * 3;

                pixelData[idx]     = (set ? color : bgColor) >> 16;
                pixelData[idx + 1] = (set ? color : bgColor) >> 8;
                pixelData[idx + 2] = (set ? color : bgColor);
            }
        }
    }
}

/* Debug function for viewing tiles */

static inline void debug_dump_tiles (
    const struct GB * gb,
    const uint16_t txWidth,
    uint8_t * pixelData)
{
    const uint8_t * data = gb->vram;
    const uint8_t TILE_SIZE_BYTES = 16;

    const uint16_t NUM_TILES = 384;
    const uint8_t  NUM_COLS = 16;
    const uint8_t  TILE_WIDTH = 8;
    const uint8_t  TILE_HEIGHT = 8;

    int t;
    for (t = 0; t < NUM_TILES; t++)
    {
        const uint16_t index = t;
        const uint16_t tileXoffset = (index % NUM_COLS) * TILE_WIDTH;
        const uint16_t tileYoffset = (index / NUM_COLS) * txWidth * TILE_HEIGHT;

        int y;
        for (y = 0; y < TILE_HEIGHT; y++)
        {
            const uint8_t row1 = *(data + (y * 2));
            const uint8_t row2 = *(data + (y * 2) + 1);
            const uint16_t yOffset = y * txWidth;

            int x;
            for (x = 0; x < 8; x++)
            {
                uint8_t col1 = row1 >> (7 - x);
                uint8_t col2 = row2 >> (7 - x);
                
                const uint8_t  colorID = 3 - ((col1 & 1) + ((col2 & 1) << 1));
                const uint32_t idx = (tileYoffset + yOffset + tileXoffset + x) * 3;

                pixelData[idx] = colorID * 0x55;
                pixelData[idx + 1] = colorID * 0x55;
                pixelData[idx + 2] = colorID * 0x55;
            }
        }
        data += TILE_SIZE_BYTES;
    }
}

static inline void debug_dump_OAM (
    const struct GB * gb,
    const uint16_t txWidth,
    uint8_t * pixelData)
{
    const uint8_t * data = gb->vram;
    const uint8_t * oam  = gb->oam;

    const uint16_t NUM_ITEMS = 40;
    const uint8_t  NUM_COLS = 10;
    const uint8_t  TILE_WIDTH = 8;
    const uint8_t  TILE_HEIGHT = 16;

    int t;
    for (t = 0; t < NUM_ITEMS; t++)
    {
        uint8_t index = oam[t * 4 + 2];  /* Tile index of sprite        */
        //index &= 0xFE;                   /* Adjust index for 8x16 size */
        const uint16_t tileXoffset = (t % NUM_COLS) * TILE_WIDTH;
        const uint16_t tileYoffset = (t / NUM_COLS) * txWidth * TILE_HEIGHT;
        const uint16_t offset = data[index];

        int y;
        for (y = 0; y < TILE_HEIGHT; y++)
        {
            const uint8_t row1 = *(data + offset + (y * 2));
            const uint8_t row2 = *(data + offset + (y * 2) + 1);
            const uint16_t yOffset = y * txWidth;

            int x;
            for (x = 0; x < 8; x++)
            {
                uint8_t col1 = row1 >> (7 - x);
                uint8_t col2 = row2 >> (7 - x);
                
                const uint8_t  colorID = 3 - ((col1 & 1) + ((col2 & 1) << 1));
                const uint32_t idx = (tileYoffset + yOffset + tileXoffset + x) * 3;

                pixelData[idx] = colorID * 0x55;
                pixelData[idx + 1] = colorID * 0x55;
                pixelData[idx + 2] = colorID * 0x55;
            }
        }
        //data += TILE_SIZE_BYTES;
    }
}


#endif