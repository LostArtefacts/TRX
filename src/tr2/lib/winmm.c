#include "lib/winmm.h"

MMRESULT(__stdcall *g_MM_auxGetDevCapsA)
(UINT_PTR uDeviceID, LPAUXCAPSA pac, UINT cbac) = NULL;

UINT(__stdcall *g_MM_auxGetNumDevs)(void) = NULL;

MMRESULT(__stdcall *g_MM_auxSetVolume)(UINT uDeviceID, DWORD dwVolume) = NULL;

MCIERROR(__stdcall *g_MM_mciSendCommandA)
(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2) = NULL;

MCIERROR(__stdcall *g_MM_mciSendStringA)
(LPCSTR lpszCommand, LPSTR lpszReturnString, UINT cchReturn,
 HANDLE hwndCallback) = NULL;

void WinMM_Load(void)
{
    HANDLE winmm = LoadLibrary("winmm.dll");

    auxGetNumDevs = (UINT(__stdcall *)())GetProcAddress(winmm, "auxGetNumDevs");

    auxSetVolume = (MMRESULT(__stdcall *)(UINT, DWORD))GetProcAddress(
        winmm, "auxSetVolume");

    mciSendCommandA =
        (MCIERROR(__stdcall *)(MCIDEVICEID, UINT, DWORD_PTR, DWORD_PTR))
            GetProcAddress(winmm, "mciSendCommandA");

    mciSendStringA = (MCIERROR(__stdcall *)(
        LPCSTR, LPSTR, UINT, HANDLE))GetProcAddress(winmm, "mciSendStringA");
}
