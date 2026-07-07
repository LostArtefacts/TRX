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
bool Output_Overlay_LoadImage(const char *file_name);
void Output_Overlay_DrawImage(const char *file_name);
void Output_Overlay_DrawImageBilinear(const char *file_name);
void Output_Overlay_DrawImageMono(const char *file_name, float intensity);
void Output_Overlay_CaptureSnapshot(void);
void Output_Overlay_CaptureGameSnapshot(void);
void Output_Overlay_DrawSnapshot(float opacity);
void Output_Overlay_DrawSnapshotEx(const OUTPUT_SNAPSHOT_SETTINGS *settings);
void Output_Overlay_DrawBackground(
    BACKGROUND_TYPE style, float opacity, const char *image_path);
void Output_Overlay_BeginTransitionFadeOut(float duration, float start);
