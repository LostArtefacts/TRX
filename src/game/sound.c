#include "game/sound.h"

#include "3dsystem/phd_math.h"
#include "game/game.h"
#include "global/vars.h"
#include "specific/init.h"
#include "specific/sndpc.h"
#include "util.h"

#include <stddef.h>
#include <stdio.h>

#define FLIPFLAG 0x40
#define UNFLIPFLAG 0x80
#define SOUND_RANGE 8
#define SOUND_RADIUS (SOUND_RANGE << 10)
#define SOUND_RANGE_MULT_CONSTANT ((int32_t)(32768 / SOUND_RADIUS))
#define MAX_VOLUME_CHANGE 0x2000
#define MAX_PITCH_CHANGE 10
#define MN_NOT_AUDIBLE -1

typedef enum SOUND_MODE {
    SOUND_WAIT = 0,
    SOUND_RESTART = 1,
    SOUND_AMBIENT = 2,
} SOUND_MODE;

typedef enum SOUND_FLAG {
    MN_FX_UNUSED = 0,
    MN_FX_USED = 1 << 0,
    MN_FX_AMBIENT = 1 << 1,
    MN_FX_RESTARTED = 1 << 2,
    MN_FX_NO_REVERB = 1 << 3,
} SOUND_FLAG;

typedef enum SAMPLE_FLAG {
    NO_PAN = 1 << 12,
    PITCH_WIBBLE = 1 << 13,
    VOLUME_WIBBLE = 1 << 14,
} SAMPLE_FLAG;

static MN_SFX_PLAY_INFO SFXPlaying[MAX_PLAYING_FX] = { 0 };
static int32_t Sound_MasterVolumeDefault = 32;
static int16_t MnAmbientLookup[MAX_AMBIENT_FX] = { -1 };
static int32_t MnAmbientLookupIdx = 0;

static MN_SFX_PLAY_INFO *Sound_GetSlot(
    int32_t sfx_num, uint32_t loudness, PHD_3DPOS *pos, int16_t mode);
static void Sound_UpdateSlotParams(MN_SFX_PLAY_INFO *slot);
static void Sound_ClearSlot(MN_SFX_PLAY_INFO *slot);
static void Sound_ClearSlotHandles(MN_SFX_PLAY_INFO *slot);

