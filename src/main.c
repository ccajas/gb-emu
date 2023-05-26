
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "utils/fileread.h"
#include "utils/v_array.h"
#include "api/glfw/utils/linmath.h"
#include "api/glfw/graphics.h"

#include "gb.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
 
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error (%d): %s\n", error, description);
}
 
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    /* make sure the viewport matches the new window dimensions; note that width and 
       height will be significantly larger than specified on retina displays. */
    glViewport(0, 0, width, height);
}

/* Container for relevant emulation data */

struct gb_data
{
    uint8_t * bootRom;
    uint8_t * rom;

    uint8_t vram_raw[VRAM_SIZE * 2 * 3]; /* 2 pixels per VRAM byte, 3 channels/pixel */
    uint8_t tilemap[256 * 64 * 3];
};

/* Concrete function definitions for the emulator frontend */

uint8_t rom_read (void * dataPtr, const uint_fast32_t addr)
{
    const struct gb_data * const gbData = dataPtr;
    return gbData->rom[addr];
};

/* Concrete debug functions for the frontend */

void peek_vram (void * dataPtr, const uint8_t * data)
{
    struct gb_data * const gbData = dataPtr;

    int i;
    int v = 0;
    for (i = 0; i < VRAM_SIZE; i++)
    {
        uint8_t hi = data[i] >> 4;
        uint8_t lo = data[i] & 0xF;

        hi *= 0x11;
        lo *= 0x11;
        
        v+= 2;
        gbData->vram_raw[v++] = hi;
        v+= 2;
        gbData->vram_raw[v++] = lo;
    }
}

void update_tiles (void * dataPtr, const uint8_t * data)
{
    struct gb_data * const gbData = dataPtr;

    const uint8_t tileSize = 16;

    int t;
    for (t = 0; t <= 255; t++)
    {
        uint16_t tileXoffset = (t % 16) * 8;
        uint16_t tileYoffset = (t >> 4) * 1024;

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
                const uint16_t pixelData = (tileYoffset + yOffset + tileXoffset + x) * 3;

                gbData->tilemap[pixelData] = colorID * 0x55;
                gbData->tilemap[pixelData + 1] = colorID * 0x55;
                gbData->tilemap[pixelData + 2] = colorID * 0x55;
            }
        }
        data += tileSize;
    }
}

/*
void update_tile (GameBoy * const gb, const uint8_t addr)
{
    // Get the "base address" for this tile row
	addr &= 0x1FFE;

	// Work out which tile and row was updated
	var tile = (addr >> 4) & 511;
	var y = (addr >> 1) & 7;



	var sx;
	for(var x = 0; x < 8; x++)
	{
	    // Find bit index for this pixel
	    sx = 1 << (7-x);

	    // Update tile set
	    GPU._tileset[tile][y][x] =
	        ((GPU._vram[addr] & sx)   ? 1 : 0) +
	        ((GPU._vram[addr+1] & sx) ? 2 : 0);
	}
}*/

int main (int argc, char * argv[])
{
    int draw = 1;
    GLFWwindow* window;
    Scene scene = { .bgColor = { 173, 175, 186 }};

    if (draw)
    {
        glfwSetErrorCallback(error_callback);

        if (!glfwInit())
            exit(EXIT_FAILURE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        window = glfwCreateWindow(512, 512, "GB Emu", NULL, NULL);
        if (!window)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwSetKeyCallback(window, key_callback);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwMakeContextCurrent(window);

        graphics_init (&scene);
    }

    GameBoy GB;

    /* Define structs for data and concrete functions */
    struct gb_data gbData =
	{
		.rom = NULL,
		.bootRom = NULL
	};

    /* Set data to default values */
    memset(gbData.vram_raw, 0, VRAM_SIZE * 6);
    memset(gbData.tilemap,  0, VRAM_SIZE * 6);

    struct gb_func gb_func =
    {
        .gb_rom_read = rom_read
    };

    struct gb_debug gb_debug =
    {
        .peek_vram = peek_vram,
        .update_tiles = update_tiles
    };

    /* Load file from command line */
    char * defaultROM = NULL;
    if (argc > 1) defaultROM = argv[1];

    LOG_("GB: Loading file \"%s\"...\n", defaultROM);
    gbData.rom = (uint8_t*) read_file (defaultROM);

    if (gbData.rom == NULL)
    {
        LOG_("GB: Failed to load file! .\n");
        return 1;
    }

    const int32_t totalFrames = -1;
    float totalSeconds = (float)totalFrames / 60.0;
    
    uint8_t gbFinished = 0;

    /* Load ROM */
    gb_init (&GB, &gbData, &gb_func, &gb_debug);

    /* Start clock */
    clock_t t;
    t = clock();
    uint32_t frames = 0;

    if (draw)
    {
        while (!glfwWindowShouldClose(window))
        { 
            draw_scene (window, &scene, gbData.tilemap);

            if (frames < totalFrames)
            {
                gb_frame (&GB);
                frames++;
                /*LOG_("Ran frame %d\n", frames);*/
            }
            else
            {
                if (!gbFinished)
                {
                    gb_shutdown (&GB);
                    t = clock() - t;
                    gbFinished = 1;         

                    double timeTaken = ((double)t)/CLOCKS_PER_SEC; /* Elapsed time */
                    LOG_("The program took %f seconds to execute %d frames.\nGB performance is %.2f times as fast.\n", timeTaken, totalFrames, totalSeconds / timeTaken);
                    LOG_("For each second, there is on average %.2f milliseconds free for overhead.", 1000 - (1.0f / (totalSeconds / timeTaken) * 1000));        
                }
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    else
    {
        while (frames < totalFrames)
        {
            gb_frame (&GB);
            frames++;
            LOG_("Ran frame %d\n", frames);
        }

        gb_shutdown (&GB);
        t = clock() - t;
        gbFinished = 1;         

        double timeTaken = ((double)t)/CLOCKS_PER_SEC; /* Elapsed time */
        LOG_("The program took %f seconds to execute %d frames.\nGB performance is %.2f times as fast.\n", timeTaken, totalFrames, totalSeconds / timeTaken);
        LOG_("For each second, there is on average %.2f milliseconds free for overhead.", 1000 - (1.0f / (totalSeconds / timeTaken) * 1000));   
    }

    t = clock() - t;
    totalSeconds = (float)frames / 60.0;

    free(gbData.rom);

    double timeTaken = ((double)t)/CLOCKS_PER_SEC; /* Elapsed time */
    LOG_("The program took %f seconds to execute %d frames.\nGB performance is %.2f times as fast.\n", timeTaken, frames, totalSeconds / timeTaken);
    LOG_("For each second, there is on average %.2f milliseconds free for overhead.", 1000 - (1.0f / (totalSeconds / timeTaken) * 1000));  

    if (draw)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        exit(EXIT_SUCCESS);
    }

    return 0;
}