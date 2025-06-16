#pragma once

#include <stdint.h>

extern void Stats_StartTimer(void);
extern void Stats_ObserveItemsLoad(void);

extern bool Stats_HasSecret(int16_t secret_num);
extern bool Stats_TakeSecret(int16_t secret_num);
extern bool Stats_AddSecret(int16_t secret_num);
