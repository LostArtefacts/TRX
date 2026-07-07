#pragma once

// Registers render-policy overrides for the O_BINOCULAR_GFX overlay mesh, so
// that it always renders as an opaque black mask like in the original game.
// Must run before OutputSource_Objects_ObserveLevelLoad.
void Output_BinocularMask_ObserveLevelLoad(void);
void Output_BinocularMask_ObserveLevelUnload(void);
