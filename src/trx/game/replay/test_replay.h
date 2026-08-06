#pragma once

#include <trx/game/shell/args.h>

// Test replay: a module to simulate game playthroughs.
// ============================================================================

// Initialize test replay for playback mode.
// @param path  Path to the recording to play from.
// @return      Parsed shell arguments from the replay file, or nullptr on
//              error.
SHELL_ARGS *TestReplay_Open(const char *path);

// Executes the initial configuration headers after the system is done
// initializing.
void TestReplay_Start(void);

// Apply the settings the recording named that no option answered to when its
// header was read: the ones a game's script declares. Called once that script
// has run.
void TestReplay_ApplyDeferredConfig(void);

// Shutdown test replay.
void TestReplay_Close(void);

// Return whether the replay mode is currently active.
bool TestReplay_IsOpened(void);

// Run all events associated with the given frame.
void TestReplay_RunFrame(void);

// Returns -1 when replay does not override process exit code.
// Otherwise 0/1 for replay-driven pass/fail status.
int32_t TestReplay_GetExitCodeOverride(void);
