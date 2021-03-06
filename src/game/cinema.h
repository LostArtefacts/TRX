#ifndef T1M_GAME_CINEMA_H
#define T1M_GAME_CINEMA_H

// clang-format off
#define ControlCinematicPlayer          ((void          (*)(int16_t item_num))0x004114A0)
// clang-format on

void CalculateCinematicCamera();
void ControlCinematicPlayer4(int16_t item_num);
void InitialisePlayer1(int16_t item_num);
void InitialiseGenPlayer(int16_t item_num);
void InGameCinematicCamera();

void T1MInjectGameCinema();

#endif
