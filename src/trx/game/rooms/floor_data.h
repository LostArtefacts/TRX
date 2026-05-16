#pragma once

#include <trx/game/rooms/types.h>

void Room_ParseFloorData(const int16_t *floor_data);
void Room_PopulateSectorData(
    SECTOR *sector, const int16_t *floor_data, uint16_t start_index,
    uint16_t null_index);
void Room_ReadTriangulation(
    SURFACE *surface, int16_t func_data, int16_t tilt_data);

void Room_RegisterTriggerHandler(
    TRIGGER_OBJECT type,
    void (*handle_func)(
        const TRIGGER *trigger, const TRIGGER_CMD *cmd,
        TRIGGER_STATUS *status));
bool Room_TestTriggers(const ITEM *item);
bool Room_TestSectorTrigger(const ITEM *item, const SECTOR *sector);
bool Room_IsAntiTrigger(TRIGGER_TYPE type);

#define REGISTER_TRIGGER_HANDLER(cmd_type, handle_func)                        \
    __attribute__((constructor)) static void                                   \
    M_RegisterTriggerHandler##cmd_type(void)                                   \
    {                                                                          \
        Room_RegisterTriggerHandler(cmd_type, handle_func);                    \
    }
