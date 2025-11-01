#include <trx/game/const.h>
#include <trx/game/effects.h>
#include <trx/game/objects.h>
#include <trx/game/sound.h>
#include <trx/log.h>
#include <trx/version.h>

typedef enum {
    // clang-format off
    STATE_IDLE   = 0,
    STATE_FIRE   = 1,
    STATE_RELOAD = 2,
    // clang-format on
} M_STATE;

static void M_CreateProjectile(ITEM *const item)
{
    const OBJECT_ID projectile_obj_id =
        item->object_id == O_DART_EMITTER ? O_DART : O_DISC;
    if (!Object_Get(projectile_obj_id)->loaded) {
        LOG_ERROR(
            "Projectile object not loaded for item #%d", Item_GetIndex(item));
        return;
    }

    const int16_t projectile_item_num = Item_Create();
    if (projectile_item_num == NO_ITEM) {
        return;
    }

    ITEM *const projectile_item = Item_Get(projectile_item_num);
    projectile_item->object_id = projectile_obj_id;
    projectile_item->room_num = item->room_num;
    projectile_item->shade.value_1 = -1;
    projectile_item->rot.y = item->rot.y;
    projectile_item->pos.y = item->pos.y - 512;

    int32_t x = 0;
    int32_t z = 0;
    switch (projectile_item->rot.y) {
    case 0:
        z = -WALL_L / 2 + 100;
        break;
    case DEG_90:
        x = -WALL_L / 2 + 100;
        break;
    case -DEG_180:
        z = WALL_L / 2 - 100;
        break;
    case -DEG_90:
        x = WALL_L / 2 - 100;
        break;
    }

    projectile_item->pos.x = item->pos.x + x;
    projectile_item->pos.z = item->pos.z + z;
    Item_Initialise(projectile_item_num);
    Item_AddActive(projectile_item_num);
    projectile_item->status = IS_ACTIVE;

    if (item->object_id == O_DART_EMITTER) {
        const int16_t effect_num = Effect_Create(projectile_item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos = projectile_item->pos;
            effect->rot = projectile_item->rot;
            effect->speed = 0;
            effect->frame_num = 0;
            effect->counter = 0;
            effect->object_id = O_DART_EFFECT;
        }
        Sound_Effect(SFX_DART, &projectile_item->pos, SPM_NORMAL);
    } else {
        Sound_Effect(SFX_DISC, &projectile_item->pos, SPM_NORMAL);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == STATE_IDLE) {
            item->goal_anim_state = STATE_FIRE;
        }
    } else if (item->current_anim_state == STATE_FIRE) {
        item->goal_anim_state = STATE_IDLE;
    }

    if (item->current_anim_state == STATE_FIRE
        && Item_TestFrameEqual(item, 0)) {
        M_CreateProjectile(item);
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_DART_EMITTER, M_Setup)
REGISTER_OBJECT(O_DISC_EMITTER, M_Setup)
