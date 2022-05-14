#include "game/room_draw.h"

#include "game/draw.h"
#include "game/shell.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "log.h"
#include "math/matrix.h"

static int32_t m_RoomNumStack[MAX_ROOMS_TO_DRAW] = { 0 };
static int32_t m_RoomNumStackIdx = 0;

static void Room_PrintDrawStack(void);

static void Room_PrintDrawStack(void)
{
    for (int i = 0; i < m_RoomNumStackIdx; i++) {
        LOG_ERROR("Room Number %d", m_RoomNumStack[i]);
    }
}

void Room_GetBounds(int16_t room_num)
{
    ROOM_INFO *r = &g_RoomInfo[room_num];
    if (!Matrix_Push()) {
        Room_PrintDrawStack();
        Shell_ExitSystem("Matrix stack overflow.");
    }
    m_RoomNumStack[m_RoomNumStackIdx++] = room_num;
    Matrix_TranslateAbs(r->x, r->y, r->z);
    if (r->doors) {
        for (int i = 0; i < r->doors->count; i++) {
            DOOR_INFO *door = &r->doors->door[i];
            if (SetRoomBounds(&door->x, door->room_num, r)) {
                Room_GetBounds(door->room_num);
            }
        }
    }
    Matrix_Pop();
    m_RoomNumStackIdx--;
}
