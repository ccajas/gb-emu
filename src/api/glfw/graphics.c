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
"    float px = 1.0/512.0;\n"
"    color = vec3(0.2) + (color * vec3(0.8));\n"
"    if (fract(position.x * screenSize.x) > 0.75) color = mix(color, vec3(1.0), 0.5);"
"    if (fract(position.y * screenSize.y) > 0.75) color = mix(color, vec3(1.0), 0.5);"
"    color *= tint;\n"
"    return color;\n"
"}\n"

"void main()\n"
"{\n"
"    vec3 tint = vec3(0.37, 0.84, 0.87);\n"
"    vec3 tint2 = vec3(0.51, 0.54, 0.03);\n"
"    vec3 sampled = texture2D(indexed, TexCoords).rgb;\n"
"    gl_FragColor = vec4(dotMatrix(sampled, tint), 1.0);\n"
"}\n";

const char * ppu_vs_source =
"#version 330\n"
"layout (location = 0) in vec4 vertex;\n" // <vec2 pos, vec2 tex>
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
    scene->debugShader   = shader_init_source (ppu_vs_source, ppu_fs_source);

    /* Create main textures */
    texture_setup (&scene->fbufferTexture, 160, 144, GL_NEAREST, NULL);
    texture_setup (&scene->debugTexture,   128, 128, GL_NEAREST, NULL);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDepthFunc(GL_LEQUAL);
}

void draw_begin (GLFWwindow * window, Scene * const scene)
{
	glClearColor((GLfloat)scene->bgColor[0] / 255, (GLfloat)scene->bgColor[1] / 255, (GLfloat)scene->bgColor[2] / 255, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void draw_screen_quad (GLFWwindow * window, Scene * const scene, struct Texture * const pixels, const float scale)
{
    int32_t width, height;
    glfwGetFramebufferSize (window, &width, &height);

    /* Render the PPU framebuffer here */
    glUseProgram(scene->debugShader.program);
    mat4x4 model;
    mat4x4 projection;

    mat4x4_ortho (projection, 0, width, 0, height, 0, 0.1f);
    mat4x4_identity (model);
    mat4x4_scale_aniso (model, model, width, height, 1.0f);

    glUniform2f (glGetUniformLocation(scene->debugShader.program, "screenSize"), (GLfloat) (width / scale), (GLfloat) (height / scale));
    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "model"),      1, GL_FALSE, (const GLfloat*) model);
    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "projection"), 1, GL_FALSE, (const GLfloat*) projection);

    glActiveTexture (GL_TEXTURE0);

    /* Draw framebuffer */
    glBindTexture (GL_TEXTURE_2D, scene->debugTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, pixels->width, pixels->height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels->data);
	draw_lazy_quad(1.0f, 1.0f, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
}