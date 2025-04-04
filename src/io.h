#ifndef IO_H
#define IO_H

/* Related IO registers */

__attribute__((unused))
static enum
{
    Joypad      = 0x0,
    SerialData  = 0x01,
    SerialCtrl  = 0x02,
    Divider     = 0x04,
    TimA        = 0x05,
    TMA         = 0x06,
    TimerCtrl   = 0x07,

    /* Audio */
    Ch1_Sweep   = 0x10,
    Ch1_Length  = 0x11,
    Ch1_Vol     = 0x12,
    Ch1_Period  = 0x13,
    Ch1_Ctrl    = 0x14,
    Ch2_Length  = 0x16,
    Ch2_Vol     = 0x17,
    Ch2_Period  = 0x18,
    Ch2_Ctrl    = 0x19,
    Ch3_DAC     = 0x1A,
    Ch3_Length  = 0x1B,
    Ch3_Vol     = 0x1C,
    Ch3_Period  = 0x1D,
    Ch3_Ctrl    = 0x1E,
    Ch4_Length  = 0x20,
    Ch4_Vol     = 0x21,
    Ch4_Freq    = 0x22,
    Ch4_Ctrl    = 0x23,

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

__attribute__((unused))
static enum
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

__attribute__((unused))
static enum
{
    LYC_LY    = 2,
    IR_HBlank = 3,
    IR_VBlank = 4,
    IR_OAM    = 5,
    IR_LYC    = 6,
}
LCD_STAT_Flags;

/* LCD Control flags by bit */

__attribute__((unused))
static enum
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

#define IO_STAT_MODE    gb->io[LCDStatus].stat_mode

#define LCDC_(bit)  (gb->io[LCDControl].r & (1 << bit))

#endif