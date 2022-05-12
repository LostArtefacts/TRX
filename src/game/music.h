#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

// Initializes music mixer.
bool Music_Init(void);

// Stops playing current track and plays a single track. Once playback is done,
// if there is an active looped track, the playback resumes from the start of
// the looped track.
bool Music_Play(int16_t track);

// Stops playing current track and plays a single track. Activates looped
// playback for the chosen track.
bool Music_PlayLooped(int16_t track);

// Stops any music, whether looped or active speech.
void Music_Stop(void);

// Sets the game volume. Value can be 0-255.
void Music_SetVolume(int16_t volume);

// Pauses the music.
void Music_Pause(void);

// Unpauses the music.
void Music_Unpause(void);

// Returns currently playing track. If there is a track playing "over" a looped
// track, returns the "overriding" track number.
int16_t Music_CurrentTrack(void);
