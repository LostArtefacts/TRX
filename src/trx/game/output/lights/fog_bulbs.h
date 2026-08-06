#pragma once

#include <trx/core/colors.h>
#include <trx/core/math/types.h>

// Volumetric fog bulbs. The shader evaluates them per fragment for every
// game; what differs is where they come from. TR4 levels carry them as room
// lights and the OG triggers timed ones for such things as underwater flares,
// while a script can ask for one anywhere.

void Output_FogBulbs_Reset(void);
// The level's own bulbs alone, for a game letting go of what it baked them
// from.
void Output_FogBulbs_ResetStatic(void);

// A bulb the level carries, kept until the level is let go of.
void Output_FogBulbs_AddStatic(
    XYZ_F world_pos, float radius, RGB_888 color, int32_t density);

// A bulb asked for this frame, forgotten at the start of the next one. It is
// staged after the level's own, so it draws only where the buffer has room
// left.
void Output_FogBulbs_AddFrame(
    XYZ_32 pos, int32_t radius, int32_t density, RGB_888 color);
void Output_FogBulbs_ResetFrame(void);

// The OG's timed bulb, which grows, holds and fades on its own.
void Output_FogBulbs_AddTimed(
    XYZ_32 pos, int32_t fx_rad, int32_t density, RGB_888 color);
void Output_FogBulbs_Animate(int32_t num_frames);

// Stages every bulb in view space and uploads what the shader reads.
void Output_FogBulbs_PrepareScene(void);

// Forgets what the shader was last given. A new uniform buffer holds nothing
// the last one did, so the next scene has to be uploaded rather than skipped.
void Output_FogBulbs_ResetUploadCache(void);
