#ifndef T1M_SPECIFIC_INIT_H
#define T1M_SPECIFIC_INIT_H

#include "global/types.h"

#include <stdarg.h>
#include <stdint.h>

void S_InitialiseSystem();
void S_ExitSystem(const char *message);
void S_ExitSystemFmt(const char *fmt, ...);

void CalculateWibbleTable();
void S_SeedRandom();

#endif