void Sound_UpdateEffects()
{
    if (!SoundIsActive) {
        return;
    }

    Sound_ResetAmbientLoudness();

    for (int i = 0; i < NumberSoundEffects; i++) {
        OBJECT_VECTOR *sound = &SoundEffectsTable[i];
        if (FlipStatus && (sound->flags & FLIPFLAG)) {
            Sound_Effect(sound->data, (PHD_3DPOS *)sound, SPM_NORMAL);
        } else if (!FlipStatus && (sound->flags & UNFLIPFLAG)) {
            Sound_Effect(sound->data, (PHD_3DPOS *)sound, SPM_NORMAL);
        }
    }

    // XXX: Why are we firing this here? Some of the FX routines rely on the
    // item to be not null!
    if (FlipEffect != -1) {
        EffectRoutines[FlipEffect](NULL);
    }

    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        MN_SFX_PLAY_INFO *slot = &SFXPlaying[i];
        if (slot->mn_flags & MN_FX_USED) {
            if (slot->mn_flags & MN_FX_AMBIENT) {
                if (slot->loudness != MN_NOT_AUDIBLE
                    && slot->handle != SOUND_INVALID_HANDLE) {
                    S_SoundSetPanAndVolume(
                        slot->handle, slot->pan, slot->volume);
                } else {
                    if (slot->handle != SOUND_INVALID_HANDLE) {
                        S_SoundStopSample(slot->handle);
                    }
                    Sound_ClearSlot(slot);
                }
            } else if (S_SoundSampleIsPlaying(slot->handle)) {
                if (slot->pos != NULL) {
                    Sound_UpdateSlotParams(slot);
                    if (slot->volume > 0
                        && slot->handle != SOUND_INVALID_HANDLE) {
                        S_SoundSetPanAndVolume(
                            slot->handle, slot->pan, slot->volume);
                    } else {
                        if (slot->handle != SOUND_INVALID_HANDLE) {
                            S_SoundStopSample(slot->handle);
                        }
                        Sound_ClearSlot(slot);
                    }
                }
            } else {
                Sound_ClearSlot(slot);
            }
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
        flags = MN_FX_NO_REVERB;
    }
    distance = phd_sqrt(distance);

    int32_t volume = s->volume - distance * SOUND_RANGE_MULT_CONSTANT;
    if (s->flags & VOLUME_WIBBLE) {
        volume -= GetRandomDraw() * MAX_VOLUME_CHANGE >> 15;
    }

    if (s->flags & NO_PAN) {
        pan = 0;
    }

    if (volume <= 0 && mode != SOUND_AMBIENT) {
        return false;
    }

    if (pan) {
        int16_t angle =
            phd_atan(pos->z - LaraItem->pos.z, pos->x - LaraItem->pos.x);
        angle -= LaraItem->pos.y_rot + Lara.torso_y_rot + Lara.head_y_rot;
        pan = angle;
    }

    int32_t pitch = 100;
    if (s->flags & PITCH_WIBBLE) {
        pitch +=
            ((GetRandomDraw() * MAX_PITCH_CHANGE) / 16384) - MAX_PITCH_CHANGE;
    }

    int32_t vars = (s->flags >> 2) & 15;
    int32_t sfx_id = s->number;
    if (vars != 1) {
        sfx_id += (GetRandomDraw() * vars) / 0x8000;
    }

    if (volume > 0x7FFF) {
        volume = 0x7FFF;
    }

    switch (mode) {
    case SOUND_WAIT: {
        MN_SFX_PLAY_INFO *fxslot = Sound_GetSlot(sfx_num, 0, pos, mode);
        if (!fxslot) {
            return false;
        }
        if (fxslot->mn_flags & MN_FX_RESTARTED) {
            fxslot->mn_flags &= 0xFFFF - MN_FX_RESTARTED;
            return true;
        }
        fxslot->handle = S_SoundPlaySample(sfx_id, volume, pitch, pan);
        if (fxslot->handle == SOUND_INVALID_HANDLE) {
            return false;
        }
        Sound_ClearSlotHandles(fxslot);
        fxslot->mn_flags = flags | MN_FX_USED;
        fxslot->fxnum = sfx_num;
        fxslot->pos = pos;
        return true;
    }

    case SOUND_RESTART: {
        MN_SFX_PLAY_INFO *fxslot = Sound_GetSlot(sfx_num, 0, pos, mode);
        if (!fxslot) {
            return false;
        }
        if (fxslot->mn_flags & MN_FX_RESTARTED) {
            S_SoundStopSample(fxslot->handle);
            fxslot->handle = S_SoundPlaySample(sfx_id, volume, pitch, pan);
            return true;
        }
        fxslot->handle = S_SoundPlaySample(sfx_id, volume, pitch, pan);
        if (fxslot->handle == SOUND_INVALID_HANDLE) {
            return false;
        }
        Sound_ClearSlotHandles(fxslot);
        fxslot->mn_flags = flags | MN_FX_USED;
        fxslot->fxnum = sfx_num;
        fxslot->pos = pos;
        return true;
    }

    case SOUND_AMBIENT: {
        uint32_t loudness = distance;
        MN_SFX_PLAY_INFO *fxslot = Sound_GetSlot(sfx_num, loudness, pos, mode);
        if (!fxslot) {
            return false;
        }
        fxslot->pos = pos;
        if (fxslot->mn_flags & MN_FX_AMBIENT) {
            if (volume > 0) {
                fxslot->loudness = loudness;
                fxslot->pan = pan;
                fxslot->volume = volume;
            } else {
                fxslot->loudness = MN_NOT_AUDIBLE;
                fxslot->volume = 0;
            }
            return true;
        }

        if (volume > 0) {
            fxslot->handle =
                S_SoundPlaySampleLooped(sfx_id, volume, pitch, pan);
            if (fxslot->handle == SOUND_INVALID_HANDLE) {
                Sound_ClearSlot(fxslot);
                return false;
            }
            Sound_ClearSlotHandles(fxslot);
            fxslot->loudness = loudness;
            fxslot->fxnum = sfx_num;
            fxslot->pan = pan;
            fxslot->volume = volume;
            fxslot->mn_flags |= MN_FX_AMBIENT | MN_FX_USED;
            fxslot->pos = pos;
            return true;
        }

        fxslot->loudness = MN_NOT_AUDIBLE;
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
        MN_SFX_PLAY_INFO *slot = &SFXPlaying[i];
        if ((slot->mn_flags & MN_FX_USED)
            && S_SoundSampleIsPlaying(slot->handle)) {
            if ((!pos && slot->fxnum == sfx_num)
                || (pos && sfx_num >= 0 && slot->fxnum == sfx_num)
                || (pos && sfx_num < 0)) {
                S_SoundStopSample(slot->handle);
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
    Sound_MasterVolume = Sound_MasterVolumeDefault;

    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        Sound_ClearSlot(&SFXPlaying[i]);
    }

    S_SoundStopAllSamples();

    MnAmbientLookupIdx = 0;

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

        if ((s->flags & 3) == SOUND_AMBIENT) {
            if (MnAmbientLookupIdx >= MAX_AMBIENT_FX) {
                S_ExitSystem("Ran out of ambient fx slots in "
                             "Sound_ResetEffects()");
            }
            MnAmbientLookup[MnAmbientLookupIdx] = i;
            MnAmbientLookupIdx++;
        }
    }
}

static MN_SFX_PLAY_INFO *Sound_GetSlot(
    int32_t sfx_num, uint32_t loudness, PHD_3DPOS *pos, int16_t mode)
{
    switch (mode) {
    case SOUND_WAIT:
    case SOUND_RESTART: {
        MN_SFX_PLAY_INFO *last_free_slot = NULL;
        for (int i = MnAmbientLookupIdx; i < MAX_PLAYING_FX; i++) {
            MN_SFX_PLAY_INFO *result = &SFXPlaying[i];
            if ((result->mn_flags & MN_FX_USED) && result->fxnum == sfx_num
                && result->pos == pos) {
                result->mn_flags |= MN_FX_RESTARTED;
                return result;
            } else if (result->mn_flags == MN_FX_UNUSED) {
                last_free_slot = result;
            }
        }
        return last_free_slot;
    }

    case SOUND_AMBIENT:
        for (int i = 0; i < MAX_AMBIENT_FX; i++) {
            if (MnAmbientLookup[i] == sfx_num) {
                MN_SFX_PLAY_INFO *result = &SFXPlaying[i];
                if (result->mn_flags != MN_FX_UNUSED
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

void Sound_ResetAmbientLoudness()
{
    if (!SoundIsActive) {
        return;
    }

    for (int i = 0; i < MnAmbientLookupIdx; i++) {
        MN_SFX_PLAY_INFO *slot = &SFXPlaying[i];
        slot->loudness = MN_NOT_AUDIBLE;
    }
}

void Sound_StopAmbientSounds()
{
    if (!SoundIsActive) {
        return;
    }

    for (int i = 0; i < MnAmbientLookupIdx; i++) {
        MN_SFX_PLAY_INFO *slot = &SFXPlaying[i];
        if (S_SoundSampleIsPlaying(slot->handle)) {
            S_SoundStopSample(slot->handle);
            Sound_ClearSlot(slot);
        }
    }
}

static void Sound_UpdateSlotParams(MN_SFX_PLAY_INFO *slot)
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

    if (!distance || (s->flags & NO_PAN)) {
        slot->pan = 0;
        return;
    }

    int16_t angle = phd_atan(
        slot->pos->z - LaraItem->pos.z, slot->pos->x - LaraItem->pos.x);
    angle -= LaraItem->pos.y_rot + Lara.torso_y_rot + Lara.head_y_rot;
    slot->pan = angle;
}

void Sound_AdjustMasterVolume(int8_t volume)
{
    Sound_MasterVolumeDefault = volume & 0x3F;
    Sound_MasterVolume = volume & 0x3F;
}

static void Sound_ClearSlot(MN_SFX_PLAY_INFO *slot)
{
    slot->handle = SOUND_INVALID_HANDLE;
    slot->pos = NULL;
    slot->mn_flags = 0;
    slot->volume = 0;
    slot->pan = 0;
    slot->loudness = MN_NOT_AUDIBLE;
    slot->fxnum = -1;
}

static void Sound_ClearSlotHandles(MN_SFX_PLAY_INFO *slot)
{
    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        MN_SFX_PLAY_INFO *rslot = &SFXPlaying[i];
        if (rslot != slot && rslot->handle == slot->handle) {
            rslot->handle = SOUND_INVALID_HANDLE;
        }
    }
}
