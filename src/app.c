#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "app.h"
#include "palettes.h"
#ifdef USE_TIGR
    #include "app_tigr.h"
#endif

/* GLFW callback functions */
#ifdef USE_GLFW

/* Forward declaration to resize window */
void app_resize_window (GLFWwindow * window, const uint8_t, const uint8_t scale);

void error_callback (int error, const char* description)
{
    fprintf(stderr, "Error (%d): %s\n", error, description);
}
 
void key_callback (GLFWwindow * window, int key, int scancode, int action, int mods)
{
    struct App * app = glfwGetWindowUserPointer (window);

    uint8_t k;
    for (k = 0; k < 8; k++) {
        if (key == app->GBkeys[k] && action == GLFW_PRESS)   app->gb.extData.joypad &= ~(1 << k);
        if (key == app->GBkeys[k] && action == GLFW_RELEASE) app->gb.extData.joypad |= (1 << k);
    }

    /* Pause emulation */
    if (key == GLFW_KEY_P && action == GLFW_PRESS) 
        if (gb_rom_loaded(&app->gb)) app->paused = !app->paused;

    /* Toggle debug */
    if (key == GLFW_KEY_B && action == GLFW_PRESS) 
    {
        app->debug = !app->debug;
        app_resize_window (window, app->debug, app->scale);
    }
    
    /* Toggle window size (scale) */
    if (key == GLFW_KEY_G && action == GLFW_PRESS)
    {
        app->scale += (app->scale == 6) ? -5 : 1;
        app_resize_window (window, app->debug, app->scale);
    }

    const uint8_t totalPalettes = (sizeof(palettes) / sizeof(palettes[0]));
    /* Switch palettes */
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
        app->gbData.paletteBG = (app->gbData.paletteBG  + 1) % totalPalettes;

    /* Reset game */
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        gb_boot_reset(&app->gb);
}

void joystick_callback(int joystickID, int event)
{
    if (event == GLFW_CONNECTED)
        LOG_("Joystick connected.\n");
}

void framebuffer_size_callback (GLFWwindow * window, int width, int height)
{
    /* make sure the viewport matches the new window dimensions; note that width and 
       height will be significantly larger than specified on retina displays. */
    glViewport(0, 0, width, height);
}

void app_resize_window (GLFWwindow * window, const uint8_t debug, const uint8_t scale)
{
    /* Resize accordingly if in debug mode. In non-debug it is always a 160x144 (scaled) window */
    struct Texture newWindow = { 
        .width = DISPLAY_WIDTH * scale + ((debug) ? 480 : 0), 
        .height = DISPLAY_HEIGHT * scale 
    };
    glfwSetWindowSize (window, newWindow.width, newWindow.height);
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
    app->fullScreen = 0;
    app->paused = 1;
    app->debug = app->step = 0;
    /* Setup game controls - buttons are assigned in this order: Right, left, Up, Down, A, B, Select, Start */
    const uint16_t GBkeys[8] = { 
        GLFW_KEY_D, GLFW_KEY_A, GLFW_KEY_W, GLFW_KEY_S, 
        GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L, GLFW_KEY_M
    };
    memcpy(&app->GBkeys, GBkeys, sizeof(uint16_t) * 8);
    /* Frameskip (for 30 FPS) */
    app->gb.extData.frameSkip = 0;
}

