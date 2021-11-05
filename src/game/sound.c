#include "game/sound.h"

#include "3dsystem/phd_math.h"
#include "game/game.h"
#include "global/vars.h"
#include "specific/init.h"
#include "specific/sound.h"
#include "util.h"

#include <stddef.h>
#include <stdio.h>

#define SOUND_FLIPFLAG 0x40
#define SOUND_UNFLIPFLAG 0x80
#define SOUND_RANGE 8
#define SOUND_RADIUS (SOUND_RANGE << 10)
#define SOUND_MAX_VOLUME 0x7FFF
#define SOUND_RANGE_MULT_CONSTANT                                              \
    ((int32_t)((SOUND_MAX_VOLUME + 1) / SOUND_RADIUS))
#define SOUND_MAX_VOLUME_CHANGE 0x2000
#define SOUND_MAX_PITCH_CHANGE 10
#define SOUND_NOT_AUDIBLE -1

typedef struct SOUND_SLOT {
    void *handle;
    PHD_3DPOS *pos;
    uint32_t loudness;
    int16_t volume;
    int16_t pan;
    int16_t fxnum;
    int16_t flags;
} SOUND_SLOT;

typedef enum SOUND_MODE {
    SOUND_MODE_WAIT = 0,
    SOUND_MODE_RESTART = 1,
    SOUND_MODE_AMBIENT = 2,
} SOUND_MODE;

typedef enum SOUND_FLAG {
    SOUND_FLAG_UNUSED = 0,
    SOUND_FLAG_USED = 1 << 0,
    SOUND_FLAG_AMBIENT = 1 << 1,
    SOUND_FLAG_RESTARTED = 1 << 2,
} SOUND_FLAG;

static struct {
    SOUND_SLOT sfx_playing[MAX_PLAYING_FX];
    int32_t master_volume_default;
    int16_t ambient_lookup[MAX_AMBIENT_FX];
    int32_t ambient_lookup_idx;
} S = { 0 };

static SOUND_SLOT *Sound_GetSlot(
    int32_t sfx_num, uint32_t loudness, PHD_3DPOS *pos, int16_t mode);
static void Sound_UpdateSlotParams(SOUND_SLOT *slot);
static void Sound_ClearSlot(SOUND_SLOT *slot);
static void Sound_ClearSlotHandles(SOUND_SLOT *slot);

static SOUND_SLOT *Sound_GetSlot(
    int32_t sfx_num, uint32_t loudness, PHD_3DPOS *pos, int16_t mode)
{
    switch (mode) {
    case SOUND_MODE_WAIT:
    case SOUND_MODE_RESTART: {
        SOUND_SLOT *last_free_slot = NULL;
        for (int i = S.ambient_lookup_idx; i < MAX_PLAYING_FX; i++) {
            SOUND_SLOT *result = &S.sfx_playing[i];
            if ((result->flags & SOUND_FLAG_USED) && result->fxnum == sfx_num
                && result->pos == pos) {
                result->flags |= SOUND_FLAG_RESTARTED;
                return result;
            } else if (result->flags == SOUND_FLAG_UNUSED) {
                last_free_slot = result;
            }
        }
        return last_free_slot;
    }

    case SOUND_MODE_AMBIENT:
        for (int i = 0; i < MAX_AMBIENT_FX; i++) {
            if (S.ambient_lookup[i] == sfx_num) {
                SOUND_SLOT *result = &S.sfx_playing[i];
                if (result->flags != SOUND_FLAG_UNUSED
                    && result->loudness <= loudness) {
                    return NULL;
                }
                return result;
            }
        }
        break;
    }

    return NULL;
}

