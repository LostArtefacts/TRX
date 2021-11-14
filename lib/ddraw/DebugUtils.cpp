#include "DebugUtils.hpp"

#include <glrage_util/ErrorUtils.hpp>
#include <glrage_util/Logger.hpp>
#include <glrage_util/StringUtils.hpp>

#include <cstdint>
#include <exception>
#include <fstream>
#include <vector>

namespace glrage {
namespace ddraw {

void DebugUtils::dumpInfo(DDSURFACEDESC& desc)
{
#ifdef DEBUG_LOG
    LOG_INFO("start");
    LOG_INFO("  desc.dwWidth = %d", desc.dwWidth);
    LOG_INFO("  desc.dwHeight = %d", desc.dwHeight);
    LOG_INFO("  desc.ddpfPixelFormat.dwRGBBitCount = %d",
        desc.ddpfPixelFormat.dwRGBBitCount);

    LOG_INFO("  desc.dwFlags = %d", desc.dwFlags);
    if (desc.dwFlags & DDSD_CAPS)
        LOG_INFO("    DDSD_CAPS");
    if (desc.dwFlags & DDSD_HEIGHT)
        LOG_INFO("    DDSD_HEIGHT");
    if (desc.dwFlags & DDSD_WIDTH)
        LOG_INFO("    DDSD_WIDTH");
    if (desc.dwFlags & DDSD_PITCH)
        LOG_INFO("    DDSD_PITCH");
    if (desc.dwFlags & DDSD_BACKBUFFERCOUNT)
        LOG_INFO("    DDSD_BACKBUFFERCOUNT");
    if (desc.dwFlags & DDSD_ZBUFFERBITDEPTH)
        LOG_INFO("    DDSD_ZBUFFERBITDEPTH");
    if (desc.dwFlags & DDSD_ALPHABITDEPTH)
        LOG_INFO("    DDSD_ALPHABITDEPTH");
    if (desc.dwFlags & DDSD_LPSURFACE)
        LOG_INFO("    DDSD_LPSURFACE");
    if (desc.dwFlags & DDSD_PIXELFORMAT)
        LOG_INFO("    DDSD_PIXELFORMAT");
    if (desc.dwFlags & DDSD_CKDESTOVERLAY)
        LOG_INFO("    DDSD_CKDESTOVERLAY");
    if (desc.dwFlags & DDSD_CKDESTBLT)
        LOG_INFO("    DDSD_CKDESTBLT");
    if (desc.dwFlags & DDSD_CKSRCOVERLAY)
        LOG_INFO("    DDSD_CKSRCOVERLAY");
    if (desc.dwFlags & DDSD_CKSRCBLT)
        LOG_INFO("    DDSD_CKSRCBLT");
    if (desc.dwFlags & DDSD_MIPMAPCOUNT)
        LOG_INFO("    DDSD_MIPMAPCOUNT");
    if (desc.dwFlags & DDSD_REFRESHRATE)
        LOG_INFO("    DDSD_REFRESHRATE");
    if (desc.dwFlags & DDSD_LINEARSIZE)
        LOG_INFO("    DDSD_LINEARSIZE");
    if (desc.dwFlags & DDSD_TEXTURESTAGE)
        LOG_INFO("    DDSD_TEXTURESTAGE");
    if (desc.dwFlags & DDSD_FVF)
        LOG_INFO("    DDSD_FVF");
    if (desc.dwFlags & DDSD_SRCVBHANDLE)
        LOG_INFO("    DDSD_SRCVBHANDLE");
    if (desc.dwFlags & DDSD_DEPTH)
        LOG_INFO("    DDSD_DEPTH");

    LOG_INFO("  desc.ddsCaps.dwCaps = %d", desc.ddsCaps.dwCaps);
    if (desc.ddsCaps.dwCaps & DDSCAPS_RESERVED1)
        LOG_INFO("    DDSCAPS_RESERVED1");
    if (desc.ddsCaps.dwCaps & DDSCAPS_ALPHA)
        LOG_INFO("    DDSCAPS_ALPHA");
    if (desc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)
        LOG_INFO("    DDSCAPS_BACKBUFFER");
    if (desc.ddsCaps.dwCaps & DDSCAPS_COMPLEX)
        LOG_INFO("    DDSCAPS_COMPLEX");
    if (desc.ddsCaps.dwCaps & DDSCAPS_FLIP)
        LOG_INFO("    DDSCAPS_FLIP");
    if (desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
        LOG_INFO("    DDSCAPS_FRONTBUFFER");
    if (desc.ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)
        LOG_INFO("    DDSCAPS_OFFSCREENPLAIN");
    if (desc.ddsCaps.dwCaps & DDSCAPS_OVERLAY)
        LOG_INFO("    DDSCAPS_OVERLAY");
    if (desc.ddsCaps.dwCaps & DDSCAPS_PALETTE)
        LOG_INFO("    DDSCAPS_PALETTE");
    if (desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        LOG_INFO("    DDSCAPS_PRIMARYSURFACE");
    if (desc.ddsCaps.dwCaps & DDSCAPS_RESERVED3)
        LOG_INFO("    DDSCAPS_RESERVED3");
    if (desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACELEFT)
        LOG_INFO("    DDSCAPS_PRIMARYSURFACELEFT");
    if (desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        LOG_INFO("    DDSCAPS_SYSTEMMEMORY");
    if (desc.ddsCaps.dwCaps & DDSCAPS_TEXTURE)
        LOG_INFO("    DDSCAPS_TEXTURE");
    if (desc.ddsCaps.dwCaps & DDSCAPS_3DDEVICE)
        LOG_INFO("    DDSCAPS_3DDEVICE");
    if (desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
        LOG_INFO("    DDSCAPS_VIDEOMEMORY");
    if (desc.ddsCaps.dwCaps & DDSCAPS_VISIBLE)
        LOG_INFO("    DDSCAPS_VISIBLE");
    if (desc.ddsCaps.dwCaps & DDSCAPS_WRITEONLY)
        LOG_INFO("    DDSCAPS_WRITEONLY");
    if (desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
        LOG_INFO("    DDSCAPS_ZBUFFER");
    if (desc.ddsCaps.dwCaps & DDSCAPS_OWNDC)
        LOG_INFO("    DDSCAPS_OWNDC");
    if (desc.ddsCaps.dwCaps & DDSCAPS_LIVEVIDEO)
        LOG_INFO("    DDSCAPS_LIVEVIDEO");
    if (desc.ddsCaps.dwCaps & DDSCAPS_HWCODEC)
        LOG_INFO("    DDSCAPS_HWCODEC");
    if (desc.ddsCaps.dwCaps & DDSCAPS_MODEX)
        LOG_INFO("    DDSCAPS_MODEX");
    if (desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
        LOG_INFO("    DDSCAPS_MIPMAP");
    if (desc.ddsCaps.dwCaps & DDSCAPS_RESERVED2)
        LOG_INFO("    DDSCAPS_RESERVED2");
    if (desc.ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD)
        LOG_INFO("    DDSCAPS_ALLOCONLOAD");
    if (desc.ddsCaps.dwCaps & DDSCAPS_VIDEOPORT)
        LOG_INFO("    DDSCAPS_VIDEOPORT");
    if (desc.ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM)
        LOG_INFO("    DDSCAPS_LOCALVIDMEM");
    if (desc.ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
        LOG_INFO("    DDSCAPS_NONLOCALVIDMEM");
    if (desc.ddsCaps.dwCaps & DDSCAPS_STANDARDVGAMODE)
        LOG_INFO("    DDSCAPS_STANDARDVGAMODE");
    if (desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        LOG_INFO("    DDSCAPS_OPTIMIZED");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_RESERVED4) LOG_INFO(" DDSCAPS2_RESERVED4");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_HARDWAREDEINTERLACE) LOG_INFO("
    // DDSCAPS2_HARDWAREDEINTERLACE");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_HINTDYNAMIC) LOG_INFO("
    // DDSCAPS2_HINTDYNAMIC");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_HINTSTATIC) LOG_INFO("
    // DDSCAPS2_HINTSTATIC");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_TEXTUREMANAGE) LOG_INFO("
    // DDSCAPS2_TEXTUREMANAGE");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_RESERVED1) LOG_INFO(" DDSCAPS2_RESERVED1");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_RESERVED2) LOG_INFO(" DDSCAPS2_RESERVED2");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_OPAQUE) LOG_INFO("    DDSCAPS2_OPAQUE");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_HINTANTIALIASING) LOG_INFO("
    // DDSCAPS2_HINTANTIALIASING");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_CUBEMAP) LOG_INFO("    DDSCAPS2_CUBEMAP");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_CUBEMAP_POSITIVEX) LOG_INFO("
    // DDSCAPS2_CUBEMAP_POSITIVEX");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_CUBEMAP_NEGATIVEX) LOG_INFO("
    // DDSCAPS2_CUBEMAP_NEGATIVEX");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_CUBEMAP_POSITIVEY) LOG_INFO("
    // DDSCAPS2_CUBEMAP_POSITIVEY");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_CUBEMAP_NEGATIVEY) LOG_INFO("
    // DDSCAPS2_CUBEMAP_NEGATIVEY");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_CUBEMAP_POSITIVEZ) LOG_INFO("
    // DDSCAPS2_CUBEMAP_POSITIVEZ");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_CUBEMAP_NEGATIVEZ) LOG_INFO("
    // DDSCAPS2_CUBEMAP_NEGATIVEZ");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_MIPMAPSUBLEVEL) LOG_INFO("
    // DDSCAPS2_MIPMAPSUBLEVEL");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_D3DTEXTUREMANAGE) LOG_INFO("
    // DDSCAPS2_D3DTEXTUREMANAGE");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_DONOTPERSIST) LOG_INFO("
    // DDSCAPS2_DONOTPERSIST");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_STEREOSURFACELEFT) LOG_INFO("
    // DDSCAPS2_STEREOSURFACELEFT");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_VOLUME) LOG_INFO("    DDSCAPS2_VOLUME");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_NOTUSERLOCKABLE) LOG_INFO("
    // DDSCAPS2_NOTUSERLOCKABLE");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_POINTS) LOG_INFO("    DDSCAPS2_POINTS");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_RTPATCHES) LOG_INFO(" DDSCAPS2_RTPATCHES");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_NPATCHES) LOG_INFO(" DDSCAPS2_NPATCHES");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_RESERVED3) LOG_INFO(" DDSCAPS2_RESERVED3");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_DISCARDBACKBUFFER) LOG_INFO("
    // DDSCAPS2_DISCARDBACKBUFFER");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_ENABLEALPHACHANNEL) LOG_INFO("
    // DDSCAPS2_ENABLEALPHACHANNEL");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_EXTENDEDFORMATPRIMARY) LOG_INFO("
    // DDSCAPS2_EXTENDEDFORMATPRIMARY");
    // if (desc.ddsCaps.dwCaps & DDSCAPS2_ADDITIONALPRIMARY) LOG_INFO("
    // DDSCAPS2_ADDITIONALPRIMARY");
    LOG_INFO("DebugUtils::dumpInfo end");
#endif
}

void DebugUtils::dumpBuffer(
    DDSURFACEDESC& desc, void* buffer, const std::string& path)
{
    uint32_t imageSize = desc.dwWidth * desc.dwHeight;
    uint32_t dataSize = imageSize * 3;
    uint32_t headerSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    auto src = reinterpret_cast<uint16_t*>(buffer);
    std::vector<uint8_t> dst(dataSize);

    for (uint32_t i = 0; i < imageSize; i++) {
        dst[i * 3 + 0] = (src[i] >> 0 & 0x1f) * 0xff / 0x1f;
        dst[i * 3 + 1] = (src[i] >> 5 & 0x1f) * 0xff / 0x1f;
        dst[i * 3 + 2] = (src[i] >> 10 & 0x1f) * 0xff / 0x1f;
    }

    BITMAPFILEHEADER fh;
    fh.bfType = 0x4d42;
    fh.bfSize = dataSize + headerSize;
    fh.bfReserved1 = 0;
    fh.bfReserved2 = 0;
    fh.bfOffBits = headerSize;

    BITMAPINFOHEADER ih;
    ih.biSize = sizeof(BITMAPINFOHEADER);
    ih.biWidth = desc.dwWidth;
    ih.biHeight = desc.dwHeight;
    ih.biPlanes = 1;
    ih.biBitCount = 24;
    ih.biCompression = 0;
    ih.biSizeImage = 0;
    ih.biXPelsPerMeter = 2835;
    ih.biYPelsPerMeter = 2835;
    ih.biClrUsed = 0;
    ih.biClrImportant = 0;

    std::ofstream file(path, std::ofstream::binary);
    if (!file.good()) {
        throw std::runtime_error("Can't open file '" +
                                 path + "': " +
                                 ErrorUtils::getSystemErrorString());
    }

    file.write(reinterpret_cast<char*>(dst[0]), dataSize);
    file.close();
}

std::string DebugUtils::getSurfaceName(DDSURFACEDESC& desc)
{
    if (desc.dwFlags & DDSCAPS_PRIMARYSURFACE) {
        return "Primary";
    } else if (desc.dwFlags & DDSCAPS_FRONTBUFFER) {
        return "Front";
    } else if (desc.dwFlags & DDSCAPS_BACKBUFFER) {
        return "Back";
    } else {
        return "Other";
    }
}

} // namespace ddraw
} // namespace glrage
