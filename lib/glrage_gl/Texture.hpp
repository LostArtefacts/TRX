#pragma once

#include "Object.hpp"
#include "gl_core_3_3.h"

namespace glrage {
namespace gl {

class Texture : public Object
{
public:
    Texture(GLenum target);
    ~Texture();
    void bind();
    GLenum target();

private:
    GLenum m_target;
};

} // namespace gl
} // namespace glrage
