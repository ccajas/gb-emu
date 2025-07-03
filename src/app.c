#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "app.h"
#ifdef NO_FILE_LOAD
    #include "rom.h"
#endif
#ifdef USE_TIGR
    #include "app_tigr.h"
#endif

/* GLFW callback functions */
#ifdef USE_GLFW

/* Forward declaration to resize window */
void app_resize_window (GLFWwindow * window, Scene * display, const uint8_t scale);

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
        if (!app->fullScreen)
            app_resize_window (window, &app->display, app->scale);
    }
    
    /* Toggle window size (scale) */
    if (key == GLFW_KEY_G && action == GLFW_PRESS)
    {
        app->scale += (app->scale == 6) ? -5 : 1;
        if (!app->fullScreen)
            app_resize_window (window, &app->display, app->scale);
    }

    /* Toggle fullscren */
    if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
    {
        app->fullScreen = !app->fullScreen;
        GLFWmonitor * pMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode * mode = glfwGetVideoMode(pMonitor);
        const uint16_t winPos = (!app->fullScreen) ? mode->height / 4 : 0;

        glfwSetWindowMonitor(window, (app->fullScreen) ? pMonitor : NULL, winPos, winPos,
            mode->width, mode->height, GLFW_DONT_CARE);
        /* Resize back if exiting fullscreen mode */
        if (!app->fullScreen)
            app_resize_window (window, &app->display,  app->scale);
    }

    const uint8_t totalPalettes = (sizeof(manualPalettes) / sizeof(manualPalettes[0]));
    /* Switch palettes */
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
        app->gbData.palette = (app->gbData.palette + 3) % totalPalettes;

    /* Reset game */
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        gb_boot_reset(&app->gb);
}

void joystick_callback(int joystickID, int event)
{    
    if (event == GLFW_CONNECTED)
        LOG_("Joystick connected.\n");

    if (event == GLFW_DISCONNECTED)
        LOG_("Joystick disconnected.\n");
}

void framebuffer_size_callback (GLFWwindow * window, int width, int height)
{
    /* make sure the viewport matches the new window dimensions; note that width and 
       height will be significantly larger than specified on retina displays. */
    glViewport(0, 0, width, height);
}

void app_resize_window (GLFWwindow * window, Scene * display, const uint8_t scale)
{
    /* Resize accordingly if in debug mode. In non-debug it is always a 160x144 (scaled) window */
    struct Texture newWindow = { 
        .width = DISPLAY_WIDTH * scale, 
        .height = DISPLAY_HEIGHT * scale 
    };
    glfwSetWindowSize (window, newWindow.width, newWindow.height);
    display->fbWidth  = newWindow.width;
    display->fbHeight = newWindow.height;
}

void drop_callback (GLFWwindow * window, int count, const char** paths)
{
    struct App * app = glfwGetWindowUserPointer (window);

    LOG_("%s\n", paths[0]);
    strcpy(app->defaultFile, paths[0]);

    /* Copy ROM to cart */
    if (app_load(&app->gb, app->defaultFile))
    {
        app->gb.extData.ptr = &app->gbData;
        app->gbData.palette = gbcChecksumPalettes[app->gb.cart.checksum] * 3;
        app->paused = 0;
    }
    else app->defaultFile[0] = '\0';
}

#endif

#ifdef ENABLE_AUDIO

/* Approx. ceiling for more correct sounding pitch */
#define GB_SAMPLE_RATE      48000
#define BUF_SIZE            (int)(GB_SAMPLE_RATE / 60)
#define CYCLES_PER_SAMPLE   (int)(CPU_FREQ_DMG / GB_SAMPLE_RATE) + 1

static int16_t audioBuf[BUF_SIZE] = {0};
unsigned int samplePos = 0;
ma_pcm_rb pRB;

void audio_data_callback (ma_device * pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    const int frames = frameCount * pDevice->playback.channels * ma_get_bytes_per_sample(pDevice->playback.format);
    struct App * app = pDevice->pUserData;

    if (app->paused)
        return;

    /* Capture samples */
    ma_pcm_rb_acquire_read(&pRB, &frameCount, (void**)&audioBuf);
    int i;
    for (i = 0; i < BUF_SIZE; i++)
        audioBuf[i] = gb_update_audio(&app->gb, CYCLES_PER_SAMPLE);

    memcpy(pOutput, audioBuf, frames);
    ma_pcm_rb_commit_read(&pRB, frameCount);
}

