#include "game/music.h"

#include "config.h"
#include "game/sound.h"
#include "global/vars.h"
#include "specific/music.h"

static int16_t MusicTrackLooped = 0;

int32_t Music_Init()
{
    return S_Music_Init();
}

int32_t Music_Play(int16_t track)
{
    if (CurrentLevel == GF.title_level_num && T1MConfig.disable_music_in_menu) {
        return 0;
    }

    if (track <= 1) {
        return 0;
    }

    if (track >= 57) {
        MusicTrackLooped = track;
    }

    MusicLoop = false;

    if (T1MConfig.fix_secrets_killing_music && track == 13) {
        Sound_Effect(SFX_SECRET, NULL, SPM_ALWAYS);
        return 1;
    }

    if (track == 0) {
        S_Music_Stop();
        return 0;
    }

    if (track == 5) {
        return 0;
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

int32_t Music_Stop()
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
    S_Music_AdjustVolume(volume);
}

void Music_Pause()
{
    S_Music_Pause();
}

void Music_Unpause()
{
    S_Music_Unpause();
}
