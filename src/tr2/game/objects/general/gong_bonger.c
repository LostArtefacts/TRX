#include "game/camera.h"
#include "game/items.h"
#include "game/music.h"
#include "game/room.h"
#include "global/vars.h"

#define GONG_BONGER_STRIKE_FRAME 41
#define GONG_BONGER_END_FRAME 79

static void M_ActivateHeavyTriggers(int16_t item_num);
static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_ActivateHeavyTriggers(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    Room_TestTriggers(item);
    Item_Kill(item_num);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    Item_Animate(item);
    if (Item_TestFrameEqual(item, GONG_BONGER_STRIKE_FRAME)) {
        Music_Play(MX_REVEAL_1, MPM_ALWAYS);
        g_Camera.bounce -= 50;
    }

    if (Item_TestFrameEqual(item, GONG_BONGER_END_FRAME)) {
        M_ActivateHeavyTriggers(item_num);
    }
}

REGISTER_OBJECT(O_GONG_BONGER, M_Setup)
