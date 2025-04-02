#ifndef IO_H
#define IO_H

/* Related IO registers */

static const enum
{
    Joypad      = 0x0,
    SerialData  = 0x01,
    SerialCtrl  = 0x02,
    Divider     = 0x04,
    TimA        = 0x05,
    TMA         = 0x06,
    TimerCtrl   = 0x07,

    /* Audio */
    Ch1Sweep    = 0x10,
    Ch1Length   = 0x11,
    Ch1Vol      = 0x12,
    Ch1Period   = 0x13,
    Ch1Ctrl     = 0x14,
    Ch2Length   = 0x16,
    Ch2Vol      = 0x17,
    Ch2Period   = 0x18,
    Ch2Ctrl     = 0x19,
    Ch3DAC      = 0x1A,
    Ch3Length   = 0x1B,
    Ch3Vol      = 0x1C,
    Ch3Period   = 0x1D,
    Ch3Ctrl     = 0x1E,
    Ch4Length   = 0x20,
    Ch4Vol      = 0x21,
    Ch4Freq     = 0x22,
    Ch4Ctrl     = 0x23,

    MasterVol   = 0x24,
    AudioPan    = 0x25,
    AudioCtrl   = 0x26,

    /* Video/display */
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

    BootROM     = 0x50,
    IntrFlags   = 0x0F,
    IntrEnabled = 0xFF  
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
    IR_VBlank = 4,
    IR_OAM    = 5,
    IR_LYC    = 6,
}
LCD_STAT_Flags;

/* LCD Control flags by bit */

static const enum
{
    BG_Win_Enable = 0,
    OBJ_Enable,
    OBJ_Size,
    BG_Area,
    BG_Win_Data,
    Window_Enable,
    Window_Area,
    LCD_Enable
}
LCD_Control_Flags;

#define IO_STAT_CLEAR   (gb->io[LCDStatus] & 0xFC)
#define IO_STAT_MODE    (gb->io[LCDStatus] & 3)

#define LCDC_(bit)  (gb->io[LCDControl] & (1 << bit))
#define STAT_(bit)  (gb->io[LCDStatus]  & (1 << bit))

#endif