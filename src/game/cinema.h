#ifndef T1M_GAME_CINEMA_H
#define T1M_GAME_CINEMA_H

// clang-format off
#define ControlCinematicPlayer          ((void          (*)(int16_t item_num))0x004114A0)
#define InitialisePlayer1               ((void          (*)(int16_t item_num))0x004114F0)
// clang-format on

void CalculateCinematicCamera();
void InitialiseGenPlayer(int16_t item_num);
void InGameCinematicCamera();

void T1MInjectGameCinema();

#endif
