#pragma once

#include <stdbool.h>
#include <stdint.h>

int32_t StopCinematic(int32_t level_num);
int32_t CinematicLoop(void);
bool DoCinematic(int32_t nframes);
void ControlCinematicPlayer(int16_t item_num);
void ControlCinematicPlayer4(int16_t item_num);
void InitialisePlayer1(int16_t item_num);
void InitialiseGenPlayer(int16_t item_num);
