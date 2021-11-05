#include "game/music.h"

#include "config.h"
#include "game/sound.h"
#include "global/vars.h"
#include "specific/music.h"

static int16_t MusicTrackLooped = 0;

bool Music_Init()
{
    return S_Music_Init();
}

bool Music_Play(int16_t track)
{
    if (CurrentLevel == GF.title_level_num && T1MConfig.disable_music_in_menu) {
        return false;
    }

    if (track <= 1) {
        return false;
    }

    if (track >= 57) {
        MusicTrackLooped = track;
    }

    MusicLoop = false;

    if (T1MConfig.fix_secrets_killing_music && track == 13) {
        return Sound_Effect(SFX_SECRET, NULL, SPM_ALWAYS);
    }

    if (track == 0) {
        S_Music_Stop();
        return false;
    }

    if (track == 5) {
        return false;
    }

    MusicTrack = track;
    return S_Music_Play(track);
}

void Music_PlayLooped()
{
    if (MusicLoop && MusicTrackLooped > 0) {
        S_Music_Play(MusicTrackLooped);
    }
}

bool Music_Stop()
{
    MusicTrack = 0;
    MusicTrackLooped = 0;
    MusicLoop = false;
    return S_Music_Stop();
}

void Music_Loop()
{
    MusicLoop = true;
}

void Music_AdjustVolume(int16_t volume)
{
    int16_t volume_raw = volume ? 25 * volume + 5 : 0;
    S_Music_AdjustVolume(volume_raw);
}

void Music_Pause()
{
    S_Music_Pause();
}

void Music_Unpause()
{
    S_Music_Unpause();
}