void app_audio_init(struct App * app)
{
    ma_device_config config   = ma_device_config_init(ma_device_type_playback);
    config.periodSizeInFrames = BUF_SIZE;
    config.playback.format    = ma_format_s16;
    config.playback.channels  = 1;
    config.sampleRate         = GB_SAMPLE_RATE;
    config.dataCallback       = audio_data_callback;
    config.pUserData          = app;
    
    if (ma_device_init(NULL, &config, &app->audioDevice) != MA_SUCCESS) {
        printf("device initialzation failed\n");
        return;
    }

    ma_result result_rb = ma_pcm_rb_init(ma_format_s16, 1, BUF_SIZE, NULL, NULL, &pRB);
    if (result_rb != MA_SUCCESS) {
        printf("Could not create the ring buffer\n");
        return;
    }

    LOG_("Using %d Hz sample rate\n", app->audioDevice.sampleRate);
    ma_device_start(&app->audioDevice);
}

#endif

void app_config (struct App * app, uint8_t const argc, char * const argv[])
{
    if (argc >= 2) {
        char * fileName = argv[1];
        strcpy (app->defaultFile, fileName);
    }
    else app->defaultFile[0] = '\0';

#if defined(USE_GLFW)
    app->draw = 1;
    /* Setup game controls - buttons are assigned in this order: Right, left, Up, Down, A, B, Select, Start */
    const uint16_t GBkeys[8] = { 
        GLFW_KEY_D, GLFW_KEY_A, GLFW_KEY_W, GLFW_KEY_S, 
        GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L, GLFW_KEY_M
    };
    const uint8_t GBbuttons[8] = { 
        GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
        GLFW_GAMEPAD_BUTTON_DPAD_UP,    GLFW_GAMEPAD_BUTTON_DPAD_DOWN, 
        GLFW_GAMEPAD_BUTTON_A,    GLFW_GAMEPAD_BUTTON_B, 
        GLFW_GAMEPAD_BUTTON_BACK, GLFW_GAMEPAD_BUTTON_START
    };
    memcpy(&app->GBkeys,    GBkeys,    sizeof(uint16_t) * 8);
    memcpy(&app->GBbuttons, GBbuttons, sizeof(uint8_t)  * 8);

#else
    app->draw = 0;
#endif
    app->scale = DEFAULT_SCALE;
    app->joystickID = 0;
    app->fullScreen = 0;
    app->paused = 1;
    app->debug = app->step = 0;
    /* Frameskip (for slower FPS) */
    app->gb.extData.frameSkip = 0;
}

void app_init (struct App * app)
{    
    /* Define structs for data and concrete functions */
    app->gb.cart.romData = NULL;
    app->gb.cart.ramData = NULL;

    app->gbData = (struct gb_data) 
    {
        .palette = 0,
        .pixelTint = 0,
#if defined(USE_GLFW)
        .tileMap = {
            .width = DEBUG_TEXTURE_W,
            .height = DEBUG_TEXTURE_H,
            .imgData = calloc (DEBUG_TEXTURE_W * DEBUG_TEXTURE_H * 3, sizeof(uint8_t))
        },
        .frameBuffer = {
            .width = DISPLAY_WIDTH,
            .height = DISPLAY_HEIGHT,
            .imgData = calloc (DISPLAY_WIDTH * DISPLAY_HEIGHT * 3, sizeof(uint8_t))
        }
#endif
    };

    /* Handle file loading */
#ifndef NO_FILE_LOAD
    if (!strcmp(app->defaultFile, "\0"))
        LOG_("No file selected.\n");
    else
#endif
    {
        /* Copy ROM to cart */
        if (app_load(&app->gb, app->defaultFile))
        {
            app->gb.extData.ptr = &app->gbData;
            app->gbData.palette = gbcChecksumPalettes[app->gb.cart.checksum] * 3;
            app->paused = 0;
        }
        else app->defaultFile[0] = '\0';
    }

    /* Assign functions to be used by emulator */
    app->gb.cart.rom_read = app_cart_rom_read;
    app->gb.draw_line     = app_draw_line;

#ifdef ENABLE_AUDIO
    app_audio_init(app);
#endif

#if defined(USE_GLFW)
    /* Objects for drawing */
    Scene newDisplay = { .bgColor = { 17, 18, 19 }};
    app->display = newDisplay;

    /* Select image to display */
    app->image = &app->gbData.tileMap;

    app->display.fbWidth  = app->gbData.frameBuffer.width  * app->scale;
    app->display.fbHeight = app->gbData.frameBuffer.height * app->scale;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

    GLFWwindow * window;
    window = glfwCreateWindow(
        app->display.fbWidth, app->display.fbHeight,
        "GB Emu", NULL, NULL);

    LOG_("Set up window\n");
    
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

    glfwSwapInterval(1);
    app->window = window;

    graphics_init (&app->display);
#endif
}

