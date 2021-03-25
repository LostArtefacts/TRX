#include "specific/ati.h"

#include "util.h"

#include <windows.h>

// clang-format off
#define HATI3DCIFModule         VAR_U_(0x00459CF0, HMODULE)
#define ATI3DCIF_GetInfo_lib                ((C3D_EC (**)(PC3D_3DCIFINFO info))0x00459CF4)
#define ATI3DCIF_TextureReg_lib             ((C3D_EC (**)(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap))0x00459CF8)
#define ATI3DCIF_TextureUnreg_lib           ((C3D_EC (**)(C3D_HTX htxToUnreg))0x00459CFC)
#define ATI3DCIF_TexturePaletteCreate_lib   ((C3D_EC (**)(C3D_ECI_TMAP_TYPE epalette, void *pPalette, C3D_PHTXPAL phtpalCreated))0x00459D00)
#define ATI3DCIF_TexturePaletteDestroy_lib  ((C3D_EC (**)(C3D_HTXPAL htxpalToDestroy))0x00459D04)
#define ATI3DCIF_TexturePaletteAnimate      ((C3D_EC (**)(C3D_HTXPAL htxpalToAnimate, C3D_UINT32 u32StartIndex, C3D_UINT32 u32NumEntries, C3D_PPALETTENTRY pclrPalette))0x00459D08)
#define ATI3DCIF_ContextCreate_lib          ((C3D_HRC (**)())0x00459D10)
#define ATI3DCIF_ContextDestroy_lib         ((C3D_EC (**)(C3D_HRC hRC))0x00459D14)
#define ATI3DCIF_ContextSetState_lib        ((C3D_EC (**)(C3D_HRC hRC, C3D_ERSID eRStateID, C3D_PRSDATA pRStateData))0x00459D18)
#define ATI3DCIF_RenderBegin_lib            ((C3D_EC (**)(C3D_HRC hRC))0x00459D1C)
#define ATI3DCIF_RenderEnd_lib              ((C3D_EC (**)())0x00459D20)
#define ATI3DCIF_RenderSwitch               ((C3D_EC (**)(C3D_HRC hRC))0x00459D24)
#define ATI3DCIF_RenderPrimStrip_lib        ((C3D_EC (**)(C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert))0x00459D28)
#define ATI3DCIF_RenderPrimList_lib         ((C3D_EC (**)(C3D_VLIST vList, C3D_UINT32 u32NumVert))0x00459D2C)
// clang-format on