static void Sound_UpdateSlotParams(SOUND_SLOT *slot)
{
    SAMPLE_INFO *s = &SampleInfos[SampleLUT[slot->fxnum]];

    int32_t x = slot->pos->x - Camera.target.x;
    int32_t y = slot->pos->y - Camera.target.y;
    int32_t z = slot->pos->z - Camera.target.z;
    if (ABS(x) > SOUND_RADIUS || ABS(y) > SOUND_RADIUS
        || ABS(z) > SOUND_RADIUS) {
        slot->volume = 0;
        return;
    }

    uint32_t distance = SQUARE(x) + SQUARE(y) + SQUARE(z);
    int32_t volume = s->volume - phd_sqrt(distance) * SOUND_RANGE_MULT_CONSTANT;
    if (volume < 0) {
        slot->volume = 0;
        return;
    }

    if (volume > 0x7FFF) {
        volume = 0x7FFF;
    }

    slot->volume = volume;

    if (!distance || (s->flags & SAMPLE_FLAG_NO_PAN)) {
        slot->pan = 0;
        return;
    }

    int16_t angle = phd_atan(
        slot->pos->z - LaraItem->pos.z, slot->pos->x - LaraItem->pos.x);
    angle -= LaraItem->pos.y_rot + Lara.torso_y_rot + Lara.head_y_rot;
    slot->pan = angle;
}

static void Sound_ClearSlot(SOUND_SLOT *slot)
{
    slot->handle = NULL;
    slot->pos = NULL;
    slot->flags = 0;
    slot->volume = 0;
    slot->pan = 0;
    slot->loudness = SOUND_NOT_AUDIBLE;
    slot->fxnum = -1;
}

static void Sound_ClearSlotHandles(SOUND_SLOT *slot)
{
    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        SOUND_SLOT *rslot = &S.sfx_playing[i];
        if (rslot != slot && rslot->handle == slot->handle) {
            rslot->handle = NULL;
        }
    }
}

bool Sound_Init()
{
    S.master_volume_default = 32;
    return S_Sound_Init();
}

void Sound_UpdateEffects()
{
    if (!SoundIsActive) {
        return;
    }

    Sound_ResetAmbientLoudness();

    for (int i = 0; i < NumberSoundEffects; i++) {
        OBJECT_VECTOR *sound = &SoundEffectsTable[i];
        if (FlipStatus && (sound->flags & SOUND_FLIPFLAG)) {
            Sound_Effect(sound->data, (PHD_3DPOS *)sound, SPM_NORMAL);
        } else if (!FlipStatus && (sound->flags & SOUND_UNFLIPFLAG)) {
            Sound_Effect(sound->data, (PHD_3DPOS *)sound, SPM_NORMAL);
        }
    }

    // XXX: Why are we firing this here? Some of the FX routines rely on the
    // item to be not null!
    if (FlipEffect != -1) {
        EffectRoutines[FlipEffect](NULL);
    }

    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        SOUND_SLOT *slot = &S.sfx_playing[i];
        if (!(slot->flags & SOUND_FLAG_USED)) {
            continue;
        }

        if (slot->flags & SOUND_FLAG_AMBIENT) {
            if (slot->loudness != SOUND_NOT_AUDIBLE && slot->handle) {
                S_Sound_SetPanAndVolume(slot->handle, slot->pan, slot->volume);
            } else {
                if (slot->handle) {
                    S_Sound_StopSample(slot->handle);
                }
                Sound_ClearSlot(slot);
            }
        } else if (S_Sound_SampleIsPlaying(slot->handle)) {
            if (slot->pos != NULL) {
                Sound_UpdateSlotParams(slot);
                if (slot->volume > 0 && slot->handle) {
                    S_Sound_SetPanAndVolume(
                        slot->handle, slot->pan, slot->volume);
                } else {
                    if (slot->handle) {
                        S_Sound_StopSample(slot->handle);
                    }
                    Sound_ClearSlot(slot);
                }
            }
        } else {
            Sound_ClearSlot(slot);
        }
    }
}

