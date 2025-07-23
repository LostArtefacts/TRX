#include "game/effects.h"
#include "game/lara/common.h"
#include "game/output.h"
#include "game/random.h"
#include "game/sound.h"

#define M_RANGE (WALL_L * 10) // = 10240

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = true;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (TR_VERSION == 1 && (item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (!Item_IsNearby(item, lara_item, M_RANGE)) {
        return;
    }

    Output_CalculateLight(item->pos, item->room_num);
    if (TR_VERSION == 2) {
        Sound_Effect(SFX_WATERFALL_LOOP, &item->pos, SPM_NORMAL);
    }

    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_SPLASH_1;
        effect->pos.x =
            item->pos.x + ((Random_GetDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
        effect->pos.y = item->pos.y;
        effect->pos.z =
            item->pos.z + ((Random_GetDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
        effect->speed = 0;
        effect->frame_num = 0;
        effect->shade = Output_GetLightAdder();
    }
}

REGISTER_OBJECT(O_WATERFALL, M_Setup)
