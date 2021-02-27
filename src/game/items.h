#ifndef T1M_GAME_ITEMS_H
#define T1M_GAME_ITEMS_H

#include <stdint.h>

// clang-format off
#define AddActiveItem           ((void          (*)(int16_t item_num))0x00421FE0)
#define InitialiseItemArray     ((void          (*)(int item_count))0x00421B10)
#define InitialiseItem          ((void          (*)(int16_t item_num))0x00421CC0)
#define GlobalItemReplace       ((int32_t       (*)(int32_t in_objnum, int32_t out_objnum))0x004221D0)
#define RemoveActiveItem        ((void          (*)(int16_t item_num))0x00421EB0)
#define ItemNewRoom             ((void          (*)(int16_t item_num, int16_t room_num))0x00422060)
// clang-format on

void InitialiseFXArray();

void T1MInjectGameItems();

#endif
