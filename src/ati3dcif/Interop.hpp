#pragma once

#include "ati3dcif/ATI3DCIF.h"

#ifdef __cplusplus
    #include <cstdbool>
extern "C" {
#else
    #include <stdbool.h>
#endif

int ATI3DCIF_TextureReg(const void *data, int width, int height);
bool ATI3DCIF_TextureUnreg(int texture_num);
bool ATI3DCIF_SetState(C3D_ERSID eRStateID, C3D_PRSDATA pRStateData);
void ATI3DCIF_RenderBegin(void);
void ATI3DCIF_RenderEnd(void);
void ATI3DCIF_RenderPrimStrip(C3D_VSTRIP vStrip, int u32NumVert);
void ATI3DCIF_RenderPrimList(C3D_VLIST vList, int u32NumVert);

#ifdef __cplusplus
}
#endif
