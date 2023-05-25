
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "api/glfw/helpers/linmath.h"
#include "api/glfw/triangle_test.h"

#define GB_DEBUG
#include "gb.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
 
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
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

int main (int argc, char * argv[])
{
    LOG_("Hello! This is GB-Emu.\n");
#ifdef GB_DEBUG
    printf("ArgCount: %d\n", argc);
#endif
    GameBoy GB;

    uint8_t useGLFW = 1;
    if (useGLFW)
    {
        GLFWwindow* window;
        glfwSetErrorCallback(error_callback);
    
        if (!glfwInit())
            exit(EXIT_FAILURE);
    
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
        window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
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
    
        /* Game Boy test stuff */
        if (argc < 2)
            gb_init (&GB, "");
        else
            gb_init (&GB, argv[1]);

        const uint32_t totalFrames = 60;
        const float totalSeconds = (float)totalFrames / 60.0;
        
        uint32_t i;
        uint8_t gbFinished = 0;

        /* Start clock */
        clock_t t;
        t = clock();

        while (!glfwWindowShouldClose(window))
        { 
            float ratio;
            int width, height;

            glfwGetFramebufferSize(window, &width, &height);
            ratio = width / (float) height;
    
            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT);

            scene_draw_triangle (program, ratio);

            if (GB.frames < totalFrames) /*for (i = 0; i < totalFrames; i++) */
                gb_frame (&GB);
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
    
        glfwDestroyWindow(window);
    
        glfwTerminate();
        exit(EXIT_SUCCESS);
    }

    return 0;
}