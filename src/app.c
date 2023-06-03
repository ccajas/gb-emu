#include <string.h>
#include <time.h>
#include "app.h"

/* GLFW callback functions */

#ifdef GB_APP_DRAW

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error (%d): %s\n", error, description);
}
 
static void key_callback (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //struct App * app = glfwGetWindowUserPointer (window);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose (window, GLFW_TRUE);
}

void framebuffer_size_callback (GLFWwindow* window, int width, int height)
{
    /* make sure the viewport matches the new window dimensions; note that width and 
       height will be significantly larger than specified on retina displays. */
    glViewport(0, 0, width, height);
}

#endif

void app_config (struct App * app, uint8_t const argc, char * const argv[])
{
    if (argc >= 2) {
        char * fileName = argv[1];
        strcpy (app->defaultFile, fileName);
    }

    app->draw = 1;
    app->scale = 3;
    app->paused = 0;
}

void app_init (struct App * app)
{    
    /* Define structs for data and concrete functions */
    app->gbData = (struct gb_data) 
    {
        .rom = NULL,// mbc_load_rom (app->defaultFile),
        .bootRom = NULL,
        .tileMap = {
            .width = 128,
            .height = 192,
            .data = calloc (128 * 192 * 3, sizeof(uint8_t))
        },
        .frameBuffer = {
            .width = DISPLAY_WIDTH,
            .height = DISPLAY_HEIGHT,
            .data = calloc (DISPLAY_WIDTH * DISPLAY_HEIGHT * 3, sizeof(uint8_t))
        }
    };

    /* Copy ROM to cart */
    app->gb.cart.romData = app_load(app->defaultFile);
    app->gb.extData.ptr = &app->gbData;
    gb_init (&app->gb);

    /* Objects for drawing */
    GLFWwindow * window;
    Scene newDisplay = { .bgColor = { 173, 175, 186 }};
    app->display = newDisplay;

    /* Assign functions to be used by emulator */
    app->gb.draw_line = app_draw_line;

    /* Select image to display */
    app->image = &app->gbData.tileMap;

    if (app->draw)
    {
        glfwSetErrorCallback(error_callback);
    
        if (!glfwInit())
            exit(EXIT_FAILURE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

        window = glfwCreateWindow(
            app->gbData.frameBuffer.width * app->scale,
            app->gbData.frameBuffer.height * app->scale,
            "GB Emu", NULL, NULL);
        
        if (!window)
        {
            glfwTerminate();
            exit (EXIT_FAILURE);
        }

        glfwSetWindowUserPointer (window, &app);
        glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);

        glfwSetKeyCallback(window, key_callback);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwMakeContextCurrent(window);

        graphics_init (&app->display);
        app->window = window;
    }
}

uint8_t * app_load (const char * fileName)
{ 
    /* Load file from command line */
    FILE * f = fopen (fileName, "rb");

    fseek (f, 0, SEEK_END);
    uint32_t size = ftell(f);
    fseek (f, 0, SEEK_SET);

    if (!f)
    {
        fclose (f);
        LOG_("Failed to load file \"%s\"\n", fileName);
        return NULL;
    }

    uint8_t * rom = calloc(size, sizeof (uint8_t));
    fread (rom, size, 1, f);
    fclose (f);
    return rom;
}

void app_draw_line (void * dataPtr, const uint8_t * pixels, const uint8_t line)
{
    struct gb_data * const gbData = dataPtr;

    const uint32_t yOffset = line * DISPLAY_WIDTH * 3;
    uint8_t x;

	for (x = 0; x < DISPLAY_WIDTH; x++)
	{
		uint8_t pixel = 3 - (*pixels);
        pixel = (pixel * 0x55 == 0xFF) ? 0xEE : pixel * 0x55;

        gbData->frameBuffer.data[yOffset + (x * 3)] = pixel;
        gbData->frameBuffer.data[yOffset + (x * 3) + 1] = pixel;
        gbData->frameBuffer.data[yOffset + (x * 3) + 2] = pixel;

        pixels++;
	}
}

void app_draw (struct App * app)
{
    /* Select image to display */
    app->image = &app->gbData.tileMap;
    draw_begin (app->window, &app->display);
    draw_quad (app->window, &app->display, &app->gbData.frameBuffer, 0, 0, 2);

    if (gb_rom_loaded(&app->gb))
        draw_quad (app->window, &app->display, app->image, 352, 0, 1);

    glfwSwapBuffers (app->window);
    glfwPollEvents();
}

void app_run (struct App * app)
{
    /* Start clock */
    //clock_t time = clock();
    uint32_t frames = 0;

    const int32_t totalFrames = 600;
    //float totalSeconds = (float) totalFrames / 60.0;

    if (app->draw)
    {
        while (!glfwWindowShouldClose (app->window))
        {
            glfwMakeContextCurrent (app->window);

            if (frames < -1 && !app->paused)
            {
                if (gb_rom_loaded (&app->gb))
                    gb_frame (&app->gb);

                debug_dump_tiles (&app->gb, app->gbData.tileMap.data);
                printf("\033[A\33[2KT\rFrames: %d\n", ++frames);
            }
            app_draw (app);
        }
    }
    else
    {
        while (frames < totalFrames)
        {
            gb_frame (&app->gb);
            frames++;
        }
    }

    free (app->gb.cart.romData);

    if (app->draw)
    {
        glfwDestroyWindow (app->window);
        glfwTerminate();
        exit (EXIT_SUCCESS);
    }
}

/*    double timeTaken = ((double) time) / CLOCKS_PER_SEC; 
Elapsed time 
    LOG_("The program ran %f seconds for %d frames.\nGB performance is %.2f times as fast.\n",
        timeTaken, frames, totalSeconds / timeTaken);
    LOG_("For each second, there is on average %.2f milliseconds free for overhead.",
        1000 - (1.0f / (totalSeconds / timeTaken) * 1000));  */