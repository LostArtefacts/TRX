#include "game/effects.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/output.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/game/lara/common.h>

#define WATERFALL_RANGE (WALL_L * 10) // = 10240

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawDummyItem;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = item->pos.x - lara_item->pos.x;
    const int32_t dy = item->pos.y - lara_item->pos.y;
    const int32_t dz = item->pos.z - lara_item->pos.z;
    if (dx < -WATERFALL_RANGE || dx > WATERFALL_RANGE || dz < -WATERFALL_RANGE
        || dz > WATERFALL_RANGE || dy < -WATERFALL_RANGE
        || dy > WATERFALL_RANGE) {
        return;
    }

    Output_CalculateLight(item->pos, item->room_num);
    Sound_Effect(SFX_WATERFALL_LOOP, &item->pos, SPM_NORMAL);

    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_SPLASH;
        effect->pos.x =
            item->pos.x + ((Random_GetDraw() - 0x4000) << 10) / 0x7FFF;
        effect->pos.y = item->pos.y;
        effect->pos.z =
            item->pos.z + ((Random_GetDraw() - 0x4000) << 10) / 0x7FFF;
        effect->speed = 0;
        effect->frame_num = 0;
        effect->shade = Output_GetLightAdder();
    }
}

REGISTER_OBJECT(O_WATERFALL, M_Setup)
