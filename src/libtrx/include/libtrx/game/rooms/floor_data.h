#pragma once

#include "./types.h"

void Room_ParseFloorData(const int16_t *floor_data);
void Room_PopulateSectorData(
    SECTOR *sector, const int16_t *floor_data, uint16_t start_index,
    uint16_t null_index);

void Room_TestTriggers(const ITEM *item);
extern void Room_TestSectorTrigger(const ITEM *item, const SECTOR *sector);
