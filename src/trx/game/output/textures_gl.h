#pragma once

// The texture atlas handles, in the renderer's own terms. They sit apart from
// the rest of the texture module because naming a sprite needs no GL:
// everything that reads sprite geometry would otherwise pull the whole of GLEW
// in for the sake of these two.

#include <GL/glew.h>

GLuint Output_Textures_GetAtlasTexture(void);
GLuint Output_Textures_GetEnvMapTexture(void);
