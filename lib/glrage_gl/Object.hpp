#pragma once

#include "gl_core_3_3.h"

namespace glrage {
namespace gl {

class Object
{
public:
    virtual ~Object(){};
    GLuint id()
    {
        return m_id;
    }
    virtual void bind() = 0;

protected:
    GLuint m_id = 0;
};

} // namespace gl
} // namespace glrage
