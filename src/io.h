#ifndef IO_H
#define IO_H

/* Related IO registers */

static const enum
{
    Joypad       = 0x0,
    SerialData   = 0x01,
    SerialCtrl   = 0x02,
    Divider      = 0x04,

    LCDControl	= 0x40,
    LCDStatus	= 0x41,
    ScrollY	    = 0x42,
    ScrollX	    = 0x43,
    LY	        = 0x44,
    LYC	        = 0x45,
    DMA	        = 0x46,
    BGPalette	= 0x47,
    OBJPalette0	= 0x48,
    OBJPalette1	= 0x49,
    WindowY	    = 0x4A,
    WindowX	    = 0x4B,

    BootROM      = 0x50,
    IntrFlags    = 0x0F,
    IntrEnabled  = 0xFF
}
registers;

/* Interrupt request/enable flags */

static const enum
{
    IF_VBlank   = 0x1,
    IF_LCD_STAT = 0x2,
    IF_Timer    = 0x4,
    IF_Serial   = 0x8,
    IF_Joypad   = 0x10,
    IF_Any      = 0x1F
}
interruptFlags;

/* LCD Status flags by bit */

static const enum
{
    LYC_LY    = 2,
    IR_HBlank = 3,
    IR_VBLank = 4,
    IR_OAM    = 5,
    IR_LYC    = 6,
}
LCD_STATFlags;

#endif