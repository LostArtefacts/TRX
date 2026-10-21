#pragma once

// Registers render-policy overrides for the O_BINOCULAR_GFX and O_TARGET_GFX
// overlay meshes, so that they always render as opaque black masks like in
// the original game. Must run before OutputSource_Objects_ObserveLevelLoad.
void Output_BinocularMask_ObserveLevelLoad(void);
void Output_BinocularMask_ObserveLevelUnload(void);
