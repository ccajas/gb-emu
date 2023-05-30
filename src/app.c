#include <string.h>
#include "app.h"

void app_config (struct App * app, uint8_t const argc, char * const argv[])
{
    if (argc >= 2) {
        char * fileName = argv[1];
        strcpy (app->defaultFile, fileName);
    }

    app->draw = 1;
    app->scale = 3;
}

/* GLFW callback functions */

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

void app_init (struct App * app)
{    
    /* Main objects */
    GLFWwindow * window;
    Scene newDisplay = { .bgColor = { 173, 175, 186 }};
    app->display = newDisplay;
    //GameBoy GB;

    /* Define structs for data and concrete functions */
    struct gb_data newData = {
        .rom = app_load (app),
        .bootRom = NULL,
        .tileMap = {
            .width = 128,
            .height = 128,
            .data = calloc(128 * 128 * 3, sizeof(uint8_t))
        },
        .frameBuffer = {
            .width = DISPLAY_WIDTH,
            .height = DISPLAY_HEIGHT,
            .data = calloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * 3, sizeof(uint8_t))
        }
    };
    app->gbData = newData;

    /* Select image to display */
    app->image = &app->gbData.frameBuffer;

    if (app->draw)
    {
        glfwSetErrorCallback(error_callback);

        if (!glfwInit())
            exit(EXIT_FAILURE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

        window = glfwCreateWindow(
            app->image->width * app->scale,
            app->image->height * app->scale,
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
}

uint8_t * app_load (struct App const * app) 
{
    /* Load file from command line */
    FILE * f = fopen (app->defaultFile, "rb");

    fseek (f, 0, SEEK_END);
    uint32_t size = ftell(f);
    fseek (f, 0, SEEK_SET);

    if (!f)
    {
        fclose (f);
        LOG_("Failed to load file \"%s\"\n", app->defaultFile);
        return NULL;
    }
    else
    {
        LOG_("ROM file size: %d\n", size);

        uint8_t * rom = calloc(size, sizeof (uint8_t));
        fread (rom, size, 1, f);
        fclose (f);

        return rom;
    }
}

void app_run (struct App * app)
{
    /* Start clock */
    app->time = clock();
    uint32_t frames = 0;

    /* Frames for time keeping */
    const int32_t totalFrames = 30;
    float totalSeconds = (float) totalFrames / 60.0;

    if (app->draw)
    {
        while (!glfwWindowShouldClose (app->window))
        {
            glfwMakeContextCurrent (app->window);

            if (frames < -1 && !app->paused)
            {
                //frames = gb_frame (&GB);
                printf("\033[A\33[2KT\rFrames: %d\n", frames);
            }

            /* Select image to display */
            app->image = &app->gbData.tileMap;

            draw_begin (app->window, &app->display);
            //draw_screen_quad (app->window, &app->display, app->image, app->scale);

            glfwSwapBuffers (app->window);
            glfwPollEvents();

            if (glfwWindowShouldClose (app->window))
            {
                goto shutdown;
            }
        }
    }
    else
    {
        while (frames < totalFrames)
        {
            //gb_frame (&GB);
            frames++;
        }
        goto shutdown;
    }

    /* Todo: remove the need for this goto */
    shutdown:
        //gb_shutdown (&GB);
        app->time = clock() - app->time;
        totalSeconds = (float)frames / 60.0;

        free (app->gbData.rom);

        double timeTaken = ((double) app->time) / CLOCKS_PER_SEC; /* Elapsed time */
        LOG_("The program ran %f seconds for %d frames.\nGB performance is %.2f times as fast.\n", timeTaken, frames, totalSeconds / timeTaken);
        LOG_("For each second, there is on average %.2f milliseconds free for overhead.", 1000 - (1.0f / (totalSeconds / timeTaken) * 1000));  

    if (app->draw)
    {
        glfwDestroyWindow (app->window);
        glfwTerminate();
        exit (EXIT_SUCCESS);
    }
}