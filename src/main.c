
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "utils/fileread.h"
#include "utils/v_array.h"
#include "api/glfw/utils/linmath.h"
#include "api/glfw/triangle_test.h"

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
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

/* Container for relevant emulation data */

struct gb_data
{
    uint8_t * bootRom;
    uint8_t * rom;
};

/* Concrete function definitions for the emulator frontend */

uint8_t rom_read (void * dataPtr, const uint16_t addr)
{
    const struct gb_data * const gbData = dataPtr;
    return gbData->rom[addr];
};

/* Concrete debug functions for the frontend */

void update_tiles (GameBoy * gb)
{

}

int main (int argc, char * argv[])
{
    GLFWwindow* window;
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(512, 512, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    gladLoadGL();
    glfwSwapInterval(1);

    GLuint program = scene_compile_shaders();
    scene_setup_buffers (program);

    GameBoy GB;

    /* Define structs for data and concrete functions */
    struct gb_data gbData =
	{
		.rom = NULL,
		.bootRom = NULL
	};

    struct gb_func gb_func =
    {
        .gb_rom_read = rom_read
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

    const uint32_t totalFrames = 150;
    const float totalSeconds = (float)totalFrames / 60.0;
    
    uint8_t gbFinished = 0;

    /* Load ROM */
    gb_init (&GB, &gbData, &gb_func, NULL);

    /* Start clock */
    clock_t t;
    t = clock();
    uint32_t frames = 0;

    while (!glfwWindowShouldClose(window))
    { 
        scene_begin (window);
        scene_draw_triangle (window, program);

        if (frames < totalFrames)
        {
            gb_frame (&GB);
            frames++;
            LOG_("Ran frame %d\n", frames);
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

    free(gbData.rom);

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);

    return 0;
}