#include "specific/s_ati.h"

#include "global/vars_platform.h"
#include "log.h"

#include "ati3dcif/Interop.hpp"

bool S_ATI_Init()
{
    C3D_EC result = ATI3DCIF_Init();
    if (result != C3D_EC_OK) {
        LOG_ERROR("Error while initializing ATI3DCIF: 0x%lx", result);
        return false;
    }

    return true;
}

bool S_ATI_Shutdown()
{
    return ATI3DCIF_Term() == C3D_EC_OK;
}
