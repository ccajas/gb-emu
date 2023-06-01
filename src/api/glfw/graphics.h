#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "utils/linmath.h"
#include "gl_gen.h"
#include "shader.h"

struct Texture
{
    uint16_t width;
    uint16_t height;

    /* Pixel data */
    uint8_t * data;
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
}
Scene;

extern const char * default_fs_source;
extern const char * ppu_vs_source;
extern const char * ppu_fs_source;

extern uint32_t quadVAO[2];

void graphics_init  (Scene * const);
void draw_lazy_quad (const float width, const float height, const int i);

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

static inline void draw_begin (GLFWwindow * window, Scene * const scene)
{
	glClearColor((GLfloat)scene->bgColor[0] / 255, (GLfloat)scene->bgColor[1] / 255, (GLfloat)scene->bgColor[2] / 255, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static inline void draw_quad (GLFWwindow * window, Scene * const scene, struct Texture * const pixels, 
    const int xpos, const int ypos, const float scale)
{
    int32_t width, height;
    glfwGetFramebufferSize (window, &width, &height);

    /* Setup matrix and send data to shader */
    glUseProgram(scene->debugShader.program);
    mat4x4 model;
    mat4x4 projection;

    mat4x4_ortho (projection, 0, width, 0, height, 0, 0.1f);
    mat4x4_identity (model);

    /* Translated as if the top left corner is x:0 y:0 */
    mat4x4_translate (model, xpos, height - (pixels->height * scale) - ypos, 0);
    mat4x4_scale_aniso (model, model, pixels->width * scale, pixels->height * scale, 1.0f);

    glUniform2f (glGetUniformLocation(scene->debugShader.program, "screenSize"), (GLfloat) (width / scale), (GLfloat) (height / scale));
    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "model"),      1, GL_FALSE, (const GLfloat*) model);
    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "projection"), 1, GL_FALSE, (const GLfloat*) projection);

    glActiveTexture (GL_TEXTURE0);

    /* Draw quad */
    glBindTexture (GL_TEXTURE_2D, scene->debugTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, pixels->width, pixels->height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels->data);
	draw_lazy_quad(1.0f, 1.0f, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

static inline void draw_screen_quad (GLFWwindow * window, Scene * const scene, struct Texture * const pixels, const float scale)
{
    draw_quad (window, scene, pixels, 0, 0, scale);
}

#endif