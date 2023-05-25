#ifndef SHADER_H
#define SHADER_H

//#include "../utils/defs.h"
#include "../../utils/fileread.h"
#include "gl_gen.h"

typedef struct Shader_struct
{
    int32_t program;
    int32_t *locations;
}
Shader;

void shader_apply     (Shader const *shader, const char *uniformName, const void * param);
void shader_apply_1f  (Shader const *shader, const char *uniformName, const float value);
void shader_apply_3f  (Shader const *shader, const char *uniformName, const void * param);
void shader_apply_int (Shader const *shader, const char *uniformName, const uint32_t value);

Shader shader_init        (const char *filenameVS, const char *filenameFS);
Shader shader_init_source (const char *codeVS, const char *codeFS);
GLuint shader_build       (const GLchar *programSrc, const GLenum type, GLuint program);
uint32_t shader_link      (const int32_t vertexShader, const int32_t fragmentShader);

void shader_apply_direct (Shader const *shader, uint32_t location, void *value);

inline int32_t shader_get_location (Shader const *shader, const char *uniformName)
{
    return glGetUniformLocation(shader->program, uniformName);
}

inline int is_empty(const char* string)
{
   return (string[0] == '\0');
}

#endif