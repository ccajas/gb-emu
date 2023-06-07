#include <string.h>
#include <time.h>
#include "app.h"
#include "palettes.h"
#ifdef USE_TIGR
    #include "app_tigr.h"
#endif

/* GLFW callback functions */
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

    /* Game controls */
    if (key == GLFW_KEY_J && action == GLFW_PRESS) app->gb.extData.joypad &= 0xEF; /* A button      */
    if (key == GLFW_KEY_K && action == GLFW_PRESS) app->gb.extData.joypad &= 0xDF; /* B button      */
    if (key == GLFW_KEY_L && action == GLFW_PRESS) app->gb.extData.joypad &= 0xBF; /* Select button */
    if (key == GLFW_KEY_M && action == GLFW_PRESS) app->gb.extData.joypad &= 0x7F; /* Start button  */
    if (key == GLFW_KEY_D && action == GLFW_PRESS) app->gb.extData.joypad &= 0xFE; /* D-pad Right   */
    if (key == GLFW_KEY_A && action == GLFW_PRESS) app->gb.extData.joypad &= 0xFD; /* D-pad Left    */
    if (key == GLFW_KEY_W && action == GLFW_PRESS) app->gb.extData.joypad &= 0xFB; /* D-pad Up      */
    if (key == GLFW_KEY_S && action == GLFW_PRESS) app->gb.extData.joypad &= 0xF7; /* D-pad Down    */

    if (key == GLFW_KEY_J && action == GLFW_RELEASE) app->gb.extData.joypad |= 0x10; /* A button      */
    if (key == GLFW_KEY_K && action == GLFW_RELEASE) app->gb.extData.joypad |= 0x20; /* B button      */
    if (key == GLFW_KEY_L && action == GLFW_RELEASE) app->gb.extData.joypad |= 0x40; /* Select button */
    if (key == GLFW_KEY_M && action == GLFW_RELEASE) app->gb.extData.joypad |= 0x80; /* Start button  */
    if (key == GLFW_KEY_D && action == GLFW_RELEASE) app->gb.extData.joypad |= 0x1;  /* D-pad Right   */
    if (key == GLFW_KEY_A && action == GLFW_RELEASE) app->gb.extData.joypad |= 0x2;  /* D-pad Left    */
    if (key == GLFW_KEY_W && action == GLFW_RELEASE) app->gb.extData.joypad |= 0x4;  /* D-pad Up      */
    if (key == GLFW_KEY_S && action == GLFW_RELEASE) app->gb.extData.joypad |= 0x8;  /* D-pad Down    */

    /* Pause emulation */
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        app->paused = !app->paused;

    /* Toggle debug */
    if (key == GLFW_KEY_B && action == GLFW_PRESS)
        app->debug = !app->debug;

    /* Switch palette */
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        app->gbData.palette++;
        if (app->gbData.palette >= sizeof(palettes) / sizeof(palettes[0]))
            app->gbData.palette = 0;
    }

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

    /* Copy ROM to cart */
    if (app_load(&app->gb, app->defaultFile))
    {
        app->gb.extData.ptr = &app->gbData;
        app->paused = 0;
    }
    else app->defaultFile[0] = '\0';
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
    app->paused = 1;
    app->debug = 0;
}

void app_init (struct App * app)
{    
    /* Define structs for data and concrete functions */
    app->gb.cart.romData = NULL;
    app->gb.cart.ramData = NULL;

    app->gbData = (struct gb_data) 
    {
        .palette = 0,
#ifdef USE_GLFW
        .tileMap = {
            .width = 128,
            .height = 192,
            .imgData = calloc (128 * 192 * 3, sizeof(uint8_t))
        },
        .frameBuffer = {
            .width = DISPLAY_WIDTH,
            .height = DISPLAY_HEIGHT,
            .imgData = calloc (DISPLAY_WIDTH * DISPLAY_HEIGHT * 3, sizeof(uint8_t))
        }
#endif
    };

    /* Handle file loading */
    if (!strcmp(app->defaultFile, "\0"))
        LOG_("No file selected.\n");
    else
    {
        /* Copy ROM to cart */
        if (app_load(&app->gb, app->defaultFile))
        {
            app->gb.extData.ptr = &app->gbData;
            app->paused = 0;
        }
        else app->defaultFile[0] = '\0';
    }

#ifdef USE_GLFW
    /* Objects for drawing */
    Scene newDisplay = { .bgColor = { 173, 175, 186 }};
    app->display = newDisplay;

    /* Assign functions to be used by emulator */
    app->gb.draw_line = app_draw_line;

    /* Select image to display */
    app->image = &app->gbData.tileMap;

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

uint8_t * app_load (struct GB * gb, const char * fileName)
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

    /* Load boot file */
    FILE * fb = fopen ("test/dmg_boot.bin", "rb");
    
    uint8_t * boot = calloc(BOOT_ROM_SIZE, sizeof (uint8_t));
    if (!fread (boot, BOOT_ROM_SIZE, 1, f))
        boot = NULL;

    fclose (fb);

    /* Copy ROM to cart */
    LOG_("GB: \"%s\"\n", fileName);
    gb->cart.romData = rom;
    if (gb->cart.romData) gb_init (gb, boot);

    return rom;
}

#ifdef USE_GLFW
void app_draw_line (void * dataPtr, const uint8_t * pixels, const uint8_t line)
{
    struct gb_data * const data = dataPtr;

    const uint32_t yOffset = line * DISPLAY_WIDTH * 3;
    uint8_t  coloredPixels[DISPLAY_WIDTH * 3];

    uint8_t x;
	for (x = 0; x < DISPLAY_WIDTH; x++)
	{
		uint8_t idx = 3 - (*pixels++);
        const uint8_t * pixel = palettes[data->palette].colors[idx];//(pixel == 3) ? 0xF5 : pixel * 0x55;

        coloredPixels[x * 3]     = pixel[0];
        coloredPixels[x * 3 + 1] = pixel[1];
        coloredPixels[x * 3 + 2] = pixel[2];
	}
    memcpy (data->frameBuffer.imgData + yOffset, coloredPixels, DISPLAY_WIDTH * 3);
}

void app_draw (struct App * app)
{
    /* Select image to display */
    app->image = &app->gbData.tileMap;
    const uint16_t w = app->gbData.frameBuffer.width  * app->scale;
    //const uint16_t h = app->gbData.frameBuffer.height * app->scale;

    set_shader (&app->display, &app->display.fbufferShader);
    draw_quad (app->window, &app->display, &app->gbData.frameBuffer, 0, 0, app->scale);

    if (app->debug)
    {
        debug_dump_tiles (&app->gb, app->gbData.tileMap.imgData);
        set_shader (&app->display, &app->display.debugShader);
        if (gb_rom_loaded(&app->gb))
            draw_quad (app->window, &app->display, app->image, w - 128, 0, 1);
    }
}
#endif

void app_run (struct App * app)
{
    /* Start clock */
    clock_t time;
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

            if (frames < -1 && app->paused == 0 && gb_rom_loaded(&app->gb))
            {
                time = clock();
                gb_frame (&app->gb);
                totalTime += (double)(clock() - time) / CLOCKS_PER_SEC;

                frames++;
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

    LOG_("The emulation took %f seconds for %d frames.\nGB performance is %f times as fast.\n",
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