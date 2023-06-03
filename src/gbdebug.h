#ifndef GB_DEBUG_H
#define GB_DEBUG_H

#include <stdint.h>
#include "gb.h"

/* Debug function for viewing tiles */

static inline void debug_dump_tiles (const struct GB * gb, uint8_t * pixelData)
{
    const uint8_t * data = gb->vram;
    const uint8_t TILE_SIZE_BYTES = 16;

    const uint16_t NUM_TILES = 384;
    const uint16_t TILE_SIZE = 8;

    int t;
    for (t = 0; t < NUM_TILES; t++)
    {
        const uint16_t tileXoffset = (t % 16) * TILE_SIZE;
        const uint16_t tileYoffset = (t >> 4) * 1024;

        int y;
        for (y = 0; y < 8; y++)
        {
            const uint8_t row1 = *(data + (y * 2));
            const uint8_t row2 = *(data + (y * 2) + 1);
            const uint16_t yOffset = y * 128;

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

#endif