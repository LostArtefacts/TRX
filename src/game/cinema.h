#ifndef T1M_GAME_CINEMA_H
#define T1M_GAME_CINEMA_H

void CalculateCinematicCamera();
void ControlCinematicPlayer(int16_t item_num);
void ControlCinematicPlayer4(int16_t item_num);
void InitialisePlayer1(int16_t item_num);
void InitialiseGenPlayer(int16_t item_num);
void InGameCinematicCamera();

void T1MInjectGameCinema();

#endif
