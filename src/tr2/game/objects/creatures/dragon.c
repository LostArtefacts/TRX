#include "game/objects/creatures/dragon.h"

#include "game/camera.h"
#include "game/collide.h"
#include "game/creature.h"
#include "game/input.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/lot.h"
#include "game/objects/common.h"
#include "game/output.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/math.h>

// clang-format off
#define DRAGON_CLOSE        900
#define DRAGON_FAR          2300
#define DRAGON_MID          ((DRAGON_CLOSE + DRAGON_FAR) / 2) // = 1600
#define DRAGON_L_COL        -512
#define DRAGON_R_COL        +512
#define DRAGON_ALMOST_LIVE  100
#define DRAGON_CLOSE_RANGE  9437184
#define DRAGON_STOP_RANGE   37748736
#define DRAGON_TOUCH_DAMAGE 10
#define DRAGON_SWIPE_DAMAGE 250
#define DRAGON_WALK_TURN    (DEG_1 * 2) // = 364
#define DRAGON_NEED_TURN    (DEG_1) // = 182
#define DRAGON_TOUCH_L      0x7F000000
#define DRAGON_TOUCH_R      0x000000FE
#define DRAGON_LIVE_TIME    330
#define DRAGON_HITPOINTS    300
#define DRAGON_RADIUS       (WALL_L / 3) // = 341
// clang-format on

typedef enum {
    // clang-format off
    DRAGON_STATE_EMPTY       = 0,
    DRAGON_STATE_WALK        = 1,
    DRAGON_STATE_LEFT        = 2,
    DRAGON_STATE_RIGHT       = 3,
    DRAGON_STATE_AIM         = 4,
    DRAGON_STATE_FIRE        = 5,
    DRAGON_STATE_STOP        = 6,
    DRAGON_STATE_TURN_LEFT   = 7,
    DRAGON_STATE_TURN_RIGHT  = 8,
    DRAGON_STATE_SWIPE_LEFT  = 9,
    DRAGON_STATE_SWIPE_RIGHT = 10,
    DRAGON_STATE_DEATH       = 11,
    // clang-format on
} DRAGON_STATE;

typedef enum {
    // clang-format off
    DRAGON_ANIM_DIE       = 21,
    DRAGON_ANIM_DEAD      = 22,
    DRAGON_ANIM_RESURRECT = 23,
    // clang-format on
} DRAGON_ANIM;

static const BITE m_DragonMouth = {
    .pos = { .x = 35, .y = 171, .z = 1168 },
    .mesh_num = 12,
};

static void M_MarkDragonDead(const ITEM *dragon_back_item);
static void M_PushLaraAway(ITEM *lara_item, ITEM *dragon_item, int32_t shift);
static void M_PullDagger(ITEM *lara_item, ITEM *dragon_back_item);
static void M_SetupFront(OBJECT *obj);
static void M_SetupBack(OBJECT *obj);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
static void M_Control(int16_t item_num);

static void M_MarkDragonDead(const ITEM *const dragon_back_item)
{
    const int16_t dragon_front_item_num = (intptr_t)dragon_back_item->data;
    const ITEM *const dragon_front_item = Item_Get(dragon_front_item_num);
    CREATURE *const creature = dragon_front_item->data;
    creature->flags = -1;
    g_SaveGame.current_stats.kills++;
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
    Item_SwitchToObjAnim(lara_item, LA_EXTRA_BREATH, 0, O_LARA_EXTRA);
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

    Camera_InvokeCinematic(lara_item, 0, 0);

    M_MarkDragonDead(dragon_back_item);
}

static void M_SetupFront(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    ASSERT(Object_Get(O_DRAGON_BACK)->loaded);
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;

    obj->hit_points = DRAGON_HITPOINTS;
    obj->radius = DRAGON_RADIUS;
    obj->pivot_length = 300;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 10)->rot_z = true;
}

