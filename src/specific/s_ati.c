#include "specific/s_ati.h"

#include "global/vars_platform.h"
#include "log.h"

#include <windows.h>

static HMODULE m_ATIModule = NULL;
static C3D_HRC m_RenderContext = NULL;

C3D_EC(__stdcall **m_ATI3DCIF_Init_lib)(void) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_Term_lib)(void) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_TextureReg_lib)
(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_TextureUnreg_lib)(C3D_HTX htxToUnreg) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_TexturePaletteCreate_lib)
(C3D_ECI_TMAP_TYPE epalette, void *pPalette, C3D_PHTXPAL phtpalCreated) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_TexturePaletteDestroy_lib)
(C3D_HTXPAL htxpalToDestroy) = NULL;
C3D_HRC(__stdcall **m_ATI3DCIF_ContextCreate_lib)(void) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_ContextDestroy_lib)(C3D_HRC hRC) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_ContextSetState_lib)
(C3D_HRC hRC, C3D_ERSID eRStateID, C3D_PRSDATA pRStateData) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_RenderBegin_lib)(C3D_HRC hRC) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_RenderEnd_lib)(void) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_RenderPrimStrip_lib)
(C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_RenderPrimList_lib)
(C3D_VLIST vList, C3D_UINT32 u32NumVert) = NULL;

C3D_EC S_ATI_Init()
{
    if (!g_GLRage) {
        g_GLRage = LoadLibrary("glrage");
    }

    m_ATIModule = g_GLRage;
    if (!m_ATIModule) {
        LOG_ERROR("Cannot find glrage.dll");
        return C3D_EC_GENFAIL;
    }

    C3D_EC result = C3D_EC_OK;
    m_ATI3DCIF_Init_lib = (C3D_EC(__stdcall **)(void))GetProcAddress(
        m_ATIModule, "ATI3DCIF_Init_lib");
    if (!m_ATI3DCIF_Init_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_Init_lib");
        goto fail;
    }

    m_ATI3DCIF_Term_lib = (C3D_EC(__stdcall **)(void))GetProcAddress(
        m_ATIModule, "ATI3DCIF_Term_lib");
    if (!m_ATI3DCIF_Term_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_Term_lib");
        goto fail;
    }

    m_ATI3DCIF_TextureReg_lib =
        (C3D_EC(__stdcall **)(C3D_PTMAP, C3D_PHTX))GetProcAddress(
            m_ATIModule, "ATI3DCIF_TextureReg_lib");
    if (!m_ATI3DCIF_TextureReg_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_TextureReg_lib");
        goto fail;
    }

    m_ATI3DCIF_TextureUnreg_lib = (C3D_EC(__stdcall **)(C3D_HTX))GetProcAddress(
        m_ATIModule, "ATI3DCIF_TextureUnreg_lib");
    if (!m_ATI3DCIF_TextureUnreg_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_TextureUnreg_lib");
        goto fail;
    }

    m_ATI3DCIF_TexturePaletteCreate_lib =
        (C3D_EC(__stdcall **)(C3D_ECI_TMAP_TYPE, void *, C3D_PHTXPAL))
            GetProcAddress(m_ATIModule, "ATI3DCIF_TexturePaletteCreate_lib");
    if (!m_ATI3DCIF_TexturePaletteCreate_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_TexturePaletteCreate_lib");
        goto fail;
    }

    m_ATI3DCIF_TexturePaletteDestroy_lib =
        (C3D_EC(__stdcall **)(C3D_HTXPAL))GetProcAddress(
            m_ATIModule, "ATI3DCIF_TexturePaletteDestroy_lib");
    if (!m_ATI3DCIF_TexturePaletteDestroy_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_TexturePaletteDestroy_lib");
        goto fail;
    }

    m_ATI3DCIF_ContextCreate_lib = (C3D_HRC(__stdcall **)(void))GetProcAddress(
        m_ATIModule, "ATI3DCIF_ContextCreate_lib");
    if (!m_ATI3DCIF_ContextCreate_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_ContextCreate_lib");
        goto fail;
    }

    m_ATI3DCIF_ContextDestroy_lib = (C3D_EC(__stdcall **)(
        C3D_HRC))GetProcAddress(m_ATIModule, "ATI3DCIF_ContextDestroy_lib");
    if (!m_ATI3DCIF_ContextDestroy_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_ContextDestroy_lib");
        goto fail;
    }

    m_ATI3DCIF_ContextSetState_lib =
        (C3D_EC(__stdcall **)(C3D_HRC, C3D_ERSID, C3D_PRSDATA))GetProcAddress(
            m_ATIModule, "ATI3DCIF_ContextSetState_lib");
    if (!m_ATI3DCIF_ContextSetState_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_ContextSetState_lib");
        goto fail;
    }

    m_ATI3DCIF_RenderBegin_lib = (C3D_EC(__stdcall **)(C3D_HRC))GetProcAddress(
        m_ATIModule, "ATI3DCIF_RenderBegin_lib");
    if (!m_ATI3DCIF_RenderBegin_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_RenderBegin_lib");
        goto fail;
    }

    m_ATI3DCIF_RenderEnd_lib = (C3D_EC(__stdcall **)(void))GetProcAddress(
        m_ATIModule, "ATI3DCIF_RenderEnd_lib");
    if (!m_ATI3DCIF_RenderEnd_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_RenderEnd_lib");
        goto fail;
    }

    m_ATI3DCIF_RenderPrimStrip_lib =
        (C3D_EC(__stdcall **)(C3D_VSTRIP, C3D_UINT32))GetProcAddress(
            m_ATIModule, "ATI3DCIF_RenderPrimStrip_lib");
    if (!m_ATI3DCIF_RenderPrimStrip_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_RenderPrimStrip_lib");
        goto fail;
    }

    m_ATI3DCIF_RenderPrimList_lib =
        (C3D_EC(__stdcall **)(C3D_VLIST, C3D_UINT32))GetProcAddress(
            m_ATIModule, "ATI3DCIF_RenderPrimList_lib");
    if (!m_ATI3DCIF_RenderPrimList_lib) {
        LOG_ERROR("Cannot find ATI3DCIF_RenderPrimList_lib");
        goto fail;
    }

    result = (*m_ATI3DCIF_Init_lib)();
    if (result != C3D_EC_OK) {
        LOG_ERROR("Error while initializing ATI3DCIF: 0x%lx", result);
        goto fail;
    }

    m_RenderContext = ATI3DCIF_ContextCreate();
    if (!m_RenderContext) {
        LOG_ERROR("Error while creating ATI3DCIF context");
        goto fail;
    }

    return result;

fail:
    if (m_ATIModule) {
        FreeLibrary(m_ATIModule);
        m_ATIModule = NULL;
    }
    return C3D_EC_GENFAIL;
}

