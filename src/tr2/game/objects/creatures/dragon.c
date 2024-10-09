#include "game/objects/creatures/dragon.h"

#include "game/creature.h"
#include "game/input.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/math.h"
#include "global/funcs.h"
#include "global/vars.h"

#define DRAGON_CLOSE 900
#define DRAGON_FAR 2300
#define DRAGON_MID ((DRAGON_CLOSE + DRAGON_FAR) / 2) // = 1600
#define DRAGON_L_COL -512
#define DRAGON_R_COL +512
#define DRAGON_ALMOST_LIVE 100

typedef enum {
    // clang-format off
    DRAGON_ANIM_EMPTY       = 0,
    DRAGON_ANIM_WALK        = 1,
    DRAGON_ANIM_LEFT        = 2,
    DRAGON_ANIM_RIGHT       = 3,
    DRAGON_ANIM_AIM         = 4,
    DRAGON_ANIM_FIRE        = 5,
    DRAGON_ANIM_STOP        = 6,
    DRAGON_ANIM_TURN_LEFT   = 7,
    DRAGON_ANIM_TURN_RIGHT  = 8,
    DRAGON_ANIM_SWIPE_LEFT  = 9,
    DRAGON_ANIM_SWIPE_RIGHT = 10,
    DRAGON_ANIM_DEATH       = 11,
    DRAGON_ANIM_DIE         = 21,
    DRAGON_ANIM_DEAD        = 22,
    DRAGON_ANIM_RESURRECT   = 23,
    // clang-format on
} DRAGON_ANIM;

static void M_MarkDragonDead(const ITEM *dragon_back_item);
static void M_PullDagger(ITEM *lara_item, ITEM *dragon_back_item);
static void M_PushLaraAway(ITEM *lara_item, ITEM *dragon_item, int32_t shift);

static void M_MarkDragonDead(const ITEM *const dragon_back_item)
{
    const ITEM *const dragon_front_item =
        Item_Get((int16_t)(intptr_t)dragon_back_item->data);
    CREATURE *const creature = dragon_front_item->data;
    creature->flags = -1;
}

static void M_PushLaraAway(
    ITEM *const lara_item, ITEM *const dragon_item, const int32_t shift)
{
    const int32_t cy = Math_Cos(dragon_item->rot.y);
    const int32_t sy = Math_Sin(dragon_item->rot.y);
    const int32_t base = shift < DRAGON_MID ? DRAGON_CLOSE : DRAGON_FAR;
    lara_item->pos.x += (cy * (base - shift)) >> W2V_SHIFT;
    lara_item->pos.z -= (sy * (base - shift)) >> W2V_SHIFT;
}

static void M_PullDagger(ITEM *const lara_item, ITEM *const dragon_back_item)
{
    lara_item->anim_num = Object_GetObject(O_LARA_EXTRA)->anim_idx;
    lara_item->frame_num = g_Anims[lara_item->anim_num].frame_base;
    lara_item->current_anim_state = LA_EXTRA_BREATH;
    lara_item->goal_anim_state = LA_EXTRA_PULL_DAGGER;
    lara_item->pos.x = dragon_back_item->pos.x;
    lara_item->pos.y = dragon_back_item->pos.y;
    lara_item->pos.z = dragon_back_item->pos.z;
    lara_item->rot.y = dragon_back_item->rot.y;
    lara_item->rot.x = dragon_back_item->rot.x;
    lara_item->rot.z = dragon_back_item->rot.z;
    lara_item->fall_speed = 0;
    lara_item->gravity = 0;
    lara_item->speed = 0;

    if (dragon_back_item->room_num != lara_item->room_num) {
        Item_NewRoom(g_Lara.item_num, dragon_back_item->room_num);
    }

    Item_Animate(g_LaraItem);

    g_Lara.extra_anim = 1;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    g_Lara.hit_direction = -1;

    Lara_SwapSingleMesh(LM_HAND_R, O_LARA_EXTRA);

    g_Camera.type = CAM_CINEMATIC;
    g_CineFrameIdx = 0;
    g_CinePos.pos = lara_item->pos;
    g_CinePos.rot = lara_item->rot;

    M_MarkDragonDead(dragon_back_item);
}

void __cdecl Dragon_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (item->current_anim_state != DRAGON_ANIM_DEATH) {
        Lara_Push(item, lara_item, coll, true, false);
        return;
    }

    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
    const int32_t cy = Math_Cos(item->rot.y);
    const int32_t sy = Math_Sin(item->rot.y);
    const int32_t side_shift = (cy * dz + sy * dx) >> W2V_SHIFT;

    if (side_shift <= DRAGON_L_COL || side_shift >= DRAGON_R_COL) {
        Lara_Push(item, lara_item, coll, true, false);
        return;
    }

    const int32_t shift = (cy * dx - sy * dz) >> W2V_SHIFT;
    const int32_t angle = lara_item->rot.y - item->rot.y;
    const int32_t anim =
        item->anim_num - Object_GetObject(O_DRAGON_BACK)->anim_idx;
    if ((anim == DRAGON_ANIM_DEAD
         || (anim == DRAGON_ANIM_RESURRECT
             && item->frame_num - g_Anims[item->anim_num].frame_base
                 <= DRAGON_ALMOST_LIVE))
        && (g_Input & IN_ACTION) && item->object_id == O_DRAGON_BACK
        && !lara_item->gravity && shift <= DRAGON_MID
        && shift > DRAGON_CLOSE - 350 && side_shift > -350 && side_shift < 350
        && angle > PHD_90 - 30 * PHD_DEGREE
        && angle < PHD_90 + 30 * PHD_DEGREE) {
        M_PullDagger(lara_item, item);
    } else {
        M_PushLaraAway(lara_item, item, shift);
    }
}
