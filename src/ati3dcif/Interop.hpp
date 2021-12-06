#pragma once

#include "ati3dcif/ATI3DCIF.h"

#ifdef __cplusplus
    #include <cstdbool>
extern "C" {
#else
    #include <stdbool.h>
#endif

void ATI3DCIF_Term(void);
void ATI3DCIF_Init(void);
bool ATI3DCIF_TextureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap);
bool ATI3DCIF_TextureUnreg(C3D_HTX htxToUnreg);
bool ATI3DCIF_SetState(C3D_ERSID eRStateID, C3D_PRSDATA pRStateData);
void ATI3DCIF_RenderBegin(void);
void ATI3DCIF_RenderEnd(void);
void ATI3DCIF_RenderPrimStrip(C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert);
void ATI3DCIF_RenderPrimList(C3D_VLIST vList, C3D_UINT32 u32NumVert);

#ifdef __cplusplus
}
#endif
