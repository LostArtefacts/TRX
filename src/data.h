#ifndef TR1MAIN_DATA_H
#define TR1MAIN_DATA_H

#include "util.h"

// data
#define cd_drive                VAR_I_(0x0045A010, char, '.')
#define DEMO                    VAR_I_(0x0045F1C0, __int32, 0)
#define dword_45A1F0            VAR_U_(0x0045A1F0, __int32)
#define newpath                 ARRAY_(0x00459F90, char, [128])
#define RoomCount               VAR_U_(0x00462BDC, __int16)
#define RoomInfo                VAR_U_(0x00462BE8, ROOM_INFO*)
#define PhdWinMaxX              VAR_I_(0x006CAD00, __int32, 0)
#define PhdWinMaxY              VAR_I_(0x006CAD10, __int32, 0)
#define Meshes                  VAR_U_(0x0045F1B8, __int16*)
#define FloorData               VAR_U_(0x0045F1BC, __int16*)
#define StringToShow            ARRAY_(0x00456AD0, char, [128])
#define MeshPtr                 VAR_U_(0x00461F34, __int16**)
#define LevelItemCount          VAR_U_(0x0045A0E0, __int32)
#define Items                   VAR_U_(0x00462CEC, ITEM_INFO*)
#define GameAllocMemPointer     VAR_U_(0x0045E32C, __int32)
#define GameAllocMemUsed        VAR_U_(0x0045E330, __int32)
#define GameAllocMemFree        VAR_U_(0x0045E334, __int32)
#define GameMemoryPointer       VAR_U_(0x0045A034, __int32)
#define GameMemorySize          VAR_U_(0x0045EEF8, __int32)
#define CurrentLevel            VAR_U_(0x00453C4C, __int32)
#define Lara                    VAR_U_(0x0045ED80, LARA_INFO)
#define LaraItem                VAR_U_(0x0045EE6C, ITEM_INFO*)
#define LevelNames              ARRAY_(0x00453648, const char*, [NUMBER_OF_LEVELS])
#define LevelTitles             ARRAY_(0x00453DF8, const char*, [NUMBER_OF_LEVELS])
#define SecretTotals            ARRAY_(0x00453CB0, __int8, [MAX_SECRETS])
#define IsResetFlag             VAR_U_(0x00459F50, int)
#define InputStatus             VAR_U_(0x0045EEF4, int)
#define HiRes                   VAR_U_(0x00459F64, int)
#define Effects                 VAR_U_(0x0045EE70, FX_INFO*)
#define NextFxFree              VAR_U_(0x0045EE74, int16_t)
#define NextFxActive            VAR_U_(0x0045EE7A, int16_t)
#define SaveGame                ARRAY_(0x0045B9C0, SAVEGAME_INFO, [2])

#endif
