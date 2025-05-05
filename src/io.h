#ifndef IO_H_
#define IO_H_

/* Assigned bit values for certain registers */
union regValues 
{
    struct /* Timer Control */
    {
        uint8_t TAC_clock  : 2;
        uint8_t TAC_Enable : 1;
        uint8_t b3_7       : 5; /* Unused */
    };
#ifdef ENABLE_AUDIO
    struct /* Audio register NR10 */
    {
        uint8_t SweepStep : 3;
        uint8_t SweepDir  : 1;
        uint8_t SweepPace : 3;
        uint8_t Sweep_hb  : 1; /* Unused */
    };
    struct /* Audio register NRx1 */
    {
        uint8_t Length  : 6;
        uint8_t Duty    : 2;
    };
    struct /* Audio register NRx2 */
    {
        uint8_t EnvPace : 3;
        uint8_t EnvDir  : 1;
        uint8_t Volume  : 4;
    };
    struct /* Audio register NRx4 */
    {
        uint8_t PeriodH    : 3;
        uint8_t b3_5       : 3; /* Unused */
        uint8_t Len_Enable : 1;
        uint8_t Trigger    : 1;
    };
    struct /* Audio Control */
    {
        uint8_t Ch1_on    : 1;
        uint8_t Ch2_on    : 1;
        uint8_t Ch3_on    : 1;
        uint8_t Ch4_on    : 1;
        uint8_t b4_6      : 3; /* Unused */
        uint8_t Master_on : 1;
    };
#endif
    struct /* LCD Control */
    {
        uint8_t BG_Win_Enable : 1;
        uint8_t OBJ_Enable    : 1;
        uint8_t OBJ_Size      : 1;
        uint8_t BG_Area       : 1;
        uint8_t BG_Win_Data   : 1;
        uint8_t Window_Enable : 1;
        uint8_t Window_Area   : 1;
        uint8_t LCD_Enable    : 1;
    };
    struct /* LCD Status */
    {
        uint8_t stat_mode   : 2;
        uint8_t stat_LYC_LY : 1;
        uint8_t stat_HBlank : 1;
        uint8_t stat_VBlank : 1;
        uint8_t stat_OAM    : 1;
        uint8_t stat_LYC    : 1;
        uint8_t stat_hb     : 1; /* Unused */
    };
    uint8_t r;
};

static const uint8_t apu_bitmasks[] = {
    0x80, 0x3F, 0,    0xFF, 0xBF, /* NR10 ... */
    0xFF, 0x3F, 0,    0xFF, 0xBF, /* NR20 ... */
    0x7F, 0xFF, 0x9F, 0xFF, 0xBF, /* NR30 ... */
    0xFF, 0xFF, 0,    0,    0xBF, /* NR40 ... */
    0,    0,    0x70,             /* NR50 ... */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* Wave RAM */
};

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
    NR10        = 0x10,
    NR11        = 0x11,
    NR12        = 0x12,
    NR13        = 0x13,
    NR14        = 0x14,
    NR21        = 0x16,
    NR22        = 0x17,
    NR23        = 0x18,
    NR24        = 0x19,
    NR30        = 0x1A,
    NR31        = 0x1B,
    NR32        = 0x1C,
    NR33        = 0x1D,
    NR34        = 0x1E,
    NR41        = 0x20,
    NR42        = 0x21,
    NR43        = 0x22,
    NR44        = 0x23,

    MasterVol   = 0x24,
    AudioPan    = 0x25,
    AudioCtrl   = 0x26,
    Wave        = 0x30,

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

/* Used for comparing and setting PPU mode timings */

__attribute__((unused))
static enum
{
    Stat_HBlank = 0,
    Stat_VBlank,
    Stat_OAM_Search,
    Stat_Transfer
}
modes;

__attribute__((unused))
static enum
{
    TICKS_OAM_READ = 80,
    TICKS_TRANSFER = 252,
    TICKS_HBLANK   = 456,
    TICKS_VBLANK   = 456
}
modeTicks;

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

#define CHANNEL_ON(n) \
    gb->audioCh[n].enabled = 1;\
    gb->io[AudioCtrl].r |= (1 << n);\

#define CHANNEL_OFF(n) \
    gb->audioCh[n].enabled = 0;\
    gb->io[AudioCtrl].r &= ~(1 << n);\

#define IO_STAT_MODE    gb->io[LCDStatus].stat_mode

#endif