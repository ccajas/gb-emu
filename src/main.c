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
    GameBoy * gb = glfwGetWindowUserPointer (window);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        gb->direct.paused = !gb->direct.paused;
    }
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
    struct VArray rom;

    /* Used in drawing VRAM */
    uint8_t vram_raw[VRAM_SIZE * 2 * 3];
    uint8_t tilemap[256 * 64 * 3];

    /* Used for drawing the display */
    uint8_t framebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT * 3];
};

/* Concrete function definitions for the emulator frontend */

uint8_t rom_read (void * dataPtr, const uint_fast32_t addr)
{
    const struct gb_data * const gbData = dataPtr;
    return gbData->rom.data[addr];
};

void draw_line (void * dataPtr, const uint8_t * pixels, const uint8_t line)
{
    struct gb_data * const gbData = dataPtr;

    const uint32_t yOffset = line * DISPLAY_WIDTH * 3;
    uint8_t x;

	for (x = 0; x < DISPLAY_WIDTH; x++)
	{
		const uint8_t pixel = 3 - (*pixels);

        gbData->framebuffer[yOffset + (x * 3)] = pixel * 0x55;
        gbData->framebuffer[yOffset + (x * 3) + 1] = pixel * 0x55;
        gbData->framebuffer[yOffset + (x * 3) + 2] = pixel * 0x55;

        pixels++;
	}
}

/* Concrete debug functions for the frontend */

void peek_vram (void * dataPtr, const uint8_t * data)
{
    struct gb_data * const gbData = dataPtr;

    int i;
    int v = 0;
    for (i = 0; i < VRAM_SIZE; i++)
    {
        const uint8_t hi = (data[i] >> 4)  * 0x11;
        const uint8_t lo = (data[i] & 0xF) * 0x11;
        
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

int main (int argc, char * argv[])
{
    int draw = 1;
    int scale = 3;

    const int32_t totalFrames = 50;
    float totalSeconds = (float)totalFrames / 60.0;
    
    GLFWwindow* window;
    Scene scene = { .bgColor = { 173, 175, 186 }};

    GameBoy GB;

    /* Define structs for data and concrete functions */
    struct gb_data gbData =
	{
		.bootRom = NULL
	};

    if (draw)
    {
        glfwSetErrorCallback(error_callback);

        if (!glfwInit())
            exit(EXIT_FAILURE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        window = glfwCreateWindow(DISPLAY_WIDTH * scale, DISPLAY_HEIGHT * scale, "GB Emu", NULL, NULL);
        if (!window)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwSetWindowUserPointer (window, &GB);

        glfwSetKeyCallback(window, key_callback);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwMakeContextCurrent(window);

        graphics_init (&scene);
    }

    /* Load file from command line */
    char * defaultROM = NULL;
    if (argc > 1) defaultROM = argv[1];

    LOG_("GB: Loading file \"%s\"...\n", defaultROM);

    uint32_t fileSize = file_size(defaultROM);
    uint8_t * filebuf = (uint8_t*) read_file (defaultROM);

    vc_init (&gbData.rom, 1);
    vc_push_array (&gbData.rom, filebuf, fileSize, 0);

    if (vc_size(&gbData.rom) == 0)
    {
        LOG_("GB: Failed to load file! .\n");
        return 1;
    }

    /* Load ROM */
    gb_init (&GB, &gbData, 
        &(struct gb_func)  { rom_read, draw_line },
        &(struct gb_debug) { peek_vram, update_tiles }
    );

    /* Start clock */
    clock_t t;
    t = clock();
    uint32_t frames = 0;

    if (draw)
    {
        while (!glfwWindowShouldClose(window))
        { 
            if (frames < -1 && !GB.direct.paused)//totalFrames)
            {
                gb_frame (&GB);
                frames++;
                printf("Frames: %d\n", frames);
            }

            draw_begin (window, &scene);
            draw_screen_quad (window, &scene, gbData.framebuffer, scale);

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
        }

        gb_shutdown (&GB);
        t = clock() - t;   
        totalSeconds = (float)frames / 60.0; 

        if (vc_size(&gbData.rom) > 0)
            vc_free(&gbData.rom);

        double timeTaken = ((double)t)/CLOCKS_PER_SEC; /* Elapsed time */
        LOG_("The program ran %f seconds for %d frames.\nGB performance is %.2f times as fast.\n", timeTaken, totalFrames, totalSeconds / timeTaken);
        LOG_("For each second, there is on average %.2f milliseconds free for overhead.", 1000 - (1.0f / (totalSeconds / timeTaken) * 1000));   
    }

    if (draw)
    {
        gb_shutdown (&GB);
        t = clock() - t;
        totalSeconds = (float)frames / 60.0;

        if (vc_size(&gbData.rom) > 0)
            vc_free(&gbData.rom);

        double timeTaken = ((double)t)/CLOCKS_PER_SEC; /* Elapsed time */
        LOG_("The program ran %f seconds for %d frames.\nGB performance is %.2f times as fast.\n", timeTaken, frames, totalSeconds / timeTaken);
        LOG_("For each second, there is on average %.2f milliseconds free for overhead.", 1000 - (1.0f / (totalSeconds / timeTaken) * 1000));  

        glfwDestroyWindow(window);
        glfwTerminate();
        exit(EXIT_SUCCESS);
    }

    return 0;
}