C3D_EC InitATI3DCIF()
{
    C3D_EC (*ATI3DCIF_Init_lib)();

    HATI3DCIFModule = LoadLibraryA("ati3dcif");
    if (!HATI3DCIFModule) {
        LOG_ERROR("cannot find ati3dcif.dll");
        return C3D_EC_GENFAIL;
    }

    C3D_EC result = C3D_EC_OK;
    ATI3DCIF_Init_lib =
        *(C3D_EC(**)())GetProcAddress(HATI3DCIFModule, "ATI3DCIF_Init_lib");
    if (!ATI3DCIF_Init_lib) {
        LOG_ERROR("cannot find ATI3DCIF_Init_lib");
        ATI3DCIF_Init_lib = (C3D_EC(*)())ATI3DCIF_NullSub;
        FreeLibrary(HATI3DCIFModule);
        return C3D_EC_GENFAIL;
    }

    result = (*ATI3DCIF_Init_lib)();
    if (result != C3D_EC_OK) {
        LOG_ERROR("error while initializing ATI3DCIF");
        return result;
    }

    *ATI3DCIF_GetInfo_lib = *(C3D_EC(**)(C3D_3DCIFINFO *))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_GetInfo_lib");
    if (!ATI3DCIF_GetInfo_lib) {
        LOG_ERROR("cannot find ATI3DCIF_GetInfo_lib");
        *ATI3DCIF_GetInfo_lib = (C3D_EC(*)(C3D_3DCIFINFO *))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_TextureReg_lib = *(C3D_EC(**)(C3D_PTMAP, C3D_PHTX))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_TextureReg_lib");
    if (!ATI3DCIF_TextureReg_lib) {
        LOG_ERROR("cannot find ATI3DCIF_TextureReg_lib");
        *ATI3DCIF_TextureReg_lib =
            (C3D_EC(*)(C3D_PTMAP, C3D_PHTX))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_TextureUnreg_lib = *(C3D_EC(**)(C3D_HTX))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_TextureUnreg_lib");
    if (!ATI3DCIF_TextureUnreg_lib) {
        LOG_ERROR("cannot find ATI3DCIF_TextureUnreg_lib");
        *ATI3DCIF_TextureUnreg_lib = (C3D_EC(*)(C3D_HTX))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_TexturePaletteCreate_lib =
        *(C3D_EC(**)(C3D_ECI_TMAP_TYPE, void *, C3D_PHTXPAL))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_TexturePaletteCreate_lib");
    if (!ATI3DCIF_TexturePaletteCreate_lib) {
        LOG_ERROR("cannot find ATI3DCIF_TexturePaletteCreate_lib");
        *ATI3DCIF_TexturePaletteCreate_lib =
            (C3D_EC(*)(C3D_ECI_TMAP_TYPE, void *, C3D_PHTXPAL))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_TexturePaletteDestroy_lib =
        *(C3D_EC(**)(C3D_HTXPAL))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_TexturePaletteDestroy_lib");
    if (!ATI3DCIF_TexturePaletteDestroy_lib) {
        LOG_ERROR("cannot find ATI3DCIF_TexturePaletteDestroy_lib");
        *ATI3DCIF_TexturePaletteDestroy_lib =
            (C3D_EC(*)(C3D_HTXPAL))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_TexturePaletteAnimate =
        *(C3D_EC(**)(C3D_HTXPAL, C3D_UINT32, C3D_UINT32, C3D_PPALETTENTRY))
            GetProcAddress(
                HATI3DCIFModule, "ATI3DCIF_TexturePaletteAnimate_lib");
    if (!ATI3DCIF_TexturePaletteAnimate) {
        LOG_ERROR("cannot find ATI3DCIF_TexturePaletteAnimate_lib");
        *ATI3DCIF_TexturePaletteAnimate =
            (C3D_EC(*)(C3D_HTXPAL, C3D_UINT32, C3D_UINT32, C3D_PPALETTENTRY))
                ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_ContextCreate_lib = *(C3D_HRC(**)())GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_ContextCreate_lib");
    if (!ATI3DCIF_ContextCreate_lib) {
        LOG_ERROR("cannot find ATI3DCIF_ContextCreate_lib");
        *ATI3DCIF_ContextCreate_lib = (C3D_HRC(*)())ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_ContextDestroy_lib = *(C3D_EC(**)(C3D_HRC))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_ContextDestroy_lib");
    if (!ATI3DCIF_ContextDestroy_lib) {
        LOG_ERROR("cannot find ATI3DCIF_ContextDestroy_lib");
        *ATI3DCIF_ContextDestroy_lib = (C3D_EC(*)(C3D_HRC))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_ContextSetState_lib =
        *(C3D_EC(**)(C3D_HRC, C3D_ERSID, C3D_PRSDATA))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_ContextSetState_lib");
    if (!ATI3DCIF_ContextSetState_lib) {
        LOG_ERROR("cannot find ATI3DCIF_ContextSetState_lib");
        *ATI3DCIF_ContextSetState_lib =
            (C3D_EC(*)(C3D_HRC, C3D_ERSID, C3D_PRSDATA))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_RenderBegin_lib = *(C3D_EC(**)(C3D_HRC))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_RenderBegin_lib");
    if (!ATI3DCIF_RenderBegin_lib) {
        LOG_ERROR("cannot find ATI3DCIF_RenderBegin_lib");
        *ATI3DCIF_RenderBegin_lib = (C3D_EC(*)(C3D_HRC))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_RenderEnd_lib = *(C3D_EC(**)())GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_RenderEnd_lib");
    if (!ATI3DCIF_RenderEnd_lib) {
        LOG_ERROR("cannot find ATI3DCIF_RenderEnd_lib");
        *ATI3DCIF_RenderEnd_lib = ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_RenderSwitch = *(C3D_EC(**)(C3D_HRC))GetProcAddress(
        HATI3DCIFModule, "ATI3DCIF_RenderSwitch_lib");
    if (!ATI3DCIF_RenderSwitch) {
        LOG_ERROR("cannot find ATI3DCIF_RenderSwitch_lib");
        *ATI3DCIF_RenderSwitch = (C3D_EC(*)(C3D_HRC))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_RenderPrimStrip_lib =
        *(C3D_EC(**)(C3D_VSTRIP, C3D_UINT32))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_RenderPrimStrip_lib");
    if (!ATI3DCIF_RenderPrimStrip_lib) {
        LOG_ERROR("cannot find ATI3DCIF_RenderPrimStrip_lib");
        *ATI3DCIF_RenderPrimStrip_lib =
            (C3D_EC(*)(C3D_VSTRIP, C3D_UINT32))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    *ATI3DCIF_RenderPrimList_lib =
        *(C3D_EC(**)(C3D_VLIST, C3D_UINT32))GetProcAddress(
            HATI3DCIFModule, "ATI3DCIF_RenderPrimList_lib");
    if (!ATI3DCIF_RenderPrimList_lib) {
        LOG_ERROR("cannot find ATI3DCIF_RenderPrimList_lib");
        *ATI3DCIF_RenderPrimList_lib =
            (C3D_EC(*)(C3D_VLIST, C3D_UINT32))ATI3DCIF_NullSub;
        result = C3D_EC_GENFAIL;
    }

    return result;
}

C3D_EC ATI3DCIF_NullSub()
{
    return C3D_EC_GENFAIL;
}

C3D_EC __stdcall ATI3DCIF_GetInfo(PC3D_3DCIFINFO info)
{
    return (*ATI3DCIF_GetInfo_lib)(info);
}

C3D_EC __stdcall ATI3DCIF_TextureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap)
{
    return (*ATI3DCIF_TextureReg_lib)(ptmapToReg, phtmap);
}

C3D_EC __stdcall ATI3DCIF_TextureUnreg(C3D_HTX htxToUnreg)
{
    return (*ATI3DCIF_TextureUnreg_lib)(htxToUnreg);
}

C3D_EC __stdcall ATI3DCIF_TexturePaletteCreate(
    C3D_ECI_TMAP_TYPE epalette, void *pPalette, C3D_PHTXPAL phtpalCreated)
{
    return (*ATI3DCIF_TexturePaletteCreate_lib)(
        epalette, pPalette, phtpalCreated);
}

C3D_EC __stdcall ATI3DCIF_TexturePaletteDestroy(C3D_HTXPAL htxpalToDestroy)
{
    return (*ATI3DCIF_TexturePaletteDestroy_lib)(htxpalToDestroy);
}

C3D_HRC __stdcall ATI3DCIF_ContextCreate()
{
    return (*ATI3DCIF_ContextCreate_lib)();
}

C3D_EC __stdcall ATI3DCIF_ContextDestroy(C3D_HRC hRC)
{
    return (*ATI3DCIF_ContextDestroy_lib)(hRC);
}

C3D_EC __stdcall ATI3DCIF_ContextSetState(
    C3D_HRC hRC, C3D_ERSID eRStateID, C3D_PRSDATA pRStateData)
{
    return (*ATI3DCIF_ContextSetState_lib)(hRC, eRStateID, pRStateData);
}

C3D_EC __stdcall ATI3DCIF_RenderBegin(C3D_HRC hRC)
{
    return (*ATI3DCIF_RenderBegin_lib)(hRC);
}

C3D_EC __stdcall ATI3DCIF_RenderEnd()
{
    return (*ATI3DCIF_RenderEnd_lib)();
}

C3D_EC __stdcall ATI3DCIF_RenderPrimStrip(
    C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert)
{
    return (*ATI3DCIF_RenderPrimStrip_lib)(vStrip, u32NumVert);
}

C3D_EC __stdcall ATI3DCIF_RenderPrimList(C3D_VLIST vList, C3D_UINT32 u32NumVert)
{
    return (*ATI3DCIF_RenderPrimList_lib)(vList, u32NumVert);
}

void T1MInjectSpecificATI()
{
    INJECT(0x00450500, ATI3DCIF_NullSub);
    INJECT(0x00450510, InitATI3DCIF);
    INJECT(0x004507B0, ATI3DCIF_GetInfo);
    INJECT(0x004507C0, ATI3DCIF_TextureReg);
    INJECT(0x004507E0, ATI3DCIF_TextureUnreg);
    INJECT(0x004507F0, ATI3DCIF_TexturePaletteCreate);
    INJECT(0x00450810, ATI3DCIF_TexturePaletteDestroy);
    INJECT(0x00450820, ATI3DCIF_ContextCreate);
    INJECT(0x00450830, ATI3DCIF_ContextDestroy);
    INJECT(0x00450840, ATI3DCIF_ContextSetState);
    INJECT(0x00450860, ATI3DCIF_RenderBegin);
    INJECT(0x00450870, ATI3DCIF_RenderEnd);
    INJECT(0x00450880, ATI3DCIF_RenderPrimStrip);
    INJECT(0x004508A0, ATI3DCIF_RenderPrimList);
}
