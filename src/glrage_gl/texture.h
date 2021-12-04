#pragma once

#include "glrage_gl/gl_core_3_3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GLRage_GLTexture {
    GLuint id;
    GLenum target;
} GLRage_GLTexture;

void GLRage_GLTexture_Init(GLRage_GLTexture *texture, GLenum target);
void GLRage_GLTexture_Close(GLRage_GLTexture *texture);
void GLRage_GLTexture_Bind(GLRage_GLTexture *texture);
GLenum GLRage_GLTexture_Target(GLRage_GLTexture *texture);

#ifdef __cplusplus
}
#endif
