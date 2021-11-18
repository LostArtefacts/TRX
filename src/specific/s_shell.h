#ifndef T1M_SPECIFIC_S_SHELL_H
#define T1M_SPECIFIC_S_SHELL_H

#include <stdint.h>

void S_Shell_SeedRandom();
void S_Shell_ExitSystem(const char *message);
void S_Shell_ExitSystemFmt(const char *fmt, ...);
void S_Shell_SpinMessageLoop();

void S_Shell_Inject();

#endif
