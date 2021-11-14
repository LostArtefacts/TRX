#pragma once

// #include <atlimage.h>
#include "ddraw.hpp"

#include <glrage/GLRage.hpp>
#include <glrage_gl/Buffer.hpp>
#include <glrage_gl/Program.hpp>
#include <glrage_gl/Sampler.hpp>
#include <glrage_gl/Texture.hpp>
#include <glrage_gl/VertexArray.hpp>
#include <glrage_util/Config.hpp>

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
    static const GLenum TEX_INTERNAL_FORMAT = GL_RGBA;
    static const GLenum TEX_FORMAT = GL_BGRA;
    static const GLenum TEX_TYPE = GL_UNSIGNED_SHORT_1_5_5_5_REV;
    static const int32_t TITLE_WIDTH = 640;
    static const int32_t TITLE_HEIGHT = 480;


    Context& m_context{GLRage::getContext()};
    Config& m_config{GLRage::getConfig()};
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    gl::VertexArray m_surfaceFormat;
    gl::Buffer m_surfaceBuffer;
    gl::Texture m_surfaceTexture = GL_TEXTURE_2D;
    gl::Sampler m_sampler;
    gl::Program m_program;
    // CImage m_overrideImage;
};

} // namespace ddraw
} // namespace glrage
