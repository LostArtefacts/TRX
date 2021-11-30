#include "glrage_gl/Sampler.hpp"

namespace glrage {
namespace gl {

Sampler::Sampler()
{
    glGenSamplers(1, &m_id);
}

Sampler::~Sampler()
{
    glDeleteSamplers(1, &m_id);
}

void Sampler::bind()
{
}

void Sampler::bind(GLuint unit)
{
    glBindSampler(unit, m_id);
}

void Sampler::parameteri(GLenum pname, GLint param)
{
    glSamplerParameteri(m_id, pname, param);
}

void Sampler::parameterf(GLenum pname, GLfloat param)
{
    glSamplerParameterf(m_id, pname, param);
}

}
}
