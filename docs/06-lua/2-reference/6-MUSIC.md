---
title: Music
---

## Music module

## Functions

- [lua]`TRX.Music.GetTrack()`  
    Returns current playing track ID, or `nil` if none.
- [lua]`TRX.Music.PlayTrack(id[, opts])`  
    Plays specified track. `opts.mode` selects a play mode constant. Errors if the track ID or mode is invalid.  
    Examples:
    - [lua]`TRX.Music.PlayTrack(1)`  
      Plays track 1 once.
    - [lua]`TRX.Music.PlayTrack(2, { mode = TRX.Music.MPM_LOOPED })`  
      Plays track 2 as a looped track.
- [lua]`TRX.Music.Pause()`  
    Pauses the music.
- [lua]`TRX.Music.Unpause()`  
    Resumes paused music.
- [lua]`TRX.Music.Stop()`  
    Stops all music.

## Play mode constants

- `TRX.Music.MPM_ALWAYS`  
    Plays the track once; after it finishes, any active looped track resumes.
- `TRX.Music.MPM_LOOPED`  
    Plays the track in looped mode continuously. This track becomes the ambient track.
- `TRX.Music.MPM_TRACKED`  
    Plays the track once but prevents retriggering if it's already playing.
- `TRX.Music.MPM_DELAYED`  
    Schedules the track for later playback without starting it immediately.