uint8_t * app_load (struct GB * gb, const char * fileName)
{ 
    /* Load file from command line */
#ifndef NO_FILE_LOAD

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
#else
    uint8_t * boot = NULL;

    LOG_("Loading default: \"%s\"\n", "pocket.gb");
    gb->cart.romData = rom_gb240p_gb;
    if (gb->cart.romData) gb_init (gb, boot);

    return rom_gb240p_gb;
#endif
}

uint8_t app_cart_rom_read (void * dataPtr, const uint32_t addr)
{
    struct Cartridge * const cart = dataPtr;

    return cart->romData[addr];
}

void app_draw_line (void * dataPtr, const uint8_t * pixels, const uint8_t line)
{
#ifdef USE_GLFW
    struct gb_data * const data = dataPtr;

    const uint32_t yOffset = line * DISPLAY_WIDTH * 3;
    uint8_t * pixel = NULL;

    uint8_t x;
	for (x = 0; x < DISPLAY_WIDTH; x++)
	{
        /* Get color and palette to use it with */
        const uint8_t idx = (3 - *pixels) & 3;
        const uint8_t pal = ((*pixels++ >> 2) & 3) - 1;
        pixel = (uint8_t*) manualPalettes[data->palette + pal].colors[idx];
        
        memcpy (data->frameBuffer.imgData + yOffset + x * 3, pixel, 3);
	}
#endif
}

void app_draw (struct App * app)
{
#ifdef USE_GLFW
    /* Select image to display */
    app->image = &app->gbData.tileMap;
    const uint16_t w = app->gbData.frameBuffer.width  * app->scale;

    set_shader  (&app->display, &app->display.fbufferShader);
    set_texture (&app->display, &app->display.fbufferTexture);

    if (app->debug)
        draw_quad (&app->display, &app->gbData.frameBuffer, 0, 0, app->scale);
    else
        draw_screen_quad (&app->display, &app->gbData.frameBuffer, app->scale);

    if (app->debug)
    {
        const unsigned char (*font)[6] = font_zxpix;
        /* Print text at (8, 0) */
        app_imgPtr_XY (&app->gbData.tileMap, 8, 1);
        const uint32_t color = 0xE6E6FA, bgColor = 0x333333;
        render_text (font, 
            app->fpsString, app->gbData.tileMap.width, 
            0x40c0FF, 0, app->gbData.tileMap.ptr);
        /* Print OAM data */
        app_imgPtr_XY(&app->gbData.tileMap, 8, 9);
        sprintf (app->debugString, "OBJ   X   Y   ID  attr");
        render_text (font, 
            app->debugString, app->gbData.tileMap.width, 
            color, bgColor, app->gbData.tileMap.ptr);
        int obj;
        for (obj = 0; obj < 20; obj++)
        {
            sprintf (app->debugString, " %2d %3d %3d | $%02x $%02x ", 
                obj,
                app->gb.oam[obj * 4 + 1], app->gb.oam[obj * 4],
                app->gb.oam[obj * 4 + 2], app->gb.oam[obj * 4 + 3]
            );
            app_imgPtr_XY(&app->gbData.tileMap, 8, 17 + (obj % 20) * 8);
            render_text (font,
                app->debugString, app->gbData.tileMap.width, 
                0xB5B5B5, 0x0A100F, app->gbData.tileMap.ptr);
        }

        /* Draw debug tiles at (176, 9) */
        app_imgPtr_XY (&app->gbData.tileMap, 176, 9);
        debug_dump_tiles (&app->gb, 
            app->gbData.tileMap.width, app->gbData.tileMap.ptr);

        set_shader (&app->display, &app->display.debugShader);
        set_texture (&app->display, &app->display.debugTexture);
        if (gb_rom_loaded(&app->gb))
            draw_quad (&app->display, app->image, w, 0, 2);
    }
#endif
}

