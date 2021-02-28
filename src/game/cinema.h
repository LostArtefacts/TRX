#ifndef T1M_GAME_CINEMA_H
#define T1M_GAME_CINEMA_H

// clang-format off
#define InGameCinematicCamera   ((void          (*)())0x004115F0)
// clang-format on

void CalculateCinematicCamera();

void T1MInjectGameCinema();

#endif
