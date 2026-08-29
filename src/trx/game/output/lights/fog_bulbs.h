#pragma once

#include <trx/core/colors.h>
#include <trx/core/handle.h>
#include <trx/core/math/types.h>

// Volumetric fog bulbs. The shader evaluates them per fragment for every
// game; what differs is where they come from. TR4 levels carry them as room
// lights and the OG triggers timed ones for such things as underwater flares,
// while a script can ask for one anywhere.

// A fog bulb the level carries. How it looks is a script's to change; where it
// sits and how far it reaches belong to the level.
typedef struct {
    XYZ_32 pos;
    float radius;
    int32_t density;
    RGB_888 color;
    // Whether `color` is a script's choice. A bulb that has none is drawn in
    // the fog color in force, and follows it as that color changes.
    bool has_own_color;
    int16_t room_num;
} FOG_BULB;

void Output_FogBulbs_Reset(void);
// The level's own bulbs alone, for a game letting go of what it baked them
// from.
void Output_FogBulbs_ResetStatic(void);

// Gives a bulb a color of its own, or hands it back to the fog color where
// `color` is null.
void Output_FogBulbs_SetColor(FOG_BULB *bulb, const RGB_888 *color);

// A bulb the level carries, kept until the level is let go of.
void Output_FogBulbs_AddStatic(
    XYZ_32 pos, float radius, int32_t density, int16_t room_num);

// The level's own bulbs, in the order the level carries them.
int32_t Output_FogBulbs_GetStaticCount(void);
FOG_BULB *Output_FogBulbs_GetStatic(int32_t idx);

// A weak reference to one of the level's bulbs. Letting the level's bulbs go
// retires every handle, so one kept across a level change resolves to null
// rather than to another level's bulb at the same index.
TRX_HANDLE Output_FogBulbs_GetStaticHandle(int32_t idx);
FOG_BULB *Output_FogBulbs_FromHandle(TRX_HANDLE handle);

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
