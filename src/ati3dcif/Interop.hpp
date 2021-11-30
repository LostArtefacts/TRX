#pragma once

#include "ati3dcif/ATI3DCIF.h"

#ifdef __cplusplus
extern "C" {
#endif

C3D_EC ATI3DCIF_Term(void);
C3D_EC ATI3DCIF_Init(void);
C3D_EC ATI3DCIF_TextureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap);
C3D_EC ATI3DCIF_TextureUnreg(C3D_HTX htxToUnreg);
C3D_EC ATI3DCIF_TexturePaletteCreate(
    C3D_ECI_TMAP_TYPE epalette, void *pPalette, C3D_PHTXPAL phtpalCreated);
C3D_EC ATI3DCIF_TexturePaletteDestroy(C3D_HTXPAL htxpalToDestroy);
C3D_HRC ATI3DCIF_ContextCreate(void);
C3D_EC ATI3DCIF_ContextDestroy(C3D_HRC hRC);
C3D_EC ATI3DCIF_ContextSetState(
    C3D_HRC hRC, C3D_ERSID eRStateID, C3D_PRSDATA pRStateData);
C3D_EC ATI3DCIF_RenderBegin(C3D_HRC hRC);
C3D_EC ATI3DCIF_RenderEnd(void);
C3D_EC ATI3DCIF_RenderPrimStrip(C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert);
C3D_EC ATI3DCIF_RenderPrimList(C3D_VLIST vList, C3D_UINT32 u32NumVert);

#ifdef __cplusplus
}
#endif
