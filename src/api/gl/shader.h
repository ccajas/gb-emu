#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE

typedef struct Shader_struct
{
    int32_t program;
    int32_t *locations;
}
Shader;

Shader shader_init_source (const char *codeVS, const char *codeFS);
GLuint shader_build       (const GLchar *programSrc, const GLenum type, GLuint program);
uint32_t shader_link      (const int32_t vertexShader, const int32_t fragmentShader);

void shader_apply_direct (Shader const *shader, uint32_t location, void *value);

static inline int32_t shader_get_location (Shader const *shader, const char *uniformName)
{
    return glGetUniformLocation(shader->program, uniformName);
}

static inline int is_empty(const char* string)
{
   return (string[0] == '\0');
}

#endif