#pragma once

#include <trx/game/game_flow/types.h>
#include <trx/game/inventory_ring/enum.h>
#include <trx/game/objects/ids.h>

#include <stdint.h>

// Engine screens that another layer, such as the scripting layer, can take
// over. A screen offers itself when it opens. An owner that accepts the
// screen draws it and reads its input until the owner closes it with a
// choice, or until the screen ends under the owner.

typedef enum {
    UI_TAKEOVER_RING_ENTRY,
    UI_TAKEOVER_PAUSE,
    UI_TAKEOVER_SAVE_LOAD,
    UI_TAKEOVER_STATS,
    UI_TAKEOVER_EXAMINE,
    UI_TAKEOVER_COMBINE,
    UI_TAKEOVER_GLOBE_SELECT,
    UI_TAKEOVER_LEVEL_VIEW,
    UI_TAKEOVER_PICTURE,
    UI_TAKEOVER_MENU,
    UI_TAKEOVER_NUMBER_OF,
} UI_TAKEOVER;

// What a screen reports to its owner as it is offered. Each screen fills in
// the fields that it reports and leaves the others zero.
typedef struct {
    // The entry that the player uses, examines or combines in the inventory
    // ring, for UI_TAKEOVER_RING_ENTRY, UI_TAKEOVER_EXAMINE and
    // UI_TAKEOVER_COMBINE, and the entry the menu opens on, or NO_OBJECT for
    // none, for UI_TAKEOVER_MENU.
    OBJECT_ID object_id;
    // What the menu opened for, for UI_TAKEOVER_SAVE_LOAD and UI_TAKEOVER_MENU.
    INVENTORY_MODE mode;
    // The level whose statistics the screen shows, whether it shows the totals
    // of the whole game, and whether it is drawn bare, for UI_TAKEOVER_STATS.
    const GF_LEVEL *level;
    bool is_final;
    bool is_bare;
    // The picture the screen stands on, or nullptr for none, for
    // UI_TAKEOVER_STATS, UI_TAKEOVER_GLOBE_SELECT, UI_TAKEOVER_PICTURE and
    // UI_TAKEOVER_MENU. The screen owns the path, and keeps it while the owner
    // holds the screen.
    const char *background;
    // Whether the title level runs behind the menu, for UI_TAKEOVER_MENU.
    bool is_live;
    // How long the screen shows and fades, in ticks, for
    // UI_TAKEOVER_LEVEL_VIEW and UI_TAKEOVER_PICTURE.
    int32_t display_ticks;
    int32_t fade_in_ticks;
    int32_t fade_out_ticks;
    // Whether the picture shows while a level loads, and whether it may fade
    // in from the frame shown before it, for UI_TAKEOVER_PICTURE.
    bool is_loading;
    bool can_cross_fade_in;
} UI_TAKEOVER_ARGS;

typedef enum {
    UI_TAKEOVER_CHOICE_NONE,
    UI_TAKEOVER_CHOICE_CANCEL,
    UI_TAKEOVER_CHOICE_CONFIRM,
    UI_TAKEOVER_CHOICE_RESUME,
    UI_TAKEOVER_CHOICE_EXIT_TO_TITLE,
    UI_TAKEOVER_CHOICE_NUMBER_OF,
} UI_TAKEOVER_CHOICE;

typedef struct {
    // Returns whether the owner takes the screen.
    bool (*offer)(UI_TAKEOVER screen, const UI_TAKEOVER_ARGS *args);
    // Tells the owner that the screen ended while the owner still held it.
    void (*release)(UI_TAKEOVER screen);
} UI_TAKEOVER_HOOKS;

void UI_Takeover_SetHooks(UI_TAKEOVER_HOOKS hooks);

// Offers the screen to the owner. Returns whether the owner took it; the
// screen then draws nothing and reads no input of its own.
bool UI_Takeover_Offer(UI_TAKEOVER screen, UI_TAKEOVER_ARGS args);

// Returns whether an owner holds the screen.
bool UI_Takeover_IsHeld(UI_TAKEOVER screen);

// Returns whether an owner holds any screen.
bool UI_Takeover_IsAnyHeld(void);

// Returns whether an owner closed any screen with a choice that has not been
// taken yet.
bool UI_Takeover_IsAnyClosed(void);

// Returns the choice the owner closed the screen with, or
// UI_TAKEOVER_CHOICE_NONE while the screen is still open. A returned choice
// ends the owner's hold on the screen.
UI_TAKEOVER_CHOICE UI_Takeover_TakeChoice(UI_TAKEOVER screen);

// Ends the owner's hold on a screen that is going away, and tells the owner
// if it had not closed the screen itself. Does nothing while no owner holds
// the screen.
void UI_Takeover_Release(UI_TAKEOVER screen);

// Returns whether the screen takes the choice. A ring entry takes
// UI_TAKEOVER_CHOICE_CANCEL, which puts the entry away, and
// UI_TAKEOVER_CHOICE_CONFIRM, which leaves the ring as a used entry does. The
// pause screen takes UI_TAKEOVER_CHOICE_CANCEL and UI_TAKEOVER_CHOICE_RESUME,
// which both return to the game, and UI_TAKEOVER_CHOICE_EXIT_TO_TITLE. The
// quick save and load screens take
// only UI_TAKEOVER_CHOICE_CANCEL. The statistics screen shown between levels
// takes UI_TAKEOVER_CHOICE_CANCEL and UI_TAKEOVER_CHOICE_CONFIRM, which both
// end it as the player skipping it does. An entry that the player examines
// takes only UI_TAKEOVER_CHOICE_CANCEL, which puts it away. An entry that the
// player combines takes UI_TAKEOVER_CHOICE_CANCEL and
// UI_TAKEOVER_CHOICE_CONFIRM, which both put it away; the owner combines it
// with what it picks before it closes the screen. The globe that picks the
// next adventure takes only UI_TAKEOVER_CHOICE_CANCEL, which leaves for the
// title screen; the owner picks an adventure by starting its level.
bool UI_Takeover_AcceptsChoice(UI_TAKEOVER screen, UI_TAKEOVER_CHOICE choice);

// Closes the screen with a choice. Called by the owner. Does nothing while no
// owner holds the screen.
void UI_Takeover_Close(UI_TAKEOVER screen, UI_TAKEOVER_CHOICE choice);
