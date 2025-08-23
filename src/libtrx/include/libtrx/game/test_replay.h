#pragma once

#include "./shell/args.h"

// Test replay: a module to simulate game playthroughs.
// ============================================================================

// Initialize test replay for playback mode.
// @param path  Path to the recording to play from.
// @return      Parsed shell arguments from the replay file, or nullptr on
//              error.
SHELL_ARGS *TestReplay_Open(const char *path);

// Shutdown test replay.
void TestReplay_Close(void);

// Return whether the replay mode is currently active.
bool TestReplay_IsOpened(void);

// Run all events associated with the given frame.
void TestReplay_RunFrame(void);
