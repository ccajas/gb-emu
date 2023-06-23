#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "utils/linmath.h"
#include "gl_gen.h"
#include "shader.h"

struct Texture
{
    uint16_t width;
    uint16_t height;

    /* Pixel data and pointer */
    uint8_t 
        * imgData,
        * ptr;
};

typedef struct Scene_struct
{
    uint8_t bgColor[3];

    GLuint 
        fbufferTexture, 
        debugTexture;
    Shader
        fbufferShader,
        debugShader;

    GLuint * activeTexture;
    Shader * activeShader;
}
Scene;

extern const char * default_fs_source;
extern const char * ppu_vs_source;
extern const char * ppu_fs_source;

extern uint32_t quadVAO[2];

void graphics_init  (Scene * const);
void draw_lazy_quad (const float width, const float height, const int i);

static inline void texture_setup (uint32_t * const textureID, uint16_t width, uint16_t height, GLenum filter, const void * data)
{
    glGenTextures (1, textureID);
    glBindTexture(GL_TEXTURE_2D, *textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
}

static inline void set_shader (Scene * const scene, Shader * shader)
{
    scene->activeShader = shader;
}

static inline void set_texture (Scene * const scene, GLuint * texture)
{
    scene->activeTexture = texture;
}

static inline void draw_begin (GLFWwindow * window, Scene * const scene)
{
	glClearColor((GLfloat)scene->bgColor[0] / 255, (GLfloat)scene->bgColor[1] / 255, (GLfloat)scene->bgColor[2] / 255, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(scene->debugShader.program);
}

/* Draw a textured quad on the display */
void draw_quad (GLFWwindow * window, Scene * const scene, 
    struct Texture * const pixels, 
    const int xpos, const int ypos, const float scale);
void draw_screen_quad (GLFWwindow * window, Scene * const scene, struct Texture * const pixels, const float scale);

#endif