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

    app->draw = 0;
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
#ifdef GB_APP_DRAW
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
#endif
    };

    /* Copy ROM to cart */
    app->gb.cart.romData = app_load(app->defaultFile);

    LOG_("Initializing emulator...\n");
    gb_init (&app->gb);

#ifdef GB_APP_DRAW

    /* Objects for drawing */
    GLFWwindow * window;
    Scene newDisplay = { .bgColor = { 173, 175, 186 }};
    app->display = newDisplay;

    /* Select image to display */
    app->image = &app->gbData.tileMap;

    if (app->draw)
    {
        glfwSetErrorCallback(error_callback);

        if (!glfwInit())
            exit(EXIT_FAILURE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

        window = glfwCreateWindow(
            app->gbData.frameBuffer.width * app->scale,
            app->gbData.frameBuffer.height * app->scale,
            "GB Emu", NULL, NULL);
        
        if (!app->window)
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
    fread (rom, size, 1, f);
    fclose (f);
    return rom;
}

#ifdef APP_DRAW

void app_draw (struct App * app)
{
    /* Select image to display */
    app->image = &app->gbData.tileMap;
    draw_begin (app->window, &app->display);

    //if (mbc_rom_loaded())
        draw_quad (app->window, &app->display, app->image, 224, 0, 2);

    glfwSwapBuffers (app->window);
    glfwPollEvents();
}

#endif

void app_run (struct App * app)
{
    /* Start clock */
    clock_t time = clock();
    uint32_t frames = 0;

    /* Frames for time keeping */
    const int32_t totalFrames = 550;
    float totalSeconds = (float) totalFrames / 60.0;

    if (app->draw)
    {
#ifdef GB_APP_DRAW
        while (!glfwWindowShouldClose (app->window))
        {
            glfwMakeContextCurrent (app->window);

            if (frames < -1 && !app->paused)
            {
                //if (mbc_rom_loaded())
                    //cpu_frame (app->gbData);

                //ppu_dump_tiles (app->gbData.tileMap.data);
                printf("\033[A\33[2KT\rFrames: %d\n", ++frames);
            }
            app_draw (app);
        }
#endif
    }
    else
    {
        while (frames < totalFrames)
        {
            gb_frame (&app->gb);
            frames++;
        }
    }

    //gb_shutdown (&GB);
    time = clock() - time;
    totalSeconds = (float)frames / 60.0;

    free (app->gbData.rom);

    double timeTaken = ((double) time) / CLOCKS_PER_SEC; /* Elapsed time */
    LOG_("The program ran %f seconds for %d frames.\nGB performance is %.2f times as fast.\n",
        timeTaken, frames, totalSeconds / timeTaken);
    LOG_("For each second, there is on average %.2f milliseconds free for overhead.",
        1000 - (1.0f / (totalSeconds / timeTaken) * 1000));  

#ifdef GB_APP_DRAW
    if (app->draw)
    {
        glfwDestroyWindow (app->window);
        glfwTerminate();
        exit (EXIT_SUCCESS);
    }
#endif
}