#pragma once

#include "ddraw/ddraw.h"

#include "gfx/gl/buffer.h"
#include "gfx/gl/program.h"
#include "gfx/gl/sampler.h"
#include "gfx/gl/texture.h"
#include "gfx/gl/vertex_array.h"
#include "glrage/Context.hpp"

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
    GFX_GL_VertexArray m_surfaceFormat;
    GFX_GL_Buffer m_surfaceBuffer;
    GFX_GL_Texture m_surfaceTexture;
    GFX_GL_Sampler m_sampler;
    GFX_GL_Program m_program;
};

}
}
