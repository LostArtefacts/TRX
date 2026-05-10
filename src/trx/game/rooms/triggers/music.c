#include <trx/config.h>
#include <trx/game/game.h>
#include <trx/game/gym.h>
#include <trx/game/music.h>
#include <trx/game/rooms.h>
#include <trx/version.h>

static bool M_IsSpeechTrack(const MUSIC_ID track_id)
{
    switch (Music_FromGameID(track_id)) {
    case MX_BALDY_SPEECH:
    case MX_COWBOY_SPEECH:
    case MX_LARSON_SPEECH:
    case MX_NATLA_SPEECH:
    case MX_PIERRE_SPEECH:
    case MX_SKATEKID_SPEECH:
        return true;
    default:
        return false;
    }
}

static void M_Handle(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    MUSIC_ID track_id = (MUSIC_ID)(intptr_t)cmd->parameter;

    if (track_id == (MUSIC_ID)0
        && (trigger->type == TT_ANTIPAD || trigger->type == TT_ANTITRIGGER)) {
        Music_Stop();
        return;
    }

    if (track_id <= Music_ToGameID(MX_UNUSED_1) || track_id >= MAX_MUSIC_TRACKS
        || (Game_IsInGym() && !Gym_CanPlayMusicTrack(&track_id))) {
        return;
    }

    uint16_t flags = Music_GetTrackFlags(track_id);
    MUSIC_PLAY_MODE play_mode = MPM_NO_REPEAT;
    if (g_Config.audio.fix_speeches_killing_music
        && M_IsSpeechTrack(track_id)) {
        play_mode = MPM_OVERLAY;
    }

    // TODO: consolidate
    if (g_TRVersion == 1) {
        if ((flags & IF_ONE_SHOT) != 0) {
            return;
        }

        if (trigger->type == TT_SWITCH) {
            flags ^= trigger->mask;
        } else if (
            trigger->type == TT_ANTIPAD || trigger->type == TT_ANTITRIGGER) {
            flags &= -1 - trigger->mask;
        } else if (trigger->mask) {
            flags |= trigger->mask;
        }

        if ((flags & IF_CODE_BITS) == IF_CODE_BITS) {
            if (trigger->one_shot) {
                flags |= IF_ONE_SHOT;
            }
            Music_Play_Direct(track_id, play_mode);
        } else {
            Music_StopTrack_Direct(track_id);
        }
    } else {
        if (trigger->type != TT_SWITCH) {
            const int32_t code = trigger->mask;
            if ((flags & code) != 0) {
                return;
            }
            if (trigger->one_shot) {
                flags |= code;
            }
        }

        if (trigger->timer == 0 || g_TRVersion != 2) {
            Music_Play_Direct(track_id, play_mode);
            goto finish;
        }

        if (track_id != Music_GetDelayedTrack()) {
            Music_Play_Direct(track_id, MPM_DELAY);
            flags = (flags & 0xFF00) | ((LOGIC_FPS * trigger->timer) & 0xFF);
            goto finish;
        }

        int32_t timer = flags & 0xFF;
        if (timer == 0) {
            goto finish;
        }

        timer--;
        if (timer == 0) {
            Music_Play_Direct(track_id, play_mode);
        }
        flags = (flags & 0xFF00) | (timer & 0xFF);
    }

finish:
    Music_SetTrackFlags(track_id, flags);
}

REGISTER_TRIGGER_HANDLER(TO_MUSIC, M_Handle)