void app_init (struct App * app)
{    
    /* Define structs for data and concrete functions */
    app->gb.cart.romData = NULL;
    app->gb.cart.ramData = NULL;

    app->gbData = (struct gb_data) 
    {
        .paletteBG = 0,
        .paletteOBJ = 0,
        .pixelTint = 0,
        .tileMap = {
            .width = 240,
            .height = 432,
            .imgData = calloc (240 * 432 * 3, sizeof(uint8_t))
        },
        .frameBuffer = {
            .width = DISPLAY_WIDTH,
            .height = DISPLAY_HEIGHT,
            .imgData = calloc (DISPLAY_WIDTH * DISPLAY_HEIGHT * 3, sizeof(uint8_t))
        }
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
        glfwSetJoystickCallback(joystick_callback);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwMakeContextCurrent(window);

        graphics_init (&app->display);
        app->window = window;
    }
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

    /* Load boot file if present */
/*    FILE * fb = fopen ("test/dmg_boot.bin", "rb");
    
    uint8_t * boot = calloc(BOOT_ROM_SIZE, sizeof (uint8_t));
    if (!fread (boot, BOOT_ROM_SIZE, 1, f))
        boot = NULL;
    else
        fclose (fb);
*/
    uint8_t * boot = NULL;
    /* Copy ROM to cart */
    LOG_("GB: \"%s\"\n", fileName);
    gb->cart.romData = rom;
    if (gb->cart.romData) gb_init (gb, boot);

    return rom;
}

void app_draw_line (void * dataPtr, const uint8_t * pixels, const uint8_t line)
{
    struct gb_data * const data = dataPtr;

    const uint32_t yOffset = line * DISPLAY_WIDTH * 3;
    uint8_t coloredPixels[DISPLAY_WIDTH * 3];

    uint8_t x;
	for (x = 0; x < DISPLAY_WIDTH; x++)
	{
        /* Get color and palette to use it with */
		const uint8_t idx = (3 - *pixels++) & 3;
        const uint8_t * pixel = palettes[data->paletteBG].colors[idx];

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

    set_shader  (&app->display, &app->display.fbufferShader);
    set_texture (&app->display, &app->display.fbufferTexture);
    draw_quad (app->window, &app->display, &app->gbData.frameBuffer, 0, 0, app->scale);

    if (app->debug)
    {
        debug_dump_tiles (&app->gb, app->gbData.tileMap.imgData);
        render_text (font8x8_basic, 
            app->debugString, app->gbData.tileMap.width, 
            app->gbData.tileMap.imgData);
        set_shader (&app->display, &app->display.debugShader);
        set_texture (&app->display, &app->display.debugTexture);
        if (gb_rom_loaded(&app->gb))
            draw_quad (app->window, &app->display, app->image, w, 0, 2);
    }
}

void app_run (struct App * app)
{
    /* Start clock */
    clock_t time;
    double totalTime = 0;
    uint32_t frames = 0;

    const int32_t totalFrames = 60;

    if (app->draw)
    {
        double lastUpdate = 0;
        while (!glfwWindowShouldClose (app->window))
        {
            double current = glfwGetTime();
            glfwPollEvents();

            if ((current - lastUpdate) < FPS_LIMIT) continue;

            //printf("\rFPS: %f", 1.0 / (current - lastUpdate));
            glfwMakeContextCurrent (app->window);
            draw_begin (app->window, &app->display);

            if (gb_rom_loaded(&app->gb))
            {
                if (app->paused == 0)
                {
                    time = clock();
                    gb_frame (&app->gb);
                    totalTime += (double)(clock() - time) / CLOCKS_PER_SEC;
                    frames++;
                    if (frames % 30 == 29)
                        sprintf(app->debugString, "Perf: %0.2fx", (double)(frames / 59.7275) / totalTime);
                }
            }
            app_draw (app);
            glfwSwapBuffers (app->window);
            lastUpdate = current;
        }
    }
    else
    {
        while (frames < totalFrames)
        {
            time = clock();
            gb_frame (&app->gb);
            totalTime += (double)(clock() - time) / CLOCKS_PER_SEC;
            frames++;
        }
    }

    float totalSeconds = (float) frames / 60.0;

    free (app->gb.cart.romData);
    free (app->gb.cart.ramData);

    LOG_("\nThe emulation took %f seconds for %d frames.\nGB performance is %f times as fast.\n",
        totalTime, frames, totalSeconds / totalTime);
    LOG_("For each second, there is on average %.2f milliseconds free for overhead.\n",
        1000 - (1.0f / (totalSeconds / totalTime) * 1000));

    if (app->draw)
    {
        glfwDestroyWindow (app->window);
        glfwTerminate();
        exit (EXIT_SUCCESS);
    }
}