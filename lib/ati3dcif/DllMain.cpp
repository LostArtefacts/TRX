#include "Error.hpp"
#include "Renderer.hpp"

#include <ati3dcif/ATI3DCIF.h>
#include <glrage/GLRage.hpp>
#include <glrage_util/ErrorUtils.hpp>
#include <glrage_util/Logger.hpp>

#include <memory>
#include <stdexcept>

namespace glrage {
namespace cif {

static Context& context = GLRage::getContext();
static std::unique_ptr<Renderer> renderer = NULL;
static bool contextCreated = false;

C3D_EC HandleException()
{
    try {
        throw;
    } catch (const Error& ex) {
        LOG_INFO("CIF error: %s (0x%x)", ex.what(), ex.getErrorCode());
        return ex.getErrorCode();
    } catch (const std::runtime_error& ex) {
        ErrorUtils::warning(ex);
        return C3D_EC_GENFAIL;
    } catch (const std::logic_error& ex) {
        ErrorUtils::warning(ex);
        return C3D_EC_GENFAIL;
    }
}

extern "C"
{
    C3D_EC WINAPI ATI3DCIF_Term(void)
    {
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

    C3D_EC WINAPI ATI3DCIF_TextureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap)
    {
        try {
            renderer->textureReg(ptmapToReg, phtmap);
        } catch (...) {
            return HandleException();
        }

        return C3D_EC_OK;
    }

    C3D_EC WINAPI ATI3DCIF_TextureUnreg(C3D_HTX htxToUnreg)
    {
        try {
            renderer->textureUnreg(htxToUnreg);
        } catch (...) {
            return HandleException();
        }

        return C3D_EC_OK;
    }

    C3D_EC WINAPI ATI3DCIF_TexturePaletteCreate(C3D_ECI_TMAP_TYPE epalette,
        void* pPalette,
        C3D_PHTXPAL phtpalCreated)
    {
        try {
            renderer->texturePaletteCreate(epalette, pPalette, phtpalCreated);
        } catch (...) {
            return HandleException();
        }

        return C3D_EC_OK;
    }

    C3D_EC WINAPI ATI3DCIF_TexturePaletteDestroy(C3D_HTXPAL htxpalToDestroy)
    {
        try {
            renderer->texturePaletteDestroy(htxpalToDestroy);
        } catch (...) {
            return HandleException();
        }

        return C3D_EC_OK;
    }

    C3D_HRC WINAPI ATI3DCIF_ContextCreate(void)
    {
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
        // can't destroy a context that wasn't created
        if (!contextCreated) {
            return C3D_EC_BADPARAM;
        }

        contextCreated = false;

        return C3D_EC_OK;
    }

    C3D_EC WINAPI ATI3DCIF_ContextSetState(C3D_HRC hRC,
        C3D_ERSID eRStateID,
        C3D_PRSDATA pRStateData)
    {

        try {
            renderer->setState(eRStateID, pRStateData);
        } catch (...) {
            return HandleException();
        }

        return C3D_EC_OK;
    }

    C3D_EC WINAPI ATI3DCIF_RenderBegin(C3D_HRC hRC)
    {
        try {
            renderer->renderBegin(hRC);
        } catch (...) {
            return HandleException();
        }

        return C3D_EC_OK;
    }

    C3D_EC WINAPI ATI3DCIF_RenderEnd(void)
    {
        try {
            renderer->renderEnd();
        } catch (...) {
            return HandleException();
        }

        return C3D_EC_OK;
    }

    C3D_EC WINAPI ATI3DCIF_RenderPrimStrip(C3D_VSTRIP vStrip,
        C3D_UINT32 u32NumVert)
    {
        try {
            renderer->renderPrimStrip(vStrip, u32NumVert);
        } catch (...) {
            return HandleException();
        }

        return C3D_EC_OK;
    }

    C3D_EC WINAPI ATI3DCIF_RenderPrimList(C3D_VLIST vList,
        C3D_UINT32 u32NumVert)
    {
        try {
            renderer->renderPrimList(vList, u32NumVert);
        } catch (...) {
            return HandleException();
        }

        return C3D_EC_OK;
    }

    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_Term_lib)(
        void) = ATI3DCIF_Term;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_Init_lib)(
        void) = ATI3DCIF_Init;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_TextureReg_lib)(
        C3D_PTMAP ptmapToReg,
        C3D_PHTX phtmap) = ATI3DCIF_TextureReg;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_TextureUnreg_lib)(
        C3D_HTX htxToUnreg) = ATI3DCIF_TextureUnreg;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_TexturePaletteCreate_lib)(
        C3D_ECI_TMAP_TYPE epalette,
        void* pPalette,
        C3D_PHTXPAL phtpalCreated) = ATI3DCIF_TexturePaletteCreate;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_TexturePaletteDestroy_lib)(
        C3D_HTXPAL htxpalToDestroy) = ATI3DCIF_TexturePaletteDestroy;
    C3D_HRC __declspec(dllexport) (*WINAPI ATI3DCIF_ContextCreate_lib)(
        void) = ATI3DCIF_ContextCreate;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_ContextDestroy_lib)(
        C3D_HRC hRC) = ATI3DCIF_ContextDestroy;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_ContextSetState_lib)(
        C3D_HRC hRC,
        C3D_ERSID eRStateID,
        C3D_PRSDATA pRStateData) = ATI3DCIF_ContextSetState;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_RenderBegin_lib)(
        C3D_HRC hRC) = ATI3DCIF_RenderBegin;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_RenderEnd_lib)(
        void) = ATI3DCIF_RenderEnd;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_RenderPrimStrip_lib)(
        C3D_VSTRIP vStrip,
        C3D_UINT32 u32NumVert) = ATI3DCIF_RenderPrimStrip;
    C3D_EC __declspec(dllexport) (*WINAPI ATI3DCIF_RenderPrimList_lib)(
        C3D_VLIST vList,
        C3D_UINT32 u32NumVert) = ATI3DCIF_RenderPrimList;

} // extern "C"

} // namespace cif
} // namespace glrage
