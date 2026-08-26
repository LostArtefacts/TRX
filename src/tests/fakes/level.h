#pragma once

// The level's world, which the engine reads once the level file is loaded.
// Loaded is the default: a test that wants what happens before the level is
// read says so.

void FakeLevel_SetWorldLoaded(bool loaded);