static void M_SetupBack(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->collision_func = M_Collision;

    obj->radius = DRAGON_RADIUS;

    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (item->current_anim_state != DRAGON_STATE_DEATH) {
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
    if (g_Input.action && item->object_id == O_DRAGON_BACK
        && (Item_TestAnimEqual(item, DRAGON_ANIM_DEAD)
            || (Item_TestAnimEqual(item, DRAGON_ANIM_RESURRECT)
                && Item_TestFrameRange(item, 0, DRAGON_ALMOST_LIVE)))
        && !lara_item->gravity && shift <= DRAGON_MID
        && shift > DRAGON_CLOSE - 350 && side_shift > -350 && side_shift < 350
        && angle > DEG_90 - 30 * DEG_1 && angle < DEG_90 + 30 * DEG_1) {
        M_PullDagger(lara_item, item);
    } else {
        M_PushLaraAway(lara_item, item, shift);
    }
}

void Dragon_Bones(const int16_t item_num)
{
    const int16_t bone_front_item_num = Item_Create();
    const int16_t bone_back_item_num = Item_Create();

    if (bone_back_item_num == NO_ITEM || bone_front_item_num == NO_ITEM) {
        return;
    }

    const ITEM *const dragon_item = Item_Get(item_num);

    ITEM *const bone_back = Item_Get(bone_back_item_num);
    bone_back->object_id = O_DRAGON_BONES_3;
    bone_back->pos.x = dragon_item->pos.x;
    bone_back->pos.y = dragon_item->pos.y;
    bone_back->pos.z = dragon_item->pos.z;
    bone_back->rot.x = 0;
    bone_back->rot.y = dragon_item->rot.y;
    bone_back->rot.z = 0;
    bone_back->room_num = dragon_item->room_num;
    bone_back->shade.value_1 = -1;
    Item_Initialise(bone_back_item_num);

    ITEM *const bone_front = Item_Get(bone_front_item_num);
    bone_front->object_id = O_DRAGON_BONES_2;
    bone_front->pos.x = dragon_item->pos.x;
    bone_front->pos.y = dragon_item->pos.y;
    bone_front->pos.z = dragon_item->pos.z;
    bone_front->rot.x = 0;
    bone_front->rot.y = dragon_item->rot.y;
    bone_front->rot.z = 0;
    bone_front->room_num = dragon_item->room_num;
    bone_front->shade.value_1 = -1;
    Item_Initialise(bone_front_item_num);
    bone_front->mesh_bits = ~0xC00000u;
}

static void M_Control(const int16_t item_num)
{
    const int16_t dragon_back_item_num = item_num;
    ITEM *const dragon_back_item = Item_Get(item_num);
    if (dragon_back_item->object_id == O_DRAGON_FRONT) {
        return;
    }

    const int16_t dragon_front_item_num = (intptr_t)dragon_back_item->data;
    ITEM *const dragon_front_item = Item_Get(dragon_front_item_num);
    if (!Creature_Activate(dragon_front_item_num)) {
        return;
    }

    int16_t angle = 0;
    int16_t head = 0;
    CREATURE *const creature = dragon_front_item->data;
    const OBJECT *const front_obj = Object_Get(O_DRAGON_FRONT);

    if (dragon_front_item->hit_points <= 0) {
        if (dragon_front_item->current_anim_state != DRAGON_STATE_DEATH) {
            Item_SwitchToAnim(dragon_front_item, DRAGON_ANIM_DIE, 0);
            dragon_front_item->goal_anim_state = DRAGON_STATE_DEATH;
            dragon_front_item->current_anim_state = DRAGON_STATE_DEATH;
            creature->flags = 0;
        } else if (creature->flags >= 0) {
            Spawn_MysticLight(dragon_front_item_num);
            creature->flags++;
            if (creature->flags == DRAGON_LIVE_TIME) {
                dragon_front_item->goal_anim_state = DRAGON_STATE_STOP;
            }
            if (creature->flags > DRAGON_LIVE_TIME + DRAGON_ALMOST_LIVE) {
                dragon_front_item->hit_points = front_obj->hit_points / 2;
            }
        } else {
            if (creature->flags > -20) {
                // clang-format off
                Output_AddDynamicLight(
                    dragon_front_item->pos,
                    ((4 * Random_GetDraw()) >> 15) + 12 + creature->flags / 2,
                    ((4 * Random_GetDraw()) >> 15) + 10 + creature->flags / 2);
                // clang-format on
            }

            if (creature->flags == -100) {
                Dragon_Bones(dragon_front_item_num);
            } else if (creature->flags == -200) {
                LOT_DisableBaddieAI(dragon_front_item_num);
                Item_Kill(dragon_front_item_num);
                dragon_front_item->status = IS_DEACTIVATED;
                Item_Kill(dragon_back_item_num);
                dragon_back_item->status = IS_DEACTIVATED;
            } else if (creature->flags < -100) {
                dragon_front_item->pos.y += 10;
                dragon_back_item->pos.y += 10;
            }

            creature->flags--;
            return;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(dragon_front_item, &info);
        Creature_Mood(dragon_front_item, &info, MOOD_ATTACK);

        angle = Creature_Turn(dragon_front_item, DRAGON_WALK_TURN);
        const bool is_ahead = info.ahead && info.distance > DRAGON_CLOSE_RANGE
            && info.distance < DRAGON_STOP_RANGE;
        if (dragon_front_item->touch_bits) {
            g_LaraItem->hit_status = 1;
            g_LaraItem->hit_points -= DRAGON_TOUCH_DAMAGE;
        }

        switch (dragon_front_item->current_anim_state) {
        case DRAGON_STATE_WALK:
            creature->flags = 0;
            if (is_ahead) {
                dragon_front_item->goal_anim_state = DRAGON_STATE_STOP;
            } else if (angle < -DRAGON_NEED_TURN) {
                if (info.distance < DRAGON_STOP_RANGE && info.ahead) {
                    dragon_front_item->goal_anim_state = DRAGON_STATE_STOP;
                } else {
                    dragon_front_item->goal_anim_state = DRAGON_STATE_LEFT;
                }
            } else if (angle > DRAGON_NEED_TURN) {
                if (info.distance < DRAGON_STOP_RANGE && info.ahead) {
                    dragon_front_item->goal_anim_state = DRAGON_STATE_STOP;
                } else {
                    dragon_front_item->goal_anim_state = DRAGON_STATE_RIGHT;
                }
            }
            break;

        case DRAGON_STATE_LEFT:
            if (angle > -DRAGON_NEED_TURN || is_ahead) {
                dragon_front_item->goal_anim_state = DRAGON_STATE_WALK;
            }
            break;

        case DRAGON_STATE_RIGHT:
            if (angle < DRAGON_NEED_TURN || is_ahead) {
                dragon_front_item->goal_anim_state = DRAGON_STATE_WALK;
            }
            break;

        case DRAGON_STATE_AIM:
            dragon_front_item->rot.y -= angle;
            if (info.ahead) {
                head = -info.angle;
            }

            if (is_ahead) {
                creature->flags = 30;
                dragon_front_item->goal_anim_state = DRAGON_STATE_FIRE;
            } else {
                creature->flags = 0;
                dragon_front_item->goal_anim_state = DRAGON_STATE_AIM;
            }
            break;

        case DRAGON_STATE_FIRE:
            dragon_front_item->rot.y -= angle;
            if (info.ahead) {
                head = -info.angle;
            }

            Sound_Effect(SFX_DRAGON_FIRE, &dragon_front_item->pos, SPM_NORMAL);

            if (creature->flags != 0) {
                if (info.ahead) {
                    Creature_Effect(
                        dragon_front_item, &m_DragonMouth, Spawn_FireStream);
                }
                creature->flags--;
            } else {
                dragon_front_item->goal_anim_state = DRAGON_STATE_STOP;
            }
            break;

        case DRAGON_STATE_STOP:
            dragon_front_item->rot.y -= angle;
            if (is_ahead) {
                dragon_front_item->goal_anim_state = DRAGON_STATE_AIM;
            } else if (info.distance > DRAGON_STOP_RANGE || !info.ahead) {
                dragon_front_item->goal_anim_state = DRAGON_STATE_WALK;
            } else if (
                info.distance >= DRAGON_CLOSE_RANGE || creature->flags != 0) {
                if (info.angle < 0) {
                    dragon_front_item->goal_anim_state = DRAGON_STATE_TURN_LEFT;
                } else {
                    dragon_front_item->goal_anim_state =
                        DRAGON_STATE_TURN_RIGHT;
                }
            } else {
                creature->flags = 1;
                if (info.angle < 0) {
                    dragon_front_item->goal_anim_state =
                        DRAGON_STATE_SWIPE_LEFT;
                } else {
                    dragon_front_item->goal_anim_state =
                        DRAGON_STATE_SWIPE_RIGHT;
                }
            }
            break;

        case DRAGON_STATE_TURN_LEFT:
            creature->flags = 0;
            dragon_front_item->rot.y += -DEG_1 - angle;
            break;

        case DRAGON_STATE_TURN_RIGHT:
            creature->flags = 0;
            dragon_front_item->rot.y += DEG_1 - angle;
            break;

        case DRAGON_STATE_SWIPE_LEFT:
            if ((dragon_front_item->touch_bits & DRAGON_TOUCH_L) != 0) {
                g_LaraItem->hit_status = 1;
                g_LaraItem->hit_points -= DRAGON_SWIPE_DAMAGE;
                creature->flags = 0;
            }
            break;

        case DRAGON_STATE_SWIPE_RIGHT:
            if ((dragon_front_item->touch_bits & DRAGON_TOUCH_R) != 0) {
                g_LaraItem->hit_status = 1;
                g_LaraItem->hit_points -= DRAGON_SWIPE_DAMAGE;
                creature->flags = 0;
            }
            break;

        default:
            break;
        }
    }

    Creature_Head(dragon_front_item, head);
    Creature_Animate(dragon_front_item_num, angle, 0);
    dragon_back_item->current_anim_state =
        dragon_front_item->current_anim_state;
    const int16_t anim_num = Item_GetRelativeAnim(dragon_front_item);
    const int16_t frame_num = Item_GetRelativeFrame(dragon_front_item);
    Item_SwitchToAnim(dragon_back_item, anim_num, frame_num);
    dragon_back_item->pos.x = dragon_front_item->pos.x;
    dragon_back_item->pos.y = dragon_front_item->pos.y;
    dragon_back_item->pos.z = dragon_front_item->pos.z;
    dragon_back_item->rot.x = dragon_front_item->rot.x;
    dragon_back_item->rot.y = dragon_front_item->rot.y;
    dragon_back_item->rot.z = dragon_front_item->rot.z;
    if (dragon_back_item->room_num != dragon_front_item->room_num) {
        Item_NewRoom(dragon_back_item_num, dragon_front_item->room_num);
    }
}

REGISTER_OBJECT(O_DRAGON_FRONT, M_SetupFront)
REGISTER_OBJECT(O_DRAGON_BACK, M_SetupBack)
