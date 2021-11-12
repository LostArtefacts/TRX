#pragma once

#include "gl_core_3_3.h"

namespace glrage {
namespace gl {

class Utils
{
public:
    static const char* getErrorString(GLenum);
    static void checkError(char*);
};

} // namespace gl
} // namespace glrage