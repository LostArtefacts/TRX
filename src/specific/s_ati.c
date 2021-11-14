#include "specific/s_ati.h"

#include "global/vars_platform.h"
#include "inject_util.h"
#include "log.h"

#include <windows.h>

C3D_EC(__stdcall **m_ATI3DCIF_Init_lib)(void) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_Term_lib)(void) = NULL;
C3D_EC(__stdcall **m_ATI3DCIF_GetInfo_lib)(PC3D_3DCIFINFO info) = NULL;
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

C3D_EC InitATI3DCIF()
{
    HATI3DCIFModule = LoadLibraryA("ati3dcif");
    if (!HATI3DCIFModule) {
        LOG_ERROR("cannot find ati3dcif.dll");
        return C3D_EC_GENFAIL;
    }

    C3D_EC result = C3D_EC_OK;
    m_ATI3DCIF_Init_lib = (C3D_EC(__stdcall **)(void))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_Init_lib");
    if (!m_ATI3DCIF_Init_lib) {
        LOG_ERROR("cannot find ATI3DCIF_Init_lib");
        goto fail;
    }

    m_ATI3DCIF_Term_lib = (C3D_EC(__stdcall **)(void))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_Term_lib");
    if (!m_ATI3DCIF_Term_lib) {
        LOG_ERROR("cannot find ATI3DCIF_Term_lib");
        goto fail;
    }

    m_ATI3DCIF_GetInfo_lib =
        (C3D_EC(__stdcall **)(C3D_3DCIFINFO *))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_GetInfo_lib");
    if (!m_ATI3DCIF_GetInfo_lib) {
        LOG_ERROR("cannot find ATI3DCIF_GetInfo_lib");
        goto fail;
    }

    m_ATI3DCIF_TextureReg_lib =
        (C3D_EC(__stdcall **)(C3D_PTMAP, C3D_PHTX))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_TextureReg_lib");
    if (!m_ATI3DCIF_TextureReg_lib) {
        LOG_ERROR("cannot find ATI3DCIF_TextureReg_lib");
        goto fail;
    }

    m_ATI3DCIF_TextureUnreg_lib = (C3D_EC(__stdcall **)(C3D_HTX))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_TextureUnreg_lib");
    if (!m_ATI3DCIF_TextureUnreg_lib) {
        LOG_ERROR("cannot find ATI3DCIF_TextureUnreg_lib");
        goto fail;
    }

    m_ATI3DCIF_TexturePaletteCreate_lib = (C3D_EC(__stdcall **)(
        C3D_ECI_TMAP_TYPE, void *, C3D_PHTXPAL))
        GetProcAddress(HATI3DCIFModule, "ATI3DCIF_TexturePaletteCreate_lib");
    if (!m_ATI3DCIF_TexturePaletteCreate_lib) {
        LOG_ERROR("cannot find ATI3DCIF_TexturePaletteCreate_lib");
        goto fail;
    }

    m_ATI3DCIF_TexturePaletteDestroy_lib =
        (C3D_EC(__stdcall **)(C3D_HTXPAL))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_TexturePaletteDestroy_lib");
    if (!m_ATI3DCIF_TexturePaletteDestroy_lib) {
        LOG_ERROR("cannot find ATI3DCIF_TexturePaletteDestroy_lib");
        goto fail;
    }

    m_ATI3DCIF_ContextCreate_lib = (C3D_HRC(__stdcall **)(void))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_ContextCreate_lib");
    if (!m_ATI3DCIF_ContextCreate_lib) {
        LOG_ERROR("cannot find ATI3DCIF_ContextCreate_lib");
        goto fail;
    }

    m_ATI3DCIF_ContextDestroy_lib = (C3D_EC(__stdcall **)(
        C3D_HRC))GetProcAddress(HATI3DCIFModule, "ATI3DCIF_ContextDestroy_lib");
    if (!m_ATI3DCIF_ContextDestroy_lib) {
        LOG_ERROR("cannot find ATI3DCIF_ContextDestroy_lib");
        goto fail;
    }

    m_ATI3DCIF_ContextSetState_lib =
        (C3D_EC(__stdcall **)(C3D_HRC, C3D_ERSID, C3D_PRSDATA))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_ContextSetState_lib");
    if (!m_ATI3DCIF_ContextSetState_lib) {
        LOG_ERROR("cannot find ATI3DCIF_ContextSetState_lib");
        goto fail;
    }

    m_ATI3DCIF_RenderBegin_lib = (C3D_EC(__stdcall **)(C3D_HRC))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_RenderBegin_lib");
    if (!m_ATI3DCIF_RenderBegin_lib) {
        LOG_ERROR("cannot find ATI3DCIF_RenderBegin_lib");
        goto fail;
    }

    m_ATI3DCIF_RenderEnd_lib = (C3D_EC(__stdcall **)(void))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_RenderEnd_lib");
    if (!m_ATI3DCIF_RenderEnd_lib) {
        LOG_ERROR("cannot find ATI3DCIF_RenderEnd_lib");
        goto fail;
    }

    m_ATI3DCIF_RenderPrimStrip_lib =
        (C3D_EC(__stdcall **)(C3D_VSTRIP, C3D_UINT32))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_RenderPrimStrip_lib");
    if (!m_ATI3DCIF_RenderPrimStrip_lib) {
        LOG_ERROR("cannot find ATI3DCIF_RenderPrimStrip_lib");
        goto fail;
    }

    m_ATI3DCIF_RenderPrimList_lib =
        (C3D_EC(__stdcall **)(C3D_VLIST, C3D_UINT32))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_RenderPrimList_lib");
    if (!m_ATI3DCIF_RenderPrimList_lib) {
        LOG_ERROR("cannot find ATI3DCIF_RenderPrimList_lib");
        goto fail;
    }

    result = (*m_ATI3DCIF_Init_lib)();
    if (result != C3D_EC_OK) {
        LOG_ERROR("error while initializing ATI3DCIF");
        return result;
    }

    return result;

fail:
    if (HATI3DCIFModule) {
        FreeLibrary(HATI3DCIFModule);
        HATI3DCIFModule = NULL;
    }
    return C3D_EC_GENFAIL;
}

C3D_EC ShutdownATI3DCIF()
{
    if (!HATI3DCIFModule) {
        return C3D_EC_GENFAIL;
    }

    C3D_EC result =
        m_ATI3DCIF_Term_lib ? (*m_ATI3DCIF_Term_lib)() : C3D_EC_GENFAIL;

    FreeLibrary(HATI3DCIFModule);
    HATI3DCIFModule = NULL;

    return result;
}

C3D_EC ATI3DCIF_GetInfo(PC3D_3DCIFINFO info)
{
    return m_ATI3DCIF_GetInfo_lib ? (*m_ATI3DCIF_GetInfo_lib)(info)
                                  : C3D_EC_GENFAIL;
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
