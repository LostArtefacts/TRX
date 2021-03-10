#include "game/vars.h"

#ifdef T1M_FEAT_GAMEPLAY
int16_t StoredLaraHealth = 0;
#endif

#ifdef T1M_FEAT_UI
int16_t BarOffsetY[6];
#endif

char **GF_LevelTitles;
char **GF_LevelNames;
char **GF_Key1Strings;
char **GF_Key2Strings;
char **GF_Key3Strings;
char **GF_Key4Strings;
char **GF_Pickup1Strings;
char **GF_Pickup2Strings;
char **GF_Puzzle1Strings;
char **GF_Puzzle2Strings;
char **GF_Puzzle3Strings;
char **GF_Puzzle4Strings;
char *GF_GameStrings[GS_NUMBER_OF];
