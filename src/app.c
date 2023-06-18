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

    /* Setup game controls - buttons are assigned in this order: Right, left, Up, Down, A, B, Select, Start */
    const uint16_t GBkeys[8] = { 
        GLFW_KEY_D, GLFW_KEY_A, GLFW_KEY_W, GLFW_KEY_S, 
        GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L, GLFW_KEY_M
    };
    uint8_t k;
    for (k = 0; k < 8; k++) {
        if (key == GBkeys[k] && action == GLFW_PRESS)   app->gb.extData.joypad &= ~(1 << k);
        if (key == GBkeys[k] && action == GLFW_RELEASE) app->gb.extData.joypad |= (1 << k);
    }

    /* Pause emulation */
    if (key == GLFW_KEY_P && action == GLFW_PRESS) 
        if (gb_rom_loaded(&app->gb)) app->paused = !app->paused;

    /* Step one instruction */
    if (key == GLFW_KEY_T && action == GLFW_PRESS) app->step = 1;

    /* Toggle debug */
    if (key == GLFW_KEY_B && action == GLFW_PRESS) app->debug = !app->debug;
    
    /* Toggle window size (scale) */
    if (key == GLFW_KEY_G && action == GLFW_PRESS)
    {
        app->scale += (app->scale == 6) ? -5 : 1;
        glfwSetWindowSize(window, 
            app->gbData.frameBuffer.width * app->scale,
            app->gbData.frameBuffer.height * app->scale);
    }

    const uint8_t totalPalettes = (sizeof(palettes) / sizeof(palettes[0]));
    /* Switch palettes */
    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        app->gbData.paletteBG  = (app->gbData.paletteBG  + 1) % totalPalettes;
        app->gbData.paletteOBJ = (app->gbData.paletteOBJ + 1) % totalPalettes;
    }

    if (key == GLFW_KEY_I && action == GLFW_PRESS)
        app->gbData.paletteOBJ = (app->gbData.paletteOBJ + 1) % totalPalettes;

    /* Reset game */
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        gb_boot_reset(&app->gb);
}

void joystick_callback(int joystickID, int event)
{
    if (event == GLFW_CONNECTED)
        LOG_("Joystick connected.\n");

    else if (event == GLFW_DISCONNECTED)
        LOG_("Joystick diconnected.\n");
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
    app->fullScreen = 0;
    app->paused = 1;
    app->debug = app->step = 0;
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
#ifdef USE_TIGR
        .tileMap = tigrBitmap (128, 128),
        .frameBuffer = tigrBitmap (DISPLAY_WIDTH, DISPLAY_HEIGHT)
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
#ifdef USE_TIGR
    app->screen = tigrWindow(DISPLAY_WIDTH * app->scale, DISPLAY_HEIGHT * app->scale, "GB Emu", TIGR_FIXED);
#endif
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
        glfwSetJoystickCallback(joystick_callback);
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
    LOG_("Attempting to open \"%s\"...\n", fileName);
    FILE * f = fopen (fileName, "rb");

    uint8_t * rom = NULL;
    if (!f)
    {
        LOG_("Failed to load file \"%s\"\n", fileName);
        return NULL;
    }
    else
    {
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

#ifdef USE_GLFW
void app_read_gamepad (struct App * app)
{
    GLFWgamepadstate state;
 
    if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
    {
        LOG_("Gamepad State: ");
        uint8_t i;
        for (i = 0; i < sizeof(state.buttons); i++)
            LOG_("%d ", state.buttons[i]);
        LOG_("\n");

        if (state.buttons[GLFW_GAMEPAD_BUTTON_A])
        {
            
        }
    }
}

#define LERP_(v0, v1, t)   ((1 - t) * v0 + t * v1)

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
        //const uint8_t pal = ((*pixels++ & 0xC) == 4) ? data->paletteBG : data->paletteOBJ;
        const uint8_t * pixel = palettes[data->paletteBG].colors[idx];
        //const float lerp = (pal == 1) ? (float)data->pixelTint / 255 : 0;

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
    set_texture (&app->display, &app->display.fbufferTexture);
    draw_quad (app->window, &app->display, &app->gbData.frameBuffer, 0, 0, app->scale);

    if (app->debug)
    {
        debug_dump_tiles (&app->gb, app->gbData.tileMap.imgData);
        set_shader (&app->display, &app->display.debugShader);
        set_texture (&app->display, &app->display.debugTexture);
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

    const int32_t totalFrames = 60;
    float totalSeconds = (float) totalFrames / 60.0;

    double lastUpdate = 0;

    if (app->draw)
    {
#ifdef USE_GLFW
        while (!glfwWindowShouldClose (app->window))
        {
            double current = glfwGetTime();
            app_read_gamepad(app);
            glfwPollEvents();

            if ((current - lastUpdate) < FPS_LIMIT) continue;

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
                }
            }
            app_draw (app);
            glfwSwapBuffers (app->window);

            lastUpdate = current;
        }
#endif
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
#ifdef USE_TIGR
        while (!tigrClosed(app->screen))
        {
            tigrClear (app->screen, tigrRGB(0x80, 0x90, 0xa0));
            app_draw (app);
        }
        tigrFree (app->screen);
#endif
}