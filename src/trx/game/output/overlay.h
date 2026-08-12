#pragma once

#include <trx/config/enum.h>
#include <trx/core/colors.h>

typedef struct {
    float opacity;
    float desaturation; // 0 = off
    RGB_F tint; // COLOR_RGB_F_WHITE = no tint
} OUTPUT_SNAPSHOT_SETTINGS;

void Output_Overlay_DrawPattern(bool wave);
void Output_Overlay_DrawPatternOpacity(bool wave, float opacity);
void Output_Overlay_DrawBlackRectangle(float opacity, bool post_ui);

// The cinematic bars across the top and bottom of the view, as a fraction of
// the screen height each. 0 removes them. The depth is remembered, so whoever
// wants the bars says so once and clears them once, rather than asking on
// every frame; drawing happens as part of the frame's overlay.
void Output_Overlay_SetLetterbox(float ratio);
float Output_Overlay_GetLetterbox(void);
void Output_Overlay_DrawLetterbox(void);

// How black the view is over the top, from 0 to 1, for whoever is framing a
// moment rather than a whole scene. Remembered, and drawn with the bars.
void Output_Overlay_SetFade(float opacity);
float Output_Overlay_GetFade(void);
bool Output_Overlay_LoadImage(const char *file_name);
void Output_Overlay_DrawImage(const char *file_name);
void Output_Overlay_DrawImageBilinear(const char *file_name);
void Output_Overlay_DrawImageMono(const char *file_name, float intensity);
// Marks the start of a new frame, so that a transition captured before it is
// drawn once no matter how many times the frame is flushed.
void Output_Overlay_BeginFrame(void);
void Output_Overlay_CaptureSnapshot(void);
void Output_Overlay_CaptureGameSnapshot(void);
void Output_Overlay_DrawSnapshot(float opacity);
void Output_Overlay_DrawSnapshotEx(const OUTPUT_SNAPSHOT_SETTINGS *settings);
void Output_Overlay_DrawBackground(
    BACKGROUND_TYPE style, float opacity, const char *image_path);
void Output_Overlay_BeginTransitionFadeOut(float duration, float start);
