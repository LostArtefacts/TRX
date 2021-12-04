#pragma once

#include "ddraw/ddraw.h"

#include "glrage/Context.hpp"
#include "glrage_gl/Program.hpp"
#include "glrage_gl/VertexArray.hpp"
#include "glrage_gl/buffer.h"
#include "glrage_gl/sampler.h"
#include "glrage_gl/texture.h"

#include <cstdint>
#include <vector>

namespace glrage {
namespace ddraw {

class Renderer {
public:
    Renderer();
    ~Renderer();

    void upload(DDSURFACEDESC &desc, std::vector<uint8_t> &data);
    void render();

private:
    Context &m_context = Context::instance();
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    gl::VertexArray m_surfaceFormat;
    GLRage_GLBuffer m_surfaceBuffer;
    GLRage_GLTexture m_surfaceTexture;
    GLRage_GLSampler m_sampler;
    gl::Program m_program;
};

}
}
