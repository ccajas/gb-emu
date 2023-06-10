#include "graphics.h"

const char * default_fs_source =
"#version 330\n"
"varying vec2 TexCoords;\n"
"uniform sampler2D indexed;\n"
"uniform vec2 screenSize;\n"

"void main()\n"
"{\n"
"    vec3 sampled = texture2D(indexed, TexCoords).rgb;\n"
"    gl_FragColor = vec4(sampled, 1.0);\n"
"}\n";

const char * ppu_fs_source =
"#version 330\n"
"varying vec2 TexCoords;\n"
"uniform sampler2D colorPalette;\n"
"uniform sampler2D indexed;\n"
"uniform vec3 textColor;\n"
"uniform vec2 screenSize;\n"

"vec3 dotMatrix(vec3 color, vec3 tint)\n"
"{\n"
"    vec2 position = (TexCoords.xy);\n"
"    //color = vec3(0.05) + (color * vec3(0.95)) - 0.05;\n"
"    if (fract(position.x * screenSize.x) > 0.75) color = mix(color, vec3(0), 0.3);"
"    if (fract(position.y * screenSize.y) > 0.75) color = mix(color, vec3(0), 0.12);"
"    //color *= 1.2;\n"
"    return color;\n"
"}\n"

"void main()\n"
"{\n"
"    vec3 tint = vec3(0.37, 0.84, 0.87);\n"
"    vec3 tint2 = vec3(0.61, 0.74, 0.03);\n"
"    vec3 sampled = texture2D(indexed, TexCoords).rgb;\n"
"    gl_FragColor = vec4(dotMatrix(sampled, tint2), 1.0);\n"
"}\n";

const char * ppu_vs_source =
"#version 330\n"
"layout (location = 0) in vec4 vertex;\n"
"layout (location = 1) in vec2 texture;"
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"
"uniform mat4 model;\n"
"uniform vec2 screenSize;\n"

"void main()\n"
"{\n"
"    vec4 localPos = model * vec4(vertex.xy, 1.0, 1.0);"
"    gl_Position = projection * vec4(localPos.xy, 0.0, 1.0);\n"
"    TexCoords = texture.xy;\n"
"}\n";

uint32_t quadVAO[2] = { 0, 0 };

void draw_lazy_quad(const float width, const float height, const int i)
{
    while (quadVAO[i] == 0)
    {
        uint32_t quadVBO = 0;
        const float quadVertices[] = {

            0.0f,  height, 0.0f, 0.0f, 0.0f,
            0.0f,  0.0f,   0.0f, 0.0f, 1.0f,
            width, height, 0.0f, 1.0f, 0.0f,
            width, 0.0f,   0.0f, 1.0f, 1.0f,
        };

        glGenVertexArrays(1, &quadVAO[i]);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO[i]);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0); 

        break;
    }

    glBindVertexArray(quadVAO[i]);
    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void graphics_init (Scene * const scene)
{
    gladLoadGL();
    glfwSwapInterval(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    /* Create shaders */
    scene->fbufferShader = shader_init_source (ppu_vs_source, ppu_fs_source);
    scene->debugShader   = shader_init_source (ppu_vs_source, default_fs_source);

    scene->activeShader  = &scene->debugShader;
    scene->activeTexture = &scene->debugTexture;

    /* Create main textures */
    texture_setup (&scene->fbufferTexture, 160, 144, GL_NEAREST, NULL);
    texture_setup (&scene->debugTexture,   128, 128, GL_NEAREST, NULL);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDepthFunc(GL_LEQUAL);
}