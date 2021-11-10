#include "specific/music.h"

#include "global/vars_platform.h"
#include "log.h"

static struct {
    uint32_t mci_device_id;
    uint32_t aux_device_id;
    int32_t num_tracks;
} S = { 0 };

bool S_Music_Init()
{
    MCI_OPEN_PARMS open_parms;
    open_parms.dwCallback = 0;
    open_parms.wDeviceID = 0;
    open_parms.lpstrDeviceType = (LPSTR)MCI_DEVTYPE_CD_AUDIO;
    open_parms.lpstrElementName = NULL;
    open_parms.lpstrAlias = NULL;

    MCIERROR result = mciSendCommandA(
        0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID, (DWORD_PTR)&open_parms);
    if (result) {
        LOG_ERROR("cannot initailize music device: %x", result);
        return false;
    }
    S.mci_device_id = open_parms.wDeviceID;

    S.aux_device_id = 0;
    for (int i = 0; i < auxGetNumDevs(); i++) {
        AUXCAPSA caps;
        auxGetDevCapsA((UINT_PTR)i, &caps, sizeof(AUXCAPSA));
        if (caps.wTechnology == AUXCAPS_CDAUDIO) {
            S.aux_device_id = i;
        }
    }

    MCI_STATUS_PARMS status_parms;
    status_parms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    mciSendCommandA(
        S.mci_device_id, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&status_parms);
    S.num_tracks = status_parms.dwReturn;
    return true;
}

void S_Music_SetVolume(int16_t volume)
{
    int32_t volume_aux = volume * 0xFFFF / 0xFF;
    volume_aux |= volume_aux << 16;
    auxSetVolume(S.aux_device_id, volume_aux);
}

void S_Music_Pause()
{
    MCIERROR result;
    MCI_GENERIC_PARMS pause_parms;

    result = mciSendCommandA(
        S.mci_device_id, MCI_PAUSE, MCI_WAIT, (DWORD_PTR)&pause_parms);
    if (result) {
        LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
    }
}

void S_Music_Unpause()
{
    MCIERROR result;
    MCI_GENERIC_PARMS pause_parms;

    result = mciSendCommandA(
        S.mci_device_id, MCI_RESUME, MCI_WAIT, (DWORD_PTR)&pause_parms);
    if (result) {
        LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
    }
}

bool S_Music_Play(int16_t track)
{
    MCI_SET_PARMS set_parms;
    set_parms.dwTimeFormat = MCI_FORMAT_TMSF;
    if (mciSendCommandA(
            S.mci_device_id, MCI_SET, MCI_SET_TIME_FORMAT,
            (DWORD_PTR)&set_parms)) {
        return false;
    }

    MCI_PLAY_PARMS open_parms;
    open_parms.dwFrom = track;
    open_parms.dwCallback = (DWORD_PTR)TombHWND;

    DWORD_PTR dwFlags = MCI_NOTIFY | MCI_FROM;
    if (track != S.num_tracks) {
        open_parms.dwTo = track + 1;
        dwFlags |= MCI_TO;
    }

    if (mciSendCommandA(
            S.mci_device_id, MCI_PLAY, dwFlags, (DWORD_PTR)&open_parms)) {
        return false;
    }

    return true;
}

bool S_Music_Stop()
{
    MCI_GENERIC_PARMS gen_parms;
    return !mciSendCommandA(
        S.mci_device_id, MCI_STOP, MCI_WAIT, (DWORD_PTR)&gen_parms);
}
