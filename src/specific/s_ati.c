#include "specific/s_ati.h"

#include "global/vars_platform.h"
#include "log.h"

#include "ati3dcif/Interop.hpp"

#include <windows.h>

static C3D_HRC m_RenderContext = NULL;

bool S_ATI_Init()
{
    C3D_EC result = ATI3DCIF_Init();
    if (result != C3D_EC_OK) {
        LOG_ERROR("Error while initializing ATI3DCIF: 0x%lx", result);
        return false;
    }

    m_RenderContext = ATI3DCIF_ContextCreate();
    if (!m_RenderContext) {
        LOG_ERROR("Error while creating ATI3DCIF context");
        return false;
    }

    return true;
}

bool S_ATI_Shutdown()
{
    if (m_RenderContext) {
        ATI3DCIF_ContextDestroy(m_RenderContext);
        m_RenderContext = 0;
    }

    return ATI3DCIF_Term() == C3D_EC_OK;
}

C3D_HRC S_ATI_GetRenderContext()
{
    return m_RenderContext;
}
