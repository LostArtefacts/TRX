#include "game/music.h"

#include "config.h"
#include "game/sound.h"
#include "global/vars.h"
#include "specific/music.h"

static struct {
    bool loop;
    int16_t track;
    int16_t track_looped;
} S = { 0 };

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
        S.track_looped = track;
    }

    S.loop = false;

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

    S.track = track;
    return S_Music_Play(track);
}

void Music_PlayLooped()
{
    if (S.loop && S.track_looped > 0) {
        S_Music_Play(S.track_looped);
    }
}

bool Music_Stop()
{
    S.track = 0;
    S.track_looped = 0;
    S.loop = false;
    return S_Music_Stop();
}

void Music_Loop()
{
    S.loop = true;
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

int16_t Music_CurrentTrack()
{
    return S.track;
}
