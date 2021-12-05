#pragma once

#include "glrage_gl/Object.hpp"
#include "glrage_gl/gl_core_3_3.h"

#include <map>

namespace glrage {
namespace gl {

class Program : public Object {
public:
    Program();
    ~Program();
    void bind();
    void attachShader(GLenum type, const std::string &path);
    void link();
    void fragmentData(const std::string &name);
    GLint attributeLocation(const std::string &name);
    GLint uniformLocation(const std::string &name);

    void uniform3f(const std::string &name, GLfloat v0, GLfloat v1, GLfloat v2);
    void uniform4f(
        const std::string &name, GLfloat v0, GLfloat v1, GLfloat v2,
        GLfloat v3);
    void uniform1i(const std::string &name, GLint v0);
    void uniformMatrix4fv(
        const std::string &name, GLsizei count, GLboolean transpose,
        const GLfloat *value);

    std::string infoLog();

private:
    std::map<std::string, GLint> m_attributeLocations;
    std::map<std::string, GLint> m_uniformLocations;
};

}
}
