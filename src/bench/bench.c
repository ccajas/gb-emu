#ifndef ENABLE_LCD
    #define ENABLE_LCD 1
#endif

#define ENABLE_SOUND 0

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../gb.h"

const uint_fast32_t frames_per_run = 32 * 1024;

/* Container for GB emulation data */
struct gb_data
{  
    uint8_t palette;
    uint8_t frameBuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT * 3];
}
gbData;

uint8_t app_cart_rom_read (void * dataPtr, const uint32_t addr)
{
    struct Cartridge * const cart = dataPtr;

    return cart->romData[addr];
}

uint8_t * app_load (struct GB * gb, const char * fileName)
{ 
    /* Load file from command line */
    LOG_("Attempting to open \"%s\"...\n", fileName);
    FILE * f = fopen (fileName, "rb");

    uint8_t * rom = NULL;
    if (!f) {
        LOG_("Failed to load file \"%s\"\n", fileName);
        return NULL;
    }
    else {
        LOG_("Attempting to seek file \"%s\"\n", fileName);
        fseek (f, 0, SEEK_END);
        uint32_t size = ftell(f);
        fseek (f, 0, SEEK_SET);

        rom = malloc(size + 1);
        if (!fread (rom, size, 1, f))
            return NULL;
        fclose (f);

        /* End with null character */
        rom[size] = 0;
    }
    uint8_t * boot = NULL;

#ifdef  USE_BOOT_ROM
    /* Load boot file if present */
    FILE * fb = fopen ("_test/mgb_boot.bin", "rb");
    
    boot = calloc(BOOT_ROM_SIZE, sizeof (uint8_t));
    if (!fread (boot, BOOT_ROM_SIZE, 1, fb))
        boot = NULL;
    else
        fclose (fb);
#endif
    /* Copy ROM to cart */
    LOG_("Loading \"%s\"\n", fileName);
    gb->cart.romData = rom;
    if (gb->cart.romData)
        gb_init (gb, boot);

    return rom;
}

#define RGB_(X)  { (X >> 16), ((X >> 8) & 255), (X & 255) }
#define RGB_4(P1, P2, P3, P4)  RGB_(P1), RGB_(P2), RGB_(P3), RGB_(P4)

typedef struct 
{
    char name[32];
    uint8_t colors[4][3];
}
GB_Palette;

static const GB_Palette palettes[] =
{
    /* Default palette */
    { "GBC_x16_BG",    { RGB_4 (0x0B0D08, 0x383E36, 0x626957, 0x8E9677) }},
    { "GBC_x16_OBJ0",  { RGB_4 (0x0B0D08, 0x383E36, 0x626957, 0x8E9677) }},
    { "GBC_x16_OBJ1",  { RGB_4 (0x0B0D08, 0x383E36, 0x626957, 0x8E9677) }},

    { "GBC_x16_BG",    { RGB_4 (0,        0x525252, 0xA5A5A5, 0xF5F5F5) }},
    { "GBC_x16_OBJ0",  { RGB_4 (0,        0x525252, 0xA5A5A5, 0xF5F5F5) }},
    { "GBC_x16_OBJ1",  { RGB_4 (0,        0x525252, 0xA5A5A5, 0xF5F5F5) }}
};

static void app_draw_line (void * dataPtr, const uint8_t * pixels, const uint8_t line)
{
    struct gb_data * const data = dataPtr;
    
    const uint32_t yOffset = line * DISPLAY_WIDTH * 3;
    uint8_t * pixel = NULL;

    uint8_t x;
	for (x = 0; x < DISPLAY_WIDTH; x++)
	{
        /* Get color and palette to use it with */
        const uint8_t idx = (3 - *pixels) & 3;
        const uint8_t pal = ((*pixels++ >> 2) & 3) - 1;
        pixel = (uint8_t*) palettes[pal].colors[idx];
        
        memcpy (data->frameBuffer + yOffset + x * 3, pixel, 3);
	}

    return;
}

int main (int argc, char **argv)
{
	char * fileName = NULL;

	switch(argc)
	{
		case 2:
			fileName = argv[1];
			break;

		default:
			fprintf(stderr, "%s [ROM filename]\n", argv[0]);
			return 1;
	}

    #define RUN_TOTAL 5
    float fpsTotal = 0;
    float durationTotal = 0;

    unsigned int i;
	for(i = 0; i < RUN_TOTAL; i++)
	{
		/* Start benchmark. */
		struct GB gb;
        gb.extData.ptr = &gbData;

		clock_t start_time;
		uint_fast32_t frames = 0;

        /* Assign functions to be used by emulator */
        gb.draw_line = app_draw_line;
        gb.cart.rom_read = app_cart_rom_read;

        if (app_load(&gb, fileName) == NULL)
            return 1;

		printf("Run %u: ", i);
		start_time = clock();

		do {
			gb_frame(&gb);
		}
		while(++frames < frames_per_run);

		{
			double duration =
				(double)(clock() - start_time) / CLOCKS_PER_SEC;
			double fps = frames / duration;
			printf("Ran %ld frames, %f FPS, duration: %f\n",
                (long int)frames_per_run, fps, duration);

            fpsTotal += fps;
            durationTotal += duration;
		}

        free (gb.cart.romData);
        free (gb.cart.ramData);
	}

    const float fpsAvg      = fpsTotal      / (float)RUN_TOTAL;
    const float durationAvg = durationTotal / (float)RUN_TOTAL;

    printf("Average %f FPS, duration: %f\n", fpsAvg, durationAvg);

	return 0;
}