#include <trx/game/ui/elements/loading_bar.h>

#include <trx/game/ui/elements/bar.h>
#include <trx/game/ui/scaler.h>

void UI_LoadingBar(const UI_LOADING_BAR_SETTINGS settings)
{
    UI_Bar((UI_BAR_SETTINGS) {
        .type = UI_BAR_PROGRESS,
        .w = settings.w,
        .h = settings.h > 0.0f
            ? settings.h
            : UI_LOADING_BAR_HEIGHT * UI_Scaler_GetScale(UI_SCALER_TARGET_BAR),
        .value = settings.progress * 100.0f,
        .max_value = 100,
        .force_smooth = true,
        .absolute_size = true,
        .border_color = { 0xFF, 0xFF, 0xFF, 0xFF },
    });
}
