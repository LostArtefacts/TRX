#pragma once

#include <stdint.h>

void Room_DrawReset(void);
void Room_MarkToBeDrawn(int16_t room_num);
int32_t Room_DrawGetCount(void);
int16_t Room_DrawGetRoom(int16_t idx);

void Room_DrawAllRooms(int16_t base_room, int16_t target_room);

// Whether the last drawn frame had any outside-flagged room in view (i.e.
// the sky was visible), matching the OG engines' "outside" draw flag.
bool Room_IsSkyVisible(void);

// Manage per-room draw queue of items
void Room_AddDrawnItem(int16_t room_num, int16_t item_num);
void Room_RemoveDrawnItem(int16_t room_num, int16_t item_num);
