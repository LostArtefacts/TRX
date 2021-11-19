#include "specific/s_music.h"

#include "global/vars_platform.h"
#include "log.h"

#include <SDL2/SDL.h>
#include <mmsystem.h>

static MCIDEVICEID m_MCIDeviceID = (MCIDEVICEID)-1;
static UINT m_AUXDeviceID = (UINT)-1;
static int32_t m_NumTracks = 0;

bool S_Music_Init()
{
    {
        int32_t result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        if (result < 0) {
            LOG_ERROR("Error while calling SDL_Init: 0x%lx", result);
            return false;
        }
    }

    {
        MCI_OPEN_PARMS open_parms;
        open_parms.dwCallback = 0;
        open_parms.wDeviceID = 0;
        open_parms.lpstrDeviceType = (LPSTR)MCI_DEVTYPE_CD_AUDIO;
        open_parms.lpstrElementName = NULL;
        open_parms.lpstrAlias = NULL;

        MCIERROR result = mciSendCommandA(
            0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID,
            (DWORD_PTR)&open_parms);
        if (result != MMSYSERR_NOERROR) {
            LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
            return false;
        }
        m_MCIDeviceID = open_parms.wDeviceID;
        LOG_INFO("MCI device ID: %d", m_MCIDeviceID);
    }

    {
        m_AUXDeviceID = 0;
        for (int i = 0; i < (int)auxGetNumDevs(); i++) {
            AUXCAPSA caps;
            auxGetDevCapsA((UINT_PTR)i, &caps, sizeof(AUXCAPSA));
            if (caps.wTechnology == AUXCAPS_CDAUDIO) {
                m_AUXDeviceID = i;
            }
        }
        LOG_INFO("MCI device ID: %d", m_AUXDeviceID);
    }

    {
        MCI_STATUS_PARMS status_parms;
        status_parms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
        MCIERROR result = mciSendCommandA(
            m_MCIDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
            (DWORD_PTR)&status_parms);
        if (result != MMSYSERR_NOERROR) {
            LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
            m_MCIDeviceID = -1;
            m_AUXDeviceID = -1;
            return false;
        }
        m_NumTracks = status_parms.dwReturn;
    }

    return true;
}

bool S_Music_SetVolume(int16_t volume)
{
    int32_t volume_aux = volume * 0xFFFF / 0xFF;
    volume_aux |= volume_aux << 16;
    MMRESULT result = auxSetVolume(m_AUXDeviceID, volume_aux);
    if (result != MMSYSERR_NOERROR) {
        LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
        return false;
    }
    return true;
}

bool S_Music_Pause()
{
    MCI_GENERIC_PARMS pause_parms;
    MCIERROR result = mciSendCommandA(
        m_MCIDeviceID, MCI_PAUSE, MCI_WAIT, (DWORD_PTR)&pause_parms);
    if (result != MMSYSERR_NOERROR) {
        LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
        return false;
    }
    return true;
}

bool S_Music_Unpause()
{
    MCI_GENERIC_PARMS pause_parms;
    MCIERROR result = mciSendCommandA(
        m_MCIDeviceID, MCI_RESUME, MCI_WAIT, (DWORD_PTR)&pause_parms);
    if (result != MMSYSERR_NOERROR) {
        LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
        return false;
    }
    return true;
}

bool S_Music_Play(int16_t track)
{
    {
        MCI_SET_PARMS set_parms;
        set_parms.dwTimeFormat = MCI_FORMAT_TMSF;

        MCIERROR result = mciSendCommandA(
            m_MCIDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&set_parms);
        if (result != MMSYSERR_NOERROR) {
            LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
            return false;
        }
    }

    {
        MCI_PLAY_PARMS open_parms;
        open_parms.dwFrom = track;
        open_parms.dwCallback = (DWORD_PTR)TombHWND;

        DWORD_PTR flags = MCI_NOTIFY | MCI_FROM;
        if (track != m_NumTracks) {
            open_parms.dwTo = track + 1;
            flags |= MCI_TO;
        }

        MCIERROR result = mciSendCommandA(
            m_MCIDeviceID, MCI_PLAY, flags, (DWORD_PTR)&open_parms);
        if (result != MMSYSERR_NOERROR) {
            LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
            return false;
        }
    }

    return true;
}

bool S_Music_Stop()
{
    MCI_GENERIC_PARMS gen_parms;
    MCIERROR result = mciSendCommandA(
        m_MCIDeviceID, MCI_STOP, MCI_WAIT, (DWORD_PTR)&gen_parms);
    if (result != MMSYSERR_NOERROR) {
        LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
        return false;
    }
    return true;
}
