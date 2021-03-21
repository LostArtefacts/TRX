#include "specific/sndpc.h"

#include "game/sound.h"
#include "game/vars.h"

#include "config.h"
#include "util.h"

#include <windows.h>

void S_CDVolume(int16_t volume)
{
    TRACE("%d", volume);
    int32_t volume_aux = volume * 0xFFFF / 0xFF;
    volume_aux |= volume_aux << 16;
    auxSetVolume(AuxDeviceID, volume_aux);
}

int32_t S_CDPlay(int16_t track)
{
    TRACE("%d", track);

    if (T1MConfig.fix_secrets_killing_music && track == 13) {
        SoundEffect(SFX_SECRET, NULL, SPM_ALWAYS);
        return 1;
    }

    if (track == 0) {
        S_CDStop();
        return 0;
    }

    if (track == 5) {
        return 0;
    }

    CDTrack = track;
    return CDPlay(track);
}

int32_t S_StartSyncedAudio(int16_t track)
{
    return S_CDPlay(track);
}

int32_t S_CDStop()
{
    TRACE("");

    CDTrack = 0;
    CDTrackLooped = 0;
    CDLoop = 0;

    MCI_GENERIC_PARMS gen_parms;
    return !mciSendCommandA(
        MCIDeviceID, MCI_STOP, MCI_WAIT, (DWORD_PTR)&gen_parms);
}

int32_t CDPlay(int16_t track)
{
    TRACE("%d", track);

    if (track < 2) {
        return 0;
    }

    if (track >= 57) {
        CDTrackLooped = track;
    }

    CDLoop = 0;

    uint32_t volume = OptionMusicVolume * 0xFFFF / 10;
    volume |= volume << 16;
    auxSetVolume(AuxDeviceID, volume);

    MCI_SET_PARMS set_parms;
    set_parms.dwTimeFormat = MCI_FORMAT_TMSF;
    if (mciSendCommandA(
            MCIDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&set_parms)) {
        return 0;
    }

    MCI_PLAY_PARMS open_parms;
    open_parms.dwFrom = track;
    open_parms.dwCallback = TombHWND;

    DWORD_PTR dwFlags = MCI_NOTIFY | MCI_FROM;
    if (track != CDNumTracks) {
        open_parms.dwTo = track + 1;
        dwFlags |= MCI_TO;
    }

    if (mciSendCommandA(
            MCIDeviceID, MCI_PLAY, dwFlags, (DWORD_PTR)&open_parms)) {
        return 0;
    }

    return 1;
}

int32_t CDPlayLooped()
{
    TRACE("");

    if (CDLoop && CDTrackLooped > 0) {
        CDPlay(CDTrackLooped);
        return 0;
    }

    return CDLoop;
}

void T1MInjectSpecificSndPC()
{
    INJECT(0x00437FB0, CDPlay);
    INJECT(0x004380C0, CDPlayLooped);
    INJECT(0x00438D40, S_CDPlay);
    INJECT(0x00438E40, S_CDStop);
    INJECT(0x00439030, S_StartSyncedAudio);

    // NOTE: this is a nullsub in OG and is called in many different places
    // for many different purposes so it's not injected.
    // INJECT(0x00437F30, S_CDVolume);
}
