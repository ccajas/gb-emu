#define DEBUG_TEXTURE_W  320
#define DEBUG_TEXTURE_H  288
#define DEFAULT_SCALE    3

#ifdef ENABLE_AUDIO
    #define MINIAUDIO_IMPLEMENTATION
    #include "../deps/miniaudio/extras/miniaudio_split/miniaudio.h"
#endif

#ifdef GBE_DEBUG
    #define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)
#else
    #define LOG_(f_, ...)
#endif

/* Common functions for different APIs */

#if defined(USE_GLFW)
    #include "gbdebug.h"
    #include "texture.h"

    #define GLFW_INCLUDE_NONE
    #include "api/gl/graphics.h"
    #include <GLFW/glfw3.h>

    #define GBE_APP_CLOSED          glfwWindowShouldClose (app->window)
    #define GBE_GET_TIME()          glfwGetTime()
    #define GBE_DRAW_BEGIN()        draw_begin (&app->display)
    #define GBE_WINDOW_TITLE(s)     glfwSetWindowTitle (app->window, s)
    #define GBE_POLL_EVENTS()       glfwPollEvents()
    #define GBE_APP_CLEANUP()\
        free (app->gbData.tileMap.imgData);\
        free (app->gbData.frameBuffer.imgData);\
        glfwDestroyWindow (app->window);\
        glfwTerminate();\

    #define GBE_DRAW_SWAP_BUFFERS()\
        app_draw (app);\
        glfwSwapBuffers (app->window);\

#elif defined(USE_TIGR)
    #include "api/tigr/tigr.h"

    #define GBE_APP_CLOSED          tigrClosed(screen)
    #define GBE_GET_TIME()          (double)(clock()) / CLOCKS_PER_SEC
    #define GBE_DRAW_BEGIN()        draw_begin (&app->display)
    #define GBE_WINDOW_TITLE(s)
    #define GBE_POLL_EVENTS()
    #define GBE_APP_CLEANUP()       tigrFree(app->window)
    #define GBE_DRAW_SWAP_BUFFERS()\
        app_draw (app);\
        tigrUpdate (app->window);\

#else
    #define GBE_APP_CLOSED          0
    #define GBE_GET_TIME()          (double)(clock()) / CLOCKS_PER_SEC
    #define GBE_DRAW_BEGIN()
    #define GBE_WINDOW_TITLE(s)
    #define GBE_POLL_EVENTS()
    #define GBE_APP_CLEANUP()
    #define GBE_DRAW_SWAP_BUFFERS()

#endif