#ifndef T1M_SPECIFIC_S_MAIN_H
#define T1M_SPECIFIC_S_MAIN_H

#include <stdint.h>

void TerminateGame(int exit_code);
void ShowFatalError(const char *message);
void WinSpinMessageLoop();

void T1MInjectSpecificSMain();

#endif
