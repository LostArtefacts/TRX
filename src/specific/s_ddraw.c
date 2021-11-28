#include "specific/s_ddraw.h"

#include "log.h"
#include "global/vars_platform.h"

#include <ddraw.h>

static HRESULT (*m_DirectDrawCreate)(GUID *, LPDIRECTDRAW *, IUnknown *) = NULL;

bool S_DDraw_Init()
{
    if (!g_GLRage) {
        LOG_ERROR("Cannot find glrage.dll");
        return false;
    }

    m_DirectDrawCreate =
        (HRESULT(*)(GUID *, LPDIRECTDRAW *, IUnknown *))GetProcAddress(
            g_GLRage, "DirectDrawCreate");

    if (!m_DirectDrawCreate) {
        LOG_ERROR("Cannot find DirectDrawCreate");
        return false;
    }

    if (m_DirectDrawCreate(NULL, &g_DDraw, NULL)) {
        LOG_ERROR("DirectDraw could not be started");
        return false;
    }

    IDirectDraw_Initialize(g_DDraw, NULL);

    return true;
}

void S_DDraw_Shutdown()
{
    if (g_DDraw) {
        IDirectDraw_FlipToGDISurface(g_DDraw);
        IDirectDraw_FlipToGDISurface(g_DDraw);
        IDirectDraw_RestoreDisplayMode(g_DDraw);
        IDirectDraw_SetCooperativeLevel(g_DDraw, g_TombHWND, 8);
        IDirectDraw_Release(g_DDraw);
        g_DDraw = NULL;
    }
}
