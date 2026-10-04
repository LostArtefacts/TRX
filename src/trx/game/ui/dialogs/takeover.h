#pragma once

#include <stdint.h>

// Engine screens that another layer, such as the scripting layer, can take
// over. A screen offers itself when it opens. An owner that accepts the
// screen draws it and reads its input until the owner closes it with a
// choice, or until the screen ends under the owner.

typedef enum {
    UI_TAKEOVER_RING_ENTRY,
    UI_TAKEOVER_PAUSE,
    UI_TAKEOVER_NUMBER_OF,
} UI_TAKEOVER;

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
    bool (*offer)(UI_TAKEOVER screen, int32_t arg);
    // Tells the owner that the screen ended while the owner still held it.
    void (*release)(UI_TAKEOVER screen);
} UI_TAKEOVER_HOOKS;

void UI_Takeover_SetHooks(UI_TAKEOVER_HOOKS hooks);

// Offers the screen to the owner. Returns whether the owner took it; the
// screen then draws nothing and reads no input of its own.
bool UI_Takeover_Offer(UI_TAKEOVER screen, int32_t arg);

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
// pause question takes UI_TAKEOVER_CHOICE_CANCEL, UI_TAKEOVER_CHOICE_RESUME
// and UI_TAKEOVER_CHOICE_EXIT_TO_TITLE.
bool UI_Takeover_AcceptsChoice(UI_TAKEOVER screen, UI_TAKEOVER_CHOICE choice);

// Closes the screen with a choice. Called by the owner. Does nothing while no
// owner holds the screen.
void UI_Takeover_Close(UI_TAKEOVER screen, UI_TAKEOVER_CHOICE choice);
