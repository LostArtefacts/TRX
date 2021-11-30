#pragma once

#include "ddraw.hpp"

#include <glrage/GLRage.hpp>
#include <glrage_gl/Buffer.hpp>
#include <glrage_gl/Program.hpp>
#include <glrage_gl/Sampler.hpp>
#include <glrage_gl/Texture.hpp>
#include <glrage_gl/VertexArray.hpp>

#include <cstdint>
#include <vector>

namespace glrage {
namespace ddraw {

class Renderer
{
public:
    Renderer();
    void upload(DDSURFACEDESC& desc, std::vector<uint8_t>& data);
    void render();

private:
    Context& m_context{GLRage::getContext()};
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    gl::VertexArray m_surfaceFormat;
    gl::Buffer m_surfaceBuffer;
    gl::Texture m_surfaceTexture = GL_TEXTURE_2D;
    gl::Sampler m_sampler;
    gl::Program m_program;
};

} // namespace ddraw
} // namespace glrage
