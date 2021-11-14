#include "Error.hpp"
#include "Renderer.hpp"
#include "Utils.hpp"

#include <ati3dcif/ATI3DCIF.h>
#include <glrage/GLRage.hpp>
#include <glrage_util/ErrorUtils.hpp>
#include <glrage_util/Logger.hpp>

#include <stdexcept>
#include <memory>

namespace glrage {
namespace cif {

static Context& context = GLRage::getContext();
static std::unique_ptr<Renderer> renderer;
static bool contextCreated = false;

C3D_EC
HandleException()
{
    try {
        throw;
    } catch (const Error& ex) {
#ifdef _DEBUG
        ErrorUtils::warning(ex);
#endif
        LOG_INFO("CIF error: %s (0x%x %s)", ex.what(), ex.getErrorCode(),
            ex.getErrorName());
        return ex.getErrorCode();
    } catch (const std::runtime_error& ex) {
        ErrorUtils::warning(ex);
        return C3D_EC_GENFAIL;
    } catch (const std::logic_error& ex) {
        ErrorUtils::warning(ex);
        return C3D_EC_GENFAIL;
    }
}

extern "C" {

C3D_EC WINAPI ATI3DCIF_Term(void)
{
    LOG_TRACE("");

    try {
        if (renderer) {
            renderer.release();
        }
    } catch (...) {
        return HandleException();
    }

    // SDK PDF says "TRUE if successful, otherwise FALSE", but the
    // function uses C3D_EC as return value.
    // In other words: TRUE = C3D_EC_GENFAIL and FALSE = C3D_EC_OK? WTF...
    // Anyway, most apps don't seem to care about the return value
    // of this function, so stick with C3D_EC_OK for now.
    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_Init(void)
{
    LOG_TRACE("");

    context.init();
    context.attach();

    ErrorUtils::setHWnd(context.getHWnd());

    // do some cleanup in case the app forgets to call ATI3DCIF_Term
    if (renderer) {
        LOG_INFO("Previous instance was not terminated by ATI3DCIF_Term!");
        ATI3DCIF_Term();
    }

    try {
        renderer = std::make_unique<Renderer>();
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_GetInfo(PC3D_3DCIFINFO p3DCIFInfo)
{
    LOG_TRACE("");

    // check for invalid struct
    if (!p3DCIFInfo || p3DCIFInfo->u32Size > 48) {
        return C3D_EC_BADPARAM;
    }

    // values from an ATI Xpert 98 with a 3D Rage Pro AGP 2x

    // Host pointer to frame buffer base (TODO: allocate memory?)
    p3DCIFInfo->u32FrameBuffBase = 0;
    // Host pointer to offscreen heap (TODO: allocate memory?)
    p3DCIFInfo->u32OffScreenHeap = 0;
    p3DCIFInfo->u32OffScreenSize = 0x4fe800; // Size of offscreen heap
    p3DCIFInfo->u32TotalRAM = 8 << 20;       // Total amount of RAM on the card
    p3DCIFInfo->u32ASICID = 0x409;           // ASIC Id. code
    p3DCIFInfo->u32ASICRevision = 0x47ff;    // ASIC revision

    // older CIF versions don't have CIF caps, so check the size first
    if (p3DCIFInfo->u32Size == 48) {
        // note: 0x400 and 0x800 are reported in 4.10.2690 and later but are not
        // defined in ATI3DCIF.H
        p3DCIFInfo->u32CIFCaps1 = C3D_CAPS1_FOG | C3D_CAPS1_POINT |
                                  C3D_CAPS1_RECT | C3D_CAPS1_Z_BUFFER |
                                  C3D_CAPS1_CI4_TMAP | C3D_CAPS1_CI8_TMAP |
                                  C3D_CAPS1_DITHER_EN | C3D_CAPS1_ENH_PERSP |
                                  C3D_CAPS1_SCISSOR | 0x400 | 0x800;
        p3DCIFInfo->u32CIFCaps2 = C3D_CAPS2_TEXTURE_CLAMP |
                                  C3D_CAPS2_DESTINATION_ALPHA_BLEND |
                                  C3D_CAPS2_TEXTURE_TILING;

        // unused caps
        p3DCIFInfo->u32CIFCaps3 = 0;
        p3DCIFInfo->u32CIFCaps4 = 0;
        p3DCIFInfo->u32CIFCaps5 = 0;
    }

    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_TextureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap)
{
    LOG_TRACE("0x%p, 0x%p", *ptmapToReg, *phtmap);

    try {
        renderer->textureReg(ptmapToReg, phtmap);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_TextureUnreg(C3D_HTX htxToUnreg)
{
    LOG_TRACE("0x%p", htxToUnreg);

    try {
        renderer->textureUnreg(htxToUnreg);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_TexturePaletteCreate(C3D_ECI_TMAP_TYPE epalette, void* pPalette, C3D_PHTXPAL phtpalCreated)
{
    LOG_TRACE("%s, 0x%p, 0x%p", cif::C3D_ECI_TMAP_TYPE_NAMES[epalette],
        pPalette, phtpalCreated);

    try {
        renderer->texturePaletteCreate(epalette, pPalette, phtpalCreated);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_TexturePaletteDestroy(C3D_HTXPAL htxpalToDestroy)
{
    LOG_TRACE("0x%p", htxpalToDestroy);

    try {
        renderer->texturePaletteDestroy(htxpalToDestroy);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_HRC WINAPI ATI3DCIF_ContextCreate(void)
{
    LOG_TRACE("");

    context.attach();

    // can't create more than one context
    if (contextCreated) {
        return nullptr;
    }

    contextCreated = true;

    // According to ATI3DCIF.H, "only one context may be exist at a time",
    // so always returning 1 should be fine
    return (C3D_HRC)1;
}

C3D_EC WINAPI ATI3DCIF_ContextDestroy(C3D_HRC hRC)
{
    LOG_TRACE("0x%p", hRC);

    // can't destroy a context that wasn't created
    if (!contextCreated) {
        return C3D_EC_BADPARAM;
    }

    contextCreated = false;

    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_ContextSetState(C3D_HRC hRC, C3D_ERSID eRStateID, C3D_PRSDATA pRStateData)
{
#ifdef LOG_TRACE_ENABLED
    std::string stateDataStr =
        cif::Utils::dumpRenderStateData(eRStateID, pRStateData);
    LOG_TRACE("0x%p, %s, %s", hRC, cif::C3D_ERSID_NAMES[eRStateID],
        stateDataStr.c_str());
#endif

    try {
        renderer->setState(eRStateID, pRStateData);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_RenderBegin(C3D_HRC hRC)
{
    LOG_TRACE("0x%p", hRC);

    try {
        renderer->renderBegin(hRC);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_RenderEnd(void)
{
    LOG_TRACE("");

    try {
        renderer->renderEnd();
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_RenderPrimStrip(C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert)
{
    LOG_TRACE("0x%p, %d", vStrip, u32NumVert);

    try {
        renderer->renderPrimStrip(vStrip, u32NumVert);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC WINAPI ATI3DCIF_RenderPrimList(C3D_VLIST vList, C3D_UINT32 u32NumVert)
{
    LOG_TRACE("0x%p, %d", vList, u32NumVert);

    try {
        renderer->renderPrimList(vList, u32NumVert);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_Term_lib)(void) = ATI3DCIF_Term;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_Init_lib)(void) = ATI3DCIF_Init;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_GetInfo_lib)(PC3D_3DCIFINFO p3DCIFInfo) = ATI3DCIF_GetInfo;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_TextureReg_lib)(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap) = ATI3DCIF_TextureReg;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_TextureUnreg_lib)(C3D_HTX htxToUnreg) = ATI3DCIF_TextureUnreg;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_TexturePaletteCreate_lib)(C3D_ECI_TMAP_TYPE epalette, void* pPalette, C3D_PHTXPAL phtpalCreated) = ATI3DCIF_TexturePaletteCreate;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_TexturePaletteDestroy_lib)(C3D_HTXPAL htxpalToDestroy) = ATI3DCIF_TexturePaletteDestroy;
C3D_HRC __declspec(dllexport) (*WINAPI ATI3DCIF_ContextCreate_lib)(void) = ATI3DCIF_ContextCreate;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_ContextDestroy_lib)(C3D_HRC hRC) = ATI3DCIF_ContextDestroy;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_ContextSetState_lib)(C3D_HRC hRC, C3D_ERSID eRStateID, C3D_PRSDATA pRStateData) = ATI3DCIF_ContextSetState;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_RenderBegin_lib)(C3D_HRC hRC) = ATI3DCIF_RenderBegin;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_RenderEnd_lib)(void) = ATI3DCIF_RenderEnd;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_RenderPrimStrip_lib)(C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert) = ATI3DCIF_RenderPrimStrip;
C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_RenderPrimList_lib)(C3D_VLIST vList, C3D_UINT32 u32NumVert) = ATI3DCIF_RenderPrimList;

} // extern "C"

} // namespace cif
} // namespace glrage
