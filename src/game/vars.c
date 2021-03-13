#include "game/vars.h"

SAVEGAME_INFO SaveGame;
int32_t DemoLevel;

#ifdef T1M_FEAT_GAMEPLAY
int16_t StoredLaraHealth = 0;
#endif

#ifdef T1M_FEAT_UI
int16_t BarOffsetY[6];
#endif

GameFlow GF;
