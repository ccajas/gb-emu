#include "graphics.h"

const char * default_fs_source =
"#version 130\n"
"varying vec2 TexCoords;\n"
"uniform sampler2D colorPalette;\n"
"uniform sampler2D indexed;\n"
"uniform vec3 textColor;\n"

"void main()\n"
"{\n"
"    vec3 sampled = texture2D(indexed, TexCoords).rgb;\n"
"    gl_FragColor = vec4(sampled, 0.75);\n"
"}\n";

const char * ppu_vs_source =
"#version 330\n"
"layout (location = 0) in vec4 vertex;\n" // <vec2 pos, vec2 tex>
"layout (location = 1) in vec2 texture;"
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"
"uniform mat4 model;\n"

"void main()\n"
"{\n"
"    vec4 localPos = model * vec4(vertex.xy, 1.0, 1.0);"
"    gl_Position = projection * vec4(localPos.xy, 0.0, 1.0);\n"
"    TexCoords = texture.xy;\n"
"}\n";
 
const char * ppu_fs_source =
"#version 130\n"
"varying vec2 TexCoords;\n"
"uniform sampler2D colorPalette;\n"
"uniform sampler2D indexed;\n"
"uniform vec3 textColor;\n"
"uniform float time;\n"

"vec3 applyVignette(vec3 color)\n"
"{\n"
"    vec2 position = (TexCoords.xy) - vec2(0.5);\n"           
"    float dist = length(position);\n"

"    float radius = 1.3;\n"
"    float softness = 1.0;\n"
"    float vignette = smoothstep(radius, radius - softness, dist);\n"
"    color.rgb = color.rgb - (0.8 - vignette);\n"
"    return color;\n"
"}\n"

"float mod(float x, float y) { return x - (y * floor( x / y )); }\n"

"vec2 curveRemapUV(vec2 uv)\n"
"{\n"
    //  as we near the edge of our screen apply greater distortion using a cubic function
"    uv = uv * 2.0 - 1.0;\n"
"    float curvature = 5.0;\n"
"    vec2 offset = abs(uv.yx) / vec2(curvature);\n"
"    uv = uv + uv * offset * offset;\n"
"    uv = uv * 0.5 + 0.5;\n"
"    return uv;\n"
"}\n"

"vec3 applyScanline(vec3 color)\n"
"{\n"
"    vec2 position = (TexCoords.xy);\n"
"    float px = 1.0/512.0;\n"
"    position.x += mod(position.y * 240.0, 2.0) * px;\n"
//"    color *= pow(fract(position.x * 256.0), 0.4);\n"
"    color *= pow(fract(position.y * 240.0), 0.5);\n"
"    return color * 1.67;\n"
"}\n"

"vec3 applyScanline1(vec3 color)\n"
"{\n"
"    vec2 position = (TexCoords.xy);\n"
"    float px = 1.0/512.0;\n"
"    position.x += mod(position.y * 240.0, 2.0) * px;\n"
"    color *= pow(fract(position.x * 256.0 - 128), 1.1);\n"
//"    color *= pow(fract(position.y * 240.0 - 120), 0.25);\n"
"    return color * 1.15;\n"
"}\n"

"void main()\n"
"{\n"
"    vec3 tint    = vec3(113, 115, 126) / 127.0;\n"
"    vec2 tx      = vec2(TexCoords.xy);\n"
//"    if (tx.x < 0.0 || tx.y < 0.0 || tx.x > 1.0 || tx.y > 1.0) {\n"
//"        gl_FragColor = vec4(0, 0, 0, 1.0);\n"
//"        return;\n"
//"    };\n"
//"    tx.x += sin(tx.y * 960.0) / 1024.0;\n"
"    vec3 sampled = texture2D(indexed, tx).rgb;\n"
//"    sampled      = pow(sampled, vec3(1/1.4));\n"
"    sampled      = applyScanline(applyVignette(sampled));\n"
"    gl_FragColor = vec4(pow(sampled, vec3(1.75)), 1.0);\n"
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

uint8_t pixelData[8192 * 2 * 3];

inline void test_pixeldata()
{
    int i;
    for (i = 0; i < 8192; i++)
    {
        int p;
        for (p = 0; p < 6; p++)
            pixelData[i * 6 + p] = 0xff;
    }
}

void graphics_init (Scene * const scene)
{
    gladLoadGL();
    glfwSwapInterval(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    /* Create shaders */
    scene->fbufferShader = shader_init_source (ppu_vs_source, ppu_fs_source);
    scene->debugShader   = shader_init_source (ppu_vs_source, default_fs_source);

    /* Create main textures */
    texture_setup (&scene->fbufferTexture,   160, 144, GL_NEAREST, NULL);
    texture_setup (&scene->vramTexture,      128, 128, GL_NEAREST, NULL);
    texture_setup (&scene->nameTableTexture, 512, 480, GL_NEAREST, NULL);
    //texture_setup (&scene->paletteTexture, 64,  1, GL_NEAREST, pixels);

    test_pixeldata();

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDepthFunc(GL_LEQUAL);
}

void draw_scene (GLFWwindow * window, Scene * const scene)
{
	glClearColor((GLfloat)scene->bgColor[0] / 255, (GLfloat)scene->bgColor[1] / 255, (GLfloat)scene->bgColor[2] / 255, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int32_t width, height;
    glfwGetFramebufferSize (window, &width, &height);

    /* Render the PPU framebuffer here */
    glUseProgram(scene->debugShader.program);
    mat4x4 model;
    mat4x4 projection;

    mat4x4_ortho (projection, 0, width, 0, height, 0, 0.1f);
    mat4x4_identity (model);
    mat4x4_scale_aniso (model, model, width, height, 1.0f);

    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "model"),      1, GL_FALSE, (const GLfloat*) model);
    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "projection"), 1, GL_FALSE, (const GLfloat*) projection);

    glActiveTexture (GL_TEXTURE0);

    /* Draw framebuffer */
    glBindTexture (GL_TEXTURE_2D, scene->vramTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixelData);
	draw_lazy_quad(1.0f, 1.0f, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

#ifdef CPU_DEBUG
    draw_debug (window);
#endif

#ifdef PPU_DEBUG  
    draw_ntable_debug (window, scene);
#endif
}