#pragma once

#include "global/types.h"

#include <SDL2/SDL_gamecontroller.h>
#include <stdbool.h>
#include <stdint.h>

void S_Input_Init(void);
void S_Input_Shutdown(void);

INPUT_STATE S_Input_GetCurrentState(
    INPUT_LAYOUT layout_num, INPUT_LAYOUT cntlr_layout_num);

INPUT_SCANCODE S_Input_GetAssignedScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role);

int16_t S_Input_GetUniqueBind(INPUT_LAYOUT layout_num, INPUT_ROLE role);

int16_t S_Input_GetAssignedButtonType(INPUT_LAYOUT layout_num, INPUT_ROLE role);

int16_t S_Input_GetAssignedBind(INPUT_LAYOUT layout_num, INPUT_ROLE role);

int16_t S_Input_GetAssignedAxisDir(INPUT_LAYOUT layout_num, INPUT_ROLE role);

void S_Input_AssignScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, INPUT_SCANCODE scancode);

void S_Input_AssignButton(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, SDL_GameControllerButton button);

void S_Input_AssignAxis(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, SDL_GameControllerAxis axis,
    int16_t axis_dir);

void S_Input_ResetControllerToDefault(INPUT_LAYOUT layout_num);

bool S_Input_ReadAndAssignKey(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num, INPUT_ROLE role);

const char *S_Input_GetKeyName(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num, INPUT_ROLE role);

const char *S_Input_GetButtonNameFromString(
    INPUT_LAYOUT layout_num, const char *btn_name);

bool S_Input_CheckKeypress(const char *key_name);

bool S_Input_CheckButtonPress(const char *button_name);
