#ifndef T1M_GAME_MUSIC_H
#define T1M_GAME_MUSIC_H

#include <stdbool.h>
#include <stdint.h>

#include "global/types.h"

// Initializes music mixer.
bool Music_Init();

// Stops playing current track and plays a single track. Once playback is done,
// if there is an active looped track, the playback resumes from the start of
// the looped track.
bool Music_Play(int16_t track);

// Stops playing current track and plays a single track. Activates looped
// playback for the chosen track.
void Music_PlayLooped(int16_t track);

// Stops any music, whether looped or active speech.
bool Music_Stop();

// Sets the game volume. Value can be 0-255.
void Music_SetVolume(int16_t volume);

// Pauses the music.
void Music_Pause();

// Unpauses the music.
void Music_Unpause();

// Returns currently playing track. If there is a track playing "over" a looped
// track, returns the "overriding" track number.
int16_t Music_CurrentTrack();

#endif
