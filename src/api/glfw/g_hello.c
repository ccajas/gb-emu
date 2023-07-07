#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../../gb.h"

/* Build command:
$(pkg-config --cflags glfw3) ?
cc -Wall -s -Os -o g_hello glad.c g_hello.c $(pkg-config --static --libs glfw3)
*/

#define LOG_(f_, ...) printf((f_), ##__VA_ARGS__)

void framebuffer_size_callback(GLFWwindow * window, int width, int height);
void processInput             (GLFWwindow * window);

/* process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly */

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char * vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
const char * fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";

struct App
{
    GLFWwindow * window;

    bool paused;
    struct GB gb;
};

void app_config (struct App * app, uint8_t const argc, char * const argv[]) 
{ 
    app->paused = true;
}

/* GLFW variables */

static unsigned int shaderProgram;
static uint32_t quadVAO[2] = { 0, 0 };

void app_init (struct App * app) 
{ 
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow * window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GB Emu test", NULL, NULL);
    if (window == NULL)
    {
        LOG_("Failed to create GLFW window\n");
        glfwTerminate();
        //return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer (window, app);
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader ((GLADloadproc)glfwGetProcAddress))
    {
        LOG_("Failed to load GLAD\n");
        //return -1;
    }
    app->window = window;

    /* Create quad buffer */
    uint32_t quadVBO = 0;
    const float width = 0.5f;
    const float height = 0.5f;

    const float quadVertices[] = 
    {
        -width,  height, 0.0f, 0.0f, 0.0f,
        -width, -height, 0.0f, 0.0f, 1.0f,
         width,  height, 0.0f, 1.0f, 0.0f,
         width, -height, 0.0f, 1.0f, 1.0f,
    };

    glGenVertexArrays(1, &quadVAO[0]);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO[0]);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 

    /* Build and compile shader program */
    int success;
    char infoLog[512];

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        LOG_("Vertex shader | COMPILATION_FAILED\n%s\n", infoLog);
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        LOG_("Fragment shader | COMPILATION_FAILED\n%s\n", infoLog);
    }
    /* Link shaders */
    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) 
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        LOG_("Shader program | LINKING_FAILED\n%s\n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void app_draw (struct App * app)
{
    glClearColor(0.2f, 0.26f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);

    glBindVertexArray(quadVAO[0]);
    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0); /* Optional to unbind */
    /* Update buffers and events */
    glfwSwapBuffers(app->window);
}

void app_run (struct App * app) 
{
    /* Start clock */
    clock_t time;
    double totalTime = 0;
    double lastUpdate = 0;
    uint32_t frames = 0;

    while (!glfwWindowShouldClose (app->window))
    {
        processInput (app->window);
        glfwMakeContextCurrent (app->window);

        double current = glfwGetTime();
        if ((current - lastUpdate) < 1.0 / (GB_FRAME_RATE)) continue;
        //const float fps = 1.0 / (current - lastUpdate);

        //if (gb_rom_loaded(&app->gb))
        {
            if (app->paused == false)
            {
                time = clock();
                gb_frame (&app->gb);
                totalTime += (double)(clock() - time) / CLOCKS_PER_SEC;
                frames++;
            }
        }

        app_draw (app);
        glfwPollEvents();

        lastUpdate = current;
    }
}

int main (int argc, char * argv[])
{
    struct App app;

    app_config (&app, argc, argv);
    app_init (&app);
    app_run (&app);

    return 0;
}