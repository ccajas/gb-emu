#include <string.h>
#include <time.h>
#include "app.h"
#include "palettes.h"
#ifdef USE_TIGR
    #include "app_tigr.h"
#endif

/* GLFW callback functions */

#define USE_GLFW
#ifdef USE_GLFW

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error (%d): %s\n", error, description);
}
 
void key_callback (GLFWwindow * window, int key, int scancode, int action, int mods)
{
    struct App * app = glfwGetWindowUserPointer (window);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose (window, GLFW_TRUE);

    /* Pause emulation */
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        app->paused = !app->paused;

    /* Reset game */
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        gb_boot_reset(&app->gb);
}

void framebuffer_size_callback (GLFWwindow * window, int width, int height)
{
    /* make sure the viewport matches the new window dimensions; note that width and 
       height will be significantly larger than specified on retina displays. */
    glViewport(0, 0, width, height);
}

void drop_callback(GLFWwindow * window, int count, const char** paths)
{
    struct App * app = glfwGetWindowUserPointer (window);

    LOG_("%s\n", paths[0]);
    strcpy(app->defaultFile, paths[0]);
    LOG_("%s\n", app->defaultFile);
}

#endif

void app_config (struct App * app, uint8_t const argc, char * const argv[])
{
    if (argc >= 2) {
        char * fileName = argv[1];
        strcpy (app->defaultFile, fileName);
    }
    else app->defaultFile[0] = '\0';

#if defined(USE_GLFW) || defined(USE_TIGR)
    app->draw = 1;
#else
    app->draw = 0;
#endif
    app->scale = 3;
    app->paused = 0;
}

void app_init (struct App * app)
{    
    /* Define structs for data and concrete functions */
    app->gb.cart.romData = NULL;
    app->gb.cart.ramData = NULL;

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
    /* Handle file loading */
    if (strcmp(app->defaultFile, " ") != 0)
    {
        /* Copy ROM to cart */
    }
    else {
        LOG_("No file selected.\n");
    }

    /* Objects for drawing */
    Scene newDisplay = { .bgColor = { 173, 175, 186 }};
    app->display = newDisplay;

    /* Assign functions to be used by emulator */
    app->gb.draw_line = app_draw_line;

    /* Select image to display */
    app->image = &app->gbData.tileMap;

    /* Handle file loading */
    if (strcmp(app->defaultFile, "\0") != 0 &&
        !(gb_rom_loaded(&app->gb)))
    {
        /* Copy ROM to cart */
        LOG_("%s\n", app->defaultFile);
        app->gb.cart.romData = app_load(app->defaultFile);
        if (app->gb.cart.romData)
        {
            app->gb.extData.ptr = &app->gbData;
            gb_init (&app->gb);
        }
        else app->defaultFile[0] = '\0';
    }

#ifdef USE_GLFW
    GLFWwindow * window;

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

        glfwSetWindowUserPointer (window, app);
        glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);

        glfwSetKeyCallback(window, key_callback);
        glfwSetDropCallback(window, drop_callback);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwMakeContextCurrent(window);

        graphics_init (&app->display);
        app->window = window;
    }
#endif
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
    if (!fread (rom, size, 1, f))
        return NULL;
        
    fclose (f);
    return rom;
}

#ifdef USE_GLFW
void app_draw_line (void * dataPtr, const uint8_t * pixels, const uint8_t line)
{
    struct gb_data * const data = dataPtr;

    const uint32_t yOffset = line * DISPLAY_WIDTH * 3;
    uint8_t x;

	for (x = 0; x < DISPLAY_WIDTH; x++)
	{
		uint8_t idx = 3 - (*pixels);
        const uint8_t * pixel = palettes[3].colors[idx];//(pixel == 3) ? 0xF5 : pixel * 0x55;

        data->frameBuffer.data[yOffset + (x * 3)]     = pixel[0];
        data->frameBuffer.data[yOffset + (x * 3) + 1] = pixel[1];
        data->frameBuffer.data[yOffset + (x * 3) + 2] = pixel[2];

        pixels++;
	}
}

void app_draw (struct App * app)
{
    /* Select image to display */
    app->image = &app->gbData.tileMap;
    const uint16_t w = app->gbData.frameBuffer.width  * app->scale;
    //const uint16_t h = app->gbData.frameBuffer.height * app->scale;

    set_shader (&app->display, &app->display.fbufferShader);
    draw_quad (app->window, &app->display, &app->gbData.frameBuffer, 0, 0, app->scale);

    set_shader (&app->display, &app->display.debugShader);
    if (gb_rom_loaded(&app->gb))
        draw_quad (app->window, &app->display, app->image, w - 128, 0, 1);
}
#endif

void app_run (struct App * app)
{
    /* Start clock */
    //clock_t time = clock();
    double totalTime = 0;
    uint32_t frames = 0;

    const int32_t totalFrames = 600;
    float totalSeconds = (float) totalFrames / 60.0;

    if (app->draw)
    {
#ifdef USE_GLFW
        while (!glfwWindowShouldClose (app->window))
        {
            glfwMakeContextCurrent (app->window);
            draw_begin (app->window, &app->display);

            if (frames < -1 && app->paused == 0)
            {
                gb_frame (&app->gb);
                frames++;
                debug_dump_tiles (&app->gb, app->gbData.tileMap.data);
                //printf("\033[A\33[2KT\rFrames: %d\n", frames);
            }
            app_draw (app);
            glfwSwapBuffers (app->window);
            glfwPollEvents();
        }
#endif
    }
    else
    {
        while (frames < totalFrames)
        {
            gb_frame (&app->gb);
            frames++;
            printf("\033[A\33[2KT\rFrames: %d\n", frames);
        }
    }

    totalSeconds = (float) frames / 60.0;

    free (app->gb.cart.romData);
    free (app->gb.cart.ramData);

    LOG_("The program ran %f seconds for %d frames.\nGB performance is %f times as fast.\n",
        totalTime, frames, totalSeconds / totalTime);
    LOG_("For each second, there is on average %.2f milliseconds free for overhead.\n",
        1000 - (1.0f / (totalSeconds / totalTime) * 1000));

#ifdef USE_GLFW
    if (app->draw)
    {
        glfwDestroyWindow (app->window);
        glfwTerminate();
        exit (EXIT_SUCCESS);
    }
#endif
}