bool Sound_Effect(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags)
{
    if (!SoundIsActive) {
        return false;
    }

    if (flags != SPM_ALWAYS
        && (flags & SPM_UNDERWATER)
            != (RoomInfo[Camera.pos.room_number].flags & RF_UNDERWATER)) {
        return false;
    }

    if (SampleLUT[sfx_num] < 0) {
        return false;
    }

    SAMPLE_INFO *s = &SampleInfos[SampleLUT[sfx_num]];
    if (s->randomness && GetRandomDraw() > (int32_t)s->randomness) {
        return false;
    }

    flags = 0;
    int32_t pan = 0x7FFF;
    int32_t mode = s->flags & 3;
    uint32_t distance;
    if (pos) {
        int32_t x = pos->x - Camera.target.x;
        int32_t y = pos->y - Camera.target.y;
        int32_t z = pos->z - Camera.target.z;
        if (ABS(x) > SOUND_RADIUS || ABS(y) > SOUND_RADIUS
            || ABS(z) > SOUND_RADIUS) {
            return false;
        }
        distance = SQUARE(x) + SQUARE(y) + SQUARE(z);
        if (!distance) {
            pan = 0;
        }
    } else {
        distance = 0;
        pan = 0;
    }
    distance = phd_sqrt(distance);

    int32_t volume = s->volume - distance * SOUND_RANGE_MULT_CONSTANT;
    if (s->flags & SAMPLE_FLAG_VOLUME_WIBBLE) {
        volume -= GetRandomDraw() * SOUND_MAX_VOLUME_CHANGE >> 15;
    }

    if (s->flags & SAMPLE_FLAG_NO_PAN) {
        pan = 0;
    }

    if (volume <= 0 && mode != SOUND_MODE_AMBIENT) {
        return false;
    }

    if (pan) {
        int16_t angle =
            phd_atan(pos->z - LaraItem->pos.z, pos->x - LaraItem->pos.x);
        angle -= LaraItem->pos.y_rot + Lara.torso_y_rot + Lara.head_y_rot;
        pan = angle;
    }

    int32_t pitch = 100;
    if (s->flags & SAMPLE_FLAG_PITCH_WIBBLE) {
        pitch += ((GetRandomDraw() * SOUND_MAX_PITCH_CHANGE) / 16384)
            - SOUND_MAX_PITCH_CHANGE;
    }

    int32_t vars = (s->flags >> 2) & 15;
    int32_t sfx_id = s->number;
    if (vars != 1) {
        sfx_id += (GetRandomDraw() * vars) / 0x8000;
    }

    if (volume > SOUND_MAX_VOLUME) {
        volume = SOUND_MAX_VOLUME;
    }

    switch (mode) {
    case SOUND_MODE_WAIT: {
        SOUND_SLOT *fxslot = Sound_GetSlot(sfx_num, 0, pos, mode);
        if (!fxslot) {
            return false;
        }
        if (fxslot->flags & SOUND_FLAG_RESTARTED) {
            fxslot->flags &= 0xFFFF - SOUND_FLAG_RESTARTED;
            return true;
        }
        fxslot->handle = S_Sound_PlaySample(sfx_id, volume, pitch, pan, false);
        if (!fxslot->handle) {
            return false;
        }
        Sound_ClearSlotHandles(fxslot);
        fxslot->flags = flags | SOUND_FLAG_USED;
        fxslot->fxnum = sfx_num;
        fxslot->pos = pos;
        return true;
    }

    case SOUND_MODE_RESTART: {
        SOUND_SLOT *fxslot = Sound_GetSlot(sfx_num, 0, pos, mode);
        if (!fxslot) {
            return false;
        }
        if (fxslot->flags & SOUND_FLAG_RESTARTED) {
            S_Sound_StopSample(fxslot->handle);
            fxslot->handle =
                S_Sound_PlaySample(sfx_id, volume, pitch, pan, false);
            return true;
        }
        fxslot->handle = S_Sound_PlaySample(sfx_id, volume, pitch, pan, false);
        if (!fxslot->handle) {
            return false;
        }
        Sound_ClearSlotHandles(fxslot);
        fxslot->flags = flags | SOUND_FLAG_USED;
        fxslot->fxnum = sfx_num;
        fxslot->pos = pos;
        return true;
    }

    case SOUND_MODE_AMBIENT: {
        uint32_t loudness = distance;
        SOUND_SLOT *fxslot = Sound_GetSlot(sfx_num, loudness, pos, mode);
        if (!fxslot) {
            return false;
        }
        fxslot->pos = pos;
        if (fxslot->flags & SOUND_FLAG_AMBIENT) {
            if (volume > 0) {
                fxslot->loudness = loudness;
                fxslot->pan = pan;
                fxslot->volume = volume;
            } else {
                fxslot->loudness = SOUND_NOT_AUDIBLE;
                fxslot->volume = 0;
            }
            return true;
        }

        if (volume > 0) {
            fxslot->handle =
                S_Sound_PlaySample(sfx_id, volume, pitch, pan, true);
            if (!fxslot->handle) {
                Sound_ClearSlot(fxslot);
                return false;
            }
            Sound_ClearSlotHandles(fxslot);
            fxslot->loudness = loudness;
            fxslot->fxnum = sfx_num;
            fxslot->pan = pan;
            fxslot->volume = volume;
            fxslot->flags |= SOUND_FLAG_AMBIENT | SOUND_FLAG_USED;
            fxslot->pos = pos;
            return true;
        }

        fxslot->loudness = SOUND_NOT_AUDIBLE;
        return true;
    }
    }

    return false;
}

