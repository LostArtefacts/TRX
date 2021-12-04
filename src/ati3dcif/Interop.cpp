#include "ati3dcif/Interop.hpp"

#include "ati3dcif/ATI3DCIF.h"
#include "ati3dcif/Error.hpp"
#include "ati3dcif/Renderer.hpp"
#include "glrage/Context.hpp"
#include "glrage_util/ErrorUtils.hpp"
#include "log.h"

#include <memory>
#include <stdexcept>

namespace glrage {
namespace cif {

static Context &context = Context::instance();
static std::unique_ptr<Renderer> renderer = NULL;
static bool contextCreated = false;

C3D_EC HandleException()
{
    try {
        throw;
    } catch (const Error &ex) {
        LOG_ERROR("CIF error: %s (0x%lx)", ex.what(), ex.getErrorCode());
        return ex.getErrorCode();
    } catch (const std::runtime_error &ex) {
        ErrorUtils::warning(ex);
        return C3D_EC_GENFAIL;
    } catch (const std::logic_error &ex) {
        ErrorUtils::warning(ex);
        return C3D_EC_GENFAIL;
    }
}

extern "C" {

C3D_EC ATI3DCIF_Term(void)
{
    try {
        if (renderer) {
            renderer.reset();
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

C3D_EC ATI3DCIF_Init(void)
{
    // do some cleanup in case the app forgets to call ATI3DCIF_Term
    if (renderer) {
        LOG_ERROR("Previous instance was not terminated by ATI3DCIF_Term!");
        ATI3DCIF_Term();
    }

    try {
        renderer = std::make_unique<Renderer>();
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC ATI3DCIF_TextureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap)
{
    if (!renderer) {
        return C3D_EC_BADSTATE;
    }

    try {
        renderer->textureReg(ptmapToReg, phtmap);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC ATI3DCIF_TextureUnreg(C3D_HTX htxToUnreg)
{
    if (!renderer) {
        return C3D_EC_BADSTATE;
    }

    try {
        renderer->textureUnreg(htxToUnreg);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC ATI3DCIF_TexturePaletteCreate(
    C3D_ECI_TMAP_TYPE epalette, void *pPalette, C3D_PHTXPAL phtpalCreated)
{
    if (!renderer) {
        return C3D_EC_BADSTATE;
    }

    try {
        renderer->texturePaletteCreate(epalette, pPalette, phtpalCreated);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC ATI3DCIF_TexturePaletteDestroy(C3D_HTXPAL htxpalToDestroy)
{
    if (!renderer) {
        return C3D_EC_BADSTATE;
    }

    try {
        renderer->texturePaletteDestroy(htxpalToDestroy);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC ATI3DCIF_SetState(C3D_ERSID eRStateID, C3D_PRSDATA pRStateData)
{
    if (!renderer) {
        return C3D_EC_BADSTATE;
    }

    try {
        renderer->setState(eRStateID, pRStateData);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC ATI3DCIF_RenderBegin(void)
{
    if (!renderer) {
        return C3D_EC_BADSTATE;
    }

    try {
        renderer->renderBegin();
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC ATI3DCIF_RenderEnd(void)
{
    if (!renderer) {
        return C3D_EC_BADSTATE;
    }

    try {
        renderer->renderEnd();
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC ATI3DCIF_RenderPrimStrip(C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert)
{
    if (!renderer) {
        return C3D_EC_BADSTATE;
    }

    try {
        renderer->renderPrimStrip(vStrip, u32NumVert);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}

C3D_EC ATI3DCIF_RenderPrimList(C3D_VLIST vList, C3D_UINT32 u32NumVert)
{
    if (!renderer) {
        return C3D_EC_BADSTATE;
    }

    try {
        renderer->renderPrimList(vList, u32NumVert);
    } catch (...) {
        return HandleException();
    }

    return C3D_EC_OK;
}
}

}
}
