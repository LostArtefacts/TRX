#include <trx/game/music.h>
#include <trx/game/rooms.h>

// Maps a floordata trigger onto a MUSIC_TRIGGER at the rooms->music
// boundary; the music mechanic only ever sees the kind.
static void M_Handle(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const MUSIC_SLOT track_id = (MUSIC_SLOT)(intptr_t)cmd->parameter;

    MUSIC_TRIGGER_KIND kind = MUSIC_TRIGGER_NORMAL;
    if (trigger->type == TT_SWITCH) {
        kind = MUSIC_TRIGGER_SWITCH;
    } else if (Room_IsAntiTrigger(trigger->type)) {
        kind = MUSIC_TRIGGER_ANTI;
    }

    const MUSIC_TRIGGER music_trigger = {
        .kind = kind,
        .mask = trigger->mask,
        .timer = trigger->timer,
        .one_shot = trigger->one_shot,
    };
    Music_Trigger(track_id, &music_trigger);
}

REGISTER_TRIGGER_HANDLER(TO_MUSIC, M_Handle)
