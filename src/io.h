#ifndef IO_H
#define IO_H

/* Related IO registers */
static const enum
{
    IO_LCDControl	= 0x40,
    IO_LCDStatus	= 0x41,
    IO_ScrollY	    = 0x42,
    IO_ScrollX	    = 0x43,
    IO_LineY	    = 0x44,
    IO_LineYC	    = 0x45,
    IO_DMA	        = 0x46,
    IO_BGPalette	= 0x47,
    IO_OBJPalette0	= 0x48,
    IO_OBJPalette1	= 0x49,
    IO_WindowY	    = 0x4A,
    IO_WindowX	    = 0x4B,
    IO_IntrFlag     = 0x0F,
    IO_IntrEnabled  = 0xFF
}
registers;

#endif