#pragma once

// Owns the overlay state that the engine sets and the overlay scripts draw.

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

void Overlay_Reset(void);
void Overlay_Control(void);

// Returns the line at the top of the screen in the player's language, or
// nullptr for none. A script draws it.
const char *Overlay_GetTopText(void);

bool Overlay_IsVersionShown(void);

// Shows the health bar for the current frame.
void Overlay_ForceHealthBar(bool show);

// Reports whether something asks for the health bar, which the inventory ring
// does while it shows a medipack.
bool Overlay_IsHealthBarForced(void);

void Overlay_ShowVersion(bool show);

void Overlay_SetTopText(OVERLAY_TEXT text);
