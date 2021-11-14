#include "Texture.hpp"
#include "Error.hpp"
#include "Utils.hpp"
#include <windows.h>

#include <glrage_gl/Utils.hpp>
#include <glrage_util/Logger.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <vector>

// #include "openssl/md5.h"
#include <glrage_util/StringUtils.hpp>

#undef max

namespace glrage {
namespace cif {

Texture::Texture()
    : gl::Texture(GL_TEXTURE_2D)
{}

Texture::~Texture()
{}

std::map<std::string, std::string>& Texture::getTextureKeys()
{
    static bool textureKeysRead = false;
    static std::map<std::string, std::string> textureKeys;

    while (!textureKeysRead) {
        textureKeysRead = true;
        std::string texDir = m_config.getString("patch.texture_directory", "");
        if (texDir.length() == 0)
            break;
        std::ifstream keyfile(texDir + "\\keys.txt");
        std::string line;
        while (std::getline(keyfile, line)) {
            std::vector<std::string> words;
            std::istringstream iss(line);
            std::string word;
            while (std::getline(iss, word, ' '))
                words.push_back(word);

            if (words.size() == 2)
                textureKeys[words[0]] = words[1];
        }
        keyfile.close();
    }

    return textureKeys;
}

void Texture::load(C3D_PTMAP tmap, std::vector<C3D_PALETTENTRY>& palette)
{
    m_chromaKey = tmap->clrTexChromaKey;
    m_keyOnAlpha = false;
    m_is_translucent = false;

    // convert and generate texture for each level
    uint32_t width = 1 << tmap->u32MaxMapXSizeLg2;
    uint32_t height = 1 << tmap->u32MaxMapYSizeLg2;
    uint32_t size = width * height;

    uint32_t levels = 1;
    if (tmap->bMipMap) {
        levels = std::max(tmap->u32MaxMapXSizeLg2, tmap->u32MaxMapYSizeLg2) + 1;
    }

    for (uint32_t level = 0; level < levels; level++) {
        LOG_INFO("level %d (%dx%d)", level, width, height);

        // convert texture data
        switch (tmap->eTexFormat) {
            case C3D_ETF_RGB1555: {
                uint16_t* src = static_cast<uint16_t*>(tmap->apvLevels[level]);

                // toggle alpha bit, which has the opposite meaning in OpenGL
                for (uint32_t i = 0; i < size; i++) {
                    src[i] ^= 1 << 15;
                }

                // upload texture data
                glTexImage2D(GL_TEXTURE_2D,
                    level,
                    GL_RGBA,
                    width,
                    height,
                    0,
                    GL_BGRA,
                    GL_UNSIGNED_SHORT_1_5_5_5_REV,
                    tmap->apvLevels[level]);

                break;
            }

            case C3D_ETF_RGB332: {
                glTexImage2D(GL_TEXTURE_2D,
                    level,
                    GL_RGB,
                    width,
                    height,
                    0,
                    GL_RGB,
                    GL_UNSIGNED_BYTE_3_3_2,
                    tmap->apvLevels[level]);
                break;
            }

            case C3D_ETF_RGB565: {
                glTexImage2D(GL_TEXTURE_2D,
                    level,
                    GL_RGB,
                    width,
                    height,
                    0,
                    GL_RGB,
                    GL_UNSIGNED_SHORT_5_6_5_REV,
                    tmap->apvLevels[level]);
                break;
            }

            case C3D_ETF_RGB4444: {
                glTexImage2D(GL_TEXTURE_2D,
                    level,
                    GL_RGBA,
                    width,
                    height,
                    0,
                    GL_BGRA,
                    GL_UNSIGNED_SHORT_4_4_4_4_REV,
                    tmap->apvLevels[level]);
                break;
            }

            case C3D_ETF_CI8: {
                uint8_t* src = static_cast<uint8_t*>(tmap->apvLevels[level]);

                // unsigned char md5sum[16];
                // MD5(&src[0], size, md5sum);
                // std::string hex;
                // for (int i = 0; i < 16; i++)
                //     hex += StringUtils::format("%02X", md5sum[i]);

                // std::map<std::string, std::string>& keys = getTextureKeys();
                std::vector<uint8_t> dst;
                // bool override = false;
                // CImage image;
                // if (level == 0 && keys.find(hex) != keys.end())
                // {
                //     std::string fileName =
                //     m_config.getString("patch.texture_directory", "") + "\\"
                //     + keys[hex]; std::string wFileName(fileName.begin(),
                //     fileName.end()); if
                //     (SUCCEEDED(image.Load(wFileName.c_str())))
                //         override = true;
                //     else
                //         LOG_INFO("Missing texture file: %s",
                //         fileName.c_str());
                // }

                // if (override)
                // {
                //     // This texture has a replacement image on disc.
                //     // Use it for all levels, even if mimmaps are available
                //     levels = 1;
                //     m_keyOnAlpha = true;
                //     width = image.GetWidth();
                //     height = image.GetHeight();
                //     bool use_map = (width == TRANS_TEX_DIM && height ==
                //     TRANS_TEX_DIM); if (use_map)
                //         m_translucency_map.assign(TRANS_MAP_DIM *
                //         TRANS_MAP_DIM, 0);
                //     int pitch = image.GetPitch();
                //     uint8_t *bits = reinterpret_cast<uint8_t
                //     *>(image.GetBits()); dst.resize(abs(pitch) * height);
                //     uint8_t *dstp = &dst[0];
                //     for (size_t y = 0; y < height; y++)
                //     {
                //         for (size_t x = 0; x < width; x++)
                //         {
                //             if (use_map && bits[4 * x + 3] != 255)
                //             {
                //                 uint32_t map_x = x / TRANS_MAP_FACTOR;
                //                 uint32_t map_y = y / TRANS_MAP_FACTOR;
                //                 m_translucency_map[map_y * TRANS_MAP_DIM +
                //                 map_x] = 1; if (bits[4 * x + 3] != 0)
                //                     m_is_translucent = true;
                //             }
                //             dstp[4 * x + 0] = bits[4 * x + 2];
                //             dstp[4 * x + 1] = bits[4 * x + 1];
                //             dstp[4 * x + 2] = bits[4 * x + 0];
                //             dstp[4 * x + 3] = bits[4 * x + 3];
                //         }
                //         bits += pitch;
                //         dstp += abs(pitch);
                //     }
                // }
                // else
                {
                    // Resolve indices to RGBA, which requires less code and is
                    // faster than texture palettes in shaders.
                    // Modern hardware really doesn't care about a few KB more
                    // or less per texture anyway.
                    dst.resize(size * 4);
                    for (uint32_t i = 0; i < size; i++) {
                        C3D_PALETTENTRY c = palette[src[i]];
                        dst[i * 4 + 0] = c.r;
                        dst[i * 4 + 1] = c.g;
                        dst[i * 4 + 2] = c.b;
                        dst[i * 4 + 3] = 0xff;
                    }
                }

                // upload texture data
                glTexImage2D(GL_TEXTURE_2D,
                    level,
                    GL_RGBA,
                    width,
                    height,
                    0,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    &dst[0]);

                break;
            }

            default:
                throw Error(
                    "Unsupported texture format: " +
                        std::string(C3D_ETEXFMT_NAMES[tmap->eTexFormat]),
                    C3D_EC_NOTIMPYET);
        }

        // set dimensions for next level
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
        size = width * height;
    }

    // generate mipmaps automatically if the application doesn't provide any
    if (levels == 1) {
        bind();
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    // FIXME: sampler object overrides these parameters
    // if (tmap->u32Size > 68) {
    //    bind();

    //    // set static texture parameters
    //    if (tmap->bClampS) {
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
    //        GL_CLAMP_TO_EDGE);
    //    }

    //    if (tmap->bClampT) {
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
    //        GL_CLAMP_TO_EDGE);
    //    }
    //}

    gl::Utils::checkError(__FUNCTION__);
}

C3D_COLOR& Texture::chromaKey()
{
    return m_chromaKey;
}

bool Texture::keyOnAlpha()
{
    return m_keyOnAlpha;
}

bool Texture::isTranslucent()
{
    return m_is_translucent;
}

std::vector<uint8_t>& Texture::translucencyMap()
{
    return m_translucency_map;
}

} // namespace cif
} // namespace glrage
