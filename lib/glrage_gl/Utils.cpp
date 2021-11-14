#include "Utils.hpp"
#include "gl_core_3_3.h"

#include <glrage_util/ErrorUtils.hpp>
#include <glrage_util/Logger.hpp>

namespace glrage {
namespace gl {

const char* Utils::getErrorString(GLenum err)
{
    switch (err) {
        case GL_NO_ERROR:
            return "GL_NO_ERROR";
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";
        case GL_STACK_UNDERFLOW:
            return "GL_STACK_UNDERFLOW";
        case GL_STACK_OVERFLOW:
            return "GL_STACK_OVERFLOW";
        default:
            return "UNKNOWN";
    }
}

void Utils::checkError(const char* section)
{
    for (GLenum err; (err = glGetError()) != GL_NO_ERROR;) {
#ifdef _DEBUG
        ErrorUtils::warning("glGetError", getErrorString(err));
#endif
        LOG_INFO("glGetError: %s (%s)", getErrorString(err), section);
    }
}

} // namespace gl
} // namespace glrage
