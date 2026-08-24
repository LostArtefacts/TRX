#pragma once

// Owns overlay state that is not part of a widget tree.

#include <trx/game/objects/types.h>

typedef enum {
    OVERLAY_TEXT_NONE = 0,
    OVERLAY_TEXT_LITERAL,
    OVERLAY_TEXT_GS_KEY,
    OVERLAY_TEXT_OBJECT_NAME,
} OVERLAY_TEXT_KIND;

typedef struct {
    OVERLAY_TEXT_KIND kind;
    // Optional GS key for a %s format wrapper.
    const char *fmt_gs_key;
    bool flash_enabled;
    union {
        const char *literal;
        const char *gs_key;
        OBJECT_ID object_id;
    };
} OVERLAY_TEXT;

typedef enum {
    OVERLAY_ARROW_TL, // top-left screen corner
    OVERLAY_ARROW_TR, // top-right screen corner
    OVERLAY_ARROW_BL, // bottom-left screen corner
    OVERLAY_ARROW_BR, // bottom-right screen corner
    OVERLAY_ARROW_BCL, // low text left side
    OVERLAY_ARROW_BCR, // low text right side
    OVERLAY_ARROW_NUMBER_OF,
} OVERLAY_ARROW;

void Overlay_Reset(void);
void Overlay_Control(void);
void Overlay_Animate(int32_t num_frames);

void Overlay_DrawGameInfo(void);
void Overlay_AddDisplayPickup(OBJECT_ID obj_id);

// Adds overlay text, arrows, and version text to UI regions.
void Overlay_DrawUI(void);

// Shows the health bar for the current frame.
void Overlay_ForceHealthBar(bool show);

// Reports whether something asks for the health bar, which the inventory ring
// does while it shows a medipack.
bool Overlay_IsHealthBarForced(void);

void Overlay_ShowArrow(OVERLAY_ARROW arrow, bool show);
void Overlay_ShowVersion(bool show);

void Overlay_SetTopText(OVERLAY_TEXT text);
void Overlay_SetBottomText(OVERLAY_TEXT text);
