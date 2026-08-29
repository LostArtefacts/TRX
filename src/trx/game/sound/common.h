#pragma once

#include <trx/core/handle.h>
#include <trx/core/math.h>
#include <trx/core/result.h>
#include <trx/game/sound/enum.h>
#include <trx/game/sound/ids.h>
#include <trx/game/sound/types.h>
#include <trx/game/types.h>

#include <stddef.h>

#define SOUND_DEFAULT_PITCH 0x10000

// Starts the sound system and opens the audio device, reporting a device that
// will not open.
RESULT Sound_Init(void);
bool Sound_IsInitialised(void);

void Sound_SetMasterVolume(float volume);
uint8_t Sound_GetReverbType(void);
void Sound_SetReverbType(uint8_t reverb_type);

void Sound_ResetSamples(void);

RESULT Sound_LoadSampleData(
    int32_t sample_data_id, const char *sample_data, size_t size);

void Sound_InitialiseSources(int32_t num_sources);
int32_t Sound_GetSourceCount(void);
OBJECT_VECTOR *Sound_GetSource(int32_t source_idx);
void Sound_ResetSources(void);

// Reserve a contiguous block of sample data IDs for loading audio samples.
// Returns the starting sample_data_id for the reserved block of size how_many.
int32_t Sound_ReserveSampleData(int32_t index, int32_t how_many);

// Return the SAMPLE_INFO for the given SAMPLE_SLOT, or nullptr if none exists.
SAMPLE_INFO *Sound_GetSample(SAMPLE_SLOT slot);

// Return or create the SAMPLE_INFO for the given SAMPLE_SLOT.
SAMPLE_INFO *Sound_GetOrCreateSample(SAMPLE_SLOT slot);

// Return true if a SAMPLE_INFO exists for the given SAMPLE_SLOT.
bool Sound_IsAvailableBySlot(SAMPLE_SLOT slot);
bool Sound_IsAvailable(SAMPLE_ID id);

// Return the highest direct SAMPLE_SLOT loaded for playback.
// Returns SFX_INVALID if no samples are available.
SAMPLE_SLOT Sound_GetMaxSlot(void);

// Reports each sample that the level declares without audio. Such samples
// play silently, and the play call cannot report them because it runs every
// frame, so this function reports them at the end of the level load.
RESULT Sound_CheckSamples(void);

// Play a sample with the given number. pos is an optional world position to
// play the sound at, and can be nullptr. Returns the active-sound slot the
// sample plays in, or -1 when it does not play.
int32_t Sound_EffectBySlot(SAMPLE_SLOT slot, const XYZ_32 *pos, uint32_t flags);
int32_t Sound_Effect(SAMPLE_ID id, const XYZ_32 *pos, uint32_t flags);

void Sound_StopEffectBySlot(SAMPLE_SLOT slot);
void Sound_StopEffect(SAMPLE_ID id);

void Sound_ResetAmbient(void);
void Sound_UpdateEffects(void);

void Sound_PauseAll(void);
void Sound_UnpauseAll(void);
void Sound_StopAll(void);

// The number of active-sound slots. A slot addresses a playing voice whether it
// holds one or not, so this is fixed rather than a count of what plays now.
int32_t Sound_GetActiveSlotCount(void);

// Fills the sample id the slot is playing. Returns false when the slot is idle.
bool Sound_GetActiveSlot(int32_t slot, SAMPLE_SLOT *out_sample_id);

// A handle to the voice currently in the slot, and the sample a handle still
// names or false. Each play hands the slot to a new voice; the generation is
// what keeps a handle from addressing the voice that later took its slot.
TRX_HANDLE Sound_GetActiveSlotHandle(int32_t slot);
bool Sound_ResolveActiveSlot(TRX_HANDLE handle, SAMPLE_SLOT *out_sample_id);

// Stops, pauses or resumes the voice in a slot.
void Sound_StopActiveSlot(int32_t slot);
void Sound_PauseActiveSlot(int32_t slot);
void Sound_UnpauseActiveSlot(int32_t slot);
