#include "game/objects/general/gong_bonger.h"

#include "game/camera.h"
#include "game/items.h"
#include "game/music.h"
#include "game/room.h"
#include "global/vars.h"

#define GONG_BONGER_STRIKE_FRAME 41
#define GONG_BONGER_END_FRAME 79

static void M_ActivateHeavyTriggers(int16_t item_num);

static void M_ActivateHeavyTriggers(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    Room_TestTriggers(item);
    Item_Kill(item_num);
}

void GongBonger_Setup(void)
{
    OBJECT *const obj = Object_Get(O_GONG_BONGER);
    obj->control_func = GongBonger_Control;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void GongBonger_Control(const int16_t item_num)
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
