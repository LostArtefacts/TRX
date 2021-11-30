#ifndef T1M_SPECIFIC_S_ATI_H
#define T1M_SPECIFIC_S_ATI_H

#include "ati3dcif/ATI3DCIF.h"

#include <stdbool.h>

bool S_ATI_Init();
bool S_ATI_Shutdown();
C3D_HRC S_ATI_GetRenderContext();

#endif