bool Sound_StopEffect(int32_t sfx_num, PHD_3DPOS *pos)
{
    if (!SoundIsActive) {
        return false;
    }

    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        SOUND_SLOT *slot = &S.sfx_playing[i];
        if ((slot->flags & SOUND_FLAG_USED)
            && S_Sound_SampleIsPlaying(slot->handle)) {
            if ((!pos && slot->fxnum == sfx_num)
                || (pos && sfx_num >= 0 && slot->fxnum == sfx_num)
                || (pos && sfx_num < 0)) {
                S_Sound_StopSample(slot->handle);
                Sound_ClearSlot(slot);
                return true;
            }
        }
    }

    return false;
}

void Sound_ResetEffects()
{
    if (!SoundIsActive) {
        return;
    }
    Sound_MasterVolume = S.master_volume_default;

    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        Sound_ClearSlot(&S.sfx_playing[i]);
    }

    Sound_StopAllSamples();

    S.ambient_lookup_idx = 0;

    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (SampleLUT[i] < 0) {
            continue;
        }
        SAMPLE_INFO *s = &SampleInfos[SampleLUT[i]];
        if (s->volume < 0) {
            S_ExitSystemFmt(
                "sample info for effect %d has incorrect volume(%d)", i,
                s->volume);
        }

        int32_t mode = s->flags & 3;
        if (mode == SOUND_MODE_AMBIENT) {
            if (S.ambient_lookup_idx >= MAX_AMBIENT_FX) {
                S_ExitSystem("Ran out of ambient fx slots in "
                             "Sound_ResetEffects()");
            }
            S.ambient_lookup[S.ambient_lookup_idx] = i;
            S.ambient_lookup_idx++;
        }
    }
}

void Sound_ResetAmbientLoudness()
{
    if (!SoundIsActive) {
        return;
    }

    for (int i = 0; i < S.ambient_lookup_idx; i++) {
        SOUND_SLOT *slot = &S.sfx_playing[i];
        slot->loudness = SOUND_NOT_AUDIBLE;
    }
}

void Sound_StopAmbientSounds()
{
    if (!SoundIsActive) {
        return;
    }

    for (int i = 0; i < S.ambient_lookup_idx; i++) {
        SOUND_SLOT *slot = &S.sfx_playing[i];
        if (S_Sound_SampleIsPlaying(slot->handle)) {
            S_Sound_StopSample(slot->handle);
            Sound_ClearSlot(slot);
        }
    }
}

void Sound_LoadSamples(char **sample_pointers, int32_t num_samples)
{
    S_Sound_LoadSamples(sample_pointers, num_samples);
}

void Sound_StopAllSamples()
{
    S_Sound_StopAllSamples();
}

void Sound_AdjustMasterVolume(int8_t volume)
{
    int8_t raw_volume = volume ? 6 * volume + 3 : 0;
    S.master_volume_default = raw_volume & 0x3F;
    Sound_MasterVolume = raw_volume & 0x3F;
}