C3D_EC S_ATI_Shutdown()
{
    if (!m_ATIModule) {
        return C3D_EC_GENFAIL;
    }

    if (m_RenderContext) {
        ATI3DCIF_ContextDestroy(m_RenderContext);
        m_RenderContext = 0;
    }

    C3D_EC result =
        m_ATI3DCIF_Term_lib ? (*m_ATI3DCIF_Term_lib)() : C3D_EC_GENFAIL;

    FreeLibrary(m_ATIModule);
    m_ATIModule = NULL;

    return result;
}

C3D_HRC S_ATI_GetRenderContext()
{
    return m_RenderContext;
}

C3D_EC ATI3DCIF_TextureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap)
{
    return m_ATI3DCIF_TextureReg_lib
        ? (*m_ATI3DCIF_TextureReg_lib)(ptmapToReg, phtmap)
        : C3D_EC_GENFAIL;
}

C3D_EC ATI3DCIF_TextureUnreg(C3D_HTX htxToUnreg)
{
    return m_ATI3DCIF_TextureUnreg_lib
        ? (*m_ATI3DCIF_TextureUnreg_lib)(htxToUnreg)
        : C3D_EC_GENFAIL;
}

C3D_EC ATI3DCIF_TexturePaletteCreate(
    C3D_ECI_TMAP_TYPE epalette, void *pPalette, C3D_PHTXPAL phtpalCreated)
{
    return m_ATI3DCIF_TexturePaletteCreate_lib
        ? (*m_ATI3DCIF_TexturePaletteCreate_lib)(
            epalette, pPalette, phtpalCreated)
        : C3D_EC_GENFAIL;
}

C3D_EC ATI3DCIF_TexturePaletteDestroy(C3D_HTXPAL htxpalToDestroy)
{
    return m_ATI3DCIF_TexturePaletteDestroy_lib
        ? (*m_ATI3DCIF_TexturePaletteDestroy_lib)(htxpalToDestroy)
        : C3D_EC_GENFAIL;
}

C3D_HRC ATI3DCIF_ContextCreate(void)
{
    return m_ATI3DCIF_ContextCreate_lib ? (*m_ATI3DCIF_ContextCreate_lib)()
                                        : NULL;
}

C3D_EC ATI3DCIF_ContextDestroy(C3D_HRC hRC)
{
    return m_ATI3DCIF_ContextDestroy_lib ? (*m_ATI3DCIF_ContextDestroy_lib)(hRC)
                                         : C3D_EC_GENFAIL;
}

C3D_EC ATI3DCIF_ContextSetState(
    C3D_HRC hRC, C3D_ERSID eRStateID, C3D_PRSDATA pRStateData)
{
    return m_ATI3DCIF_ContextSetState_lib
        ? (*m_ATI3DCIF_ContextSetState_lib)(hRC, eRStateID, pRStateData)
        : C3D_EC_GENFAIL;
}

C3D_EC ATI3DCIF_RenderBegin(C3D_HRC hRC)
{
    return m_ATI3DCIF_RenderBegin_lib ? (*m_ATI3DCIF_RenderBegin_lib)(hRC)
                                      : C3D_EC_GENFAIL;
}

C3D_EC ATI3DCIF_RenderEnd(void)
{
    return m_ATI3DCIF_RenderEnd_lib ? (*m_ATI3DCIF_RenderEnd_lib)()
                                    : C3D_EC_GENFAIL;
}

C3D_EC ATI3DCIF_RenderPrimStrip(C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert)
{
    return m_ATI3DCIF_RenderPrimStrip_lib
        ? (*m_ATI3DCIF_RenderPrimStrip_lib)(vStrip, u32NumVert)
        : C3D_EC_GENFAIL;
}

C3D_EC ATI3DCIF_RenderPrimList(C3D_VLIST vList, C3D_UINT32 u32NumVert)
{
    return m_ATI3DCIF_RenderPrimList_lib
        ? (*m_ATI3DCIF_RenderPrimList_lib)(vList, u32NumVert)
        : C3D_EC_GENFAIL;
}