enum { NS_PER_SECOND = 1000000000 };

inline uint64_t as_nanoseconds(struct timespec* ts) {
    return ts->tv_sec * (uint64_t)1000000000L + ts->tv_nsec;
}

void app_run (struct App * app)
{
    uint32_t frames = 0;

    /* Start timekeeping */
    /*struct timespec start, finish;
    uint64_t accu_nsec = 0;*/

    /* Start clock */
    clock_t time;
    clock_t startTime = clock();
    double totalTime = 0;
    double lastUpdate = 0;

    while (!GBE_APP_CLOSED)
    {
        double current = GBE_GET_TIME();
        GBE_POLL_EVENTS();

        if ((current - lastUpdate) < 1.0 / (GB_FRAME_RATE)) continue;

#if defined(USE_GLFW)
        if (app->joystickID != 255 && glfwJoystickIsGamepad(app->joystickID))
        {
            GLFWgamepadstate state;
            glfwGetGamepadState(app->joystickID, &state);

            uint8_t k;
            for (k = 0; k < 8; k++) {
                if (state.buttons[app->GBbuttons[k]] == GLFW_PRESS)   app->gb.extData.joypad &= ~(1 << k);
                if (state.buttons[app->GBbuttons[k]] == GLFW_RELEASE) app->gb.extData.joypad |=  (1 << k);
            }
        }
#endif

        const float fps = 1.0 / (current - lastUpdate);
        GBE_DRAW_BEGIN();

        if (gb_rom_loaded(&app->gb))
        {
            if (app->paused == 0)
            {
                //clock_gettime(CLOCK_REALTIME, &start);
                time = clock();
                gb_frame (&app->gb);
                //clock_gettime(CLOCK_REALTIME, &finish);
                totalTime += (double)(clock() - time) / CLOCKS_PER_SEC;
                //accu_nsec += as_nanoseconds(&finish) - as_nanoseconds(&start);
                ++frames;
                if (frames % 30 == 29)
                {
                    sprintf(app->fpsString, "FPS: %0.2f | Perf: %0.2fx ", 
                        fps, (double)(frames / (GB_FRAME_RATE)) / totalTime);
                    sprintf(app->fpsString, "GB emu | %s", app->gb.extData.title);
                    GBE_WINDOW_TITLE(app->fpsString);
                }
            }
        }

        GBE_DRAW_SWAP_BUFFERS();
        lastUpdate = current;
    }

#ifndef NO_FILE_LOAD
    free (app->gb.cart.romData);
#endif
    free (app->gb.cart.ramData);

    const double totalSeconds = (double)(frames / 60.0);
    //const double totalTime    = (double)(accu_nsec / 1000000000.0);

    double duration =
        (double)(clock() - startTime) / CLOCKS_PER_SEC;
    double fps = frames / duration;

    printf("%f FPS, dur: %f\n", fps, duration);

    printf("The emulation took %f seconds for %d frames.\n"
        "GB performance is %f times as fast (%d FPS average).\n",
        totalTime, frames, totalSeconds / totalTime, (int)(totalSeconds / totalTime * 60.0f));
    printf("For each second, there is on average %.2f milliseconds free for overhead.\n",
            1000.0 - (1.0f / (totalSeconds / totalTime) * 1000));

#ifdef ENABLE_AUDIO
    ma_device_uninit(&app->audioDevice);
#endif
    GBE_APP_CLEANUP();

    exit (EXIT_SUCCESS);
}