#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "utils/linmath.h"
#include "gl_gen.h"
#include "shader.h"

typedef struct Scene_struct
{
    uint8_t bgColor[3];

    GLuint 
        fbufferTexture, 
        vramTexture, 
        nameTableTexture,
        paletteTexture;
    Shader 
        fbufferShader,
        debugShader;
}
Scene;

extern const char * default_fs_source;
extern const char * ppu_vs_source;
extern const char * ppu_fs_source;

extern uint32_t quadVAO[2];

void graphics_init  (Scene * const);
void draw_lazy_quad (const float width, const float height, const int i);
void draw_scene     (GLFWwindow *, Scene * const);

inline void texture_setup (uint32_t * const textureID, uint16_t width, uint16_t height, GLenum filter, const void * data)
{
    glGenTextures (1, textureID);
    glBindTexture(GL_TEXTURE_2D, *textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
}

#endif