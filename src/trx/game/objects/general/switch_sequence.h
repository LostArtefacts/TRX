// The order in which Lara presses the three TR4 sequence switches, and which
// orders have already opened their sequence door. A level holds one such
// puzzle; the state is reset as each level loads.
#pragma once

#include <stdint.h>

#define SWITCH_SEQUENCE_LENGTH 3
#define SWITCH_SEQUENCE_RESULT_COUNT 6
#define SWITCH_SEQUENCE_NO_RESULT (-1)

typedef struct {
    // The number of switches pressed so far, from 0 to 3. Steps 4 to 6 put
    // the switches back up, one step for each switch, and then wrap to 0.
    int32_t step;
    // The OCB of each switch, in the order Lara pressed them.
    int32_t entries[SWITCH_SEQUENCE_LENGTH];
    // One bit for each result whose door has already opened.
    uint32_t used_mask;
} SWITCH_SEQUENCE_STATE;

void SwitchSequence_Reset(void);

// The whole state, for savegames.
const SWITCH_SEQUENCE_STATE *SwitchSequence_GetState(void);
void SwitchSequence_SetState(const SWITCH_SEQUENCE_STATE *state);

// Whether Lara can press another switch.
bool SwitchSequence_IsOpen(void);
// Whether all three switches are pressed.
bool SwitchSequence_IsComplete(void);
// Whether the switches are going back up.
bool SwitchSequence_IsResetting(void);
// The number of switches pressed so far.
int32_t SwitchSequence_GetEnteredCount(void);

// Records the press of the switch with this OCB. A completed order whose door
// has already opened puts the switches back up at once.
void SwitchSequence_Enter(int32_t switch_ocb);
// Replaces the order Lara pressed with this one.
void SwitchSequence_SetEntries(const int32_t *entries);

// Returns the result of the entered order, which is the OCB of the door it
// opens, or SWITCH_SEQUENCE_NO_RESULT for a switch OCB outside 0 to 2.
int32_t SwitchSequence_GetResult(void);
bool SwitchSequence_IsUsed(int32_t result);
void SwitchSequence_SetUsed(int32_t result);

// Starts to put the switches back up.
void SwitchSequence_BeginReset(void);
// Puts one switch back up.
void SwitchSequence_StepReset(void);
