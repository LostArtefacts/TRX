#include "game/carrier.h"
#include "game/creature.h"
#include "game/effects.h"
#include "game/lara.h"
#include "game/lara/const.h"
#include "game/los.h"
#include "game/objects/effects/missile_common.h"
#include "game/pathing.h"
#include "game/rooms.h"
#include "game/spawn.h"

// clang-format off
#define DIVER_SWIM_TURN     (3 * DEG_1) // = 546
#define DIVER_FRONT_ARC     DEG_45
#define DIVER_DIE_ANIM      16
#define DIVER_HITPOINTS     20
#define DIVER_RADIUS        (WALL_L / 3) // = 341
// clang-format on

typedef enum {
    // clang-format off
    DIVER_STATE_EMPTY   = 0,
    DIVER_STATE_SWIM_1  = 1,
    DIVER_STATE_SWIM_2  = 2,
    DIVER_STATE_SHOOT_1 = 3,
    DIVER_STATE_AIM_1   = 4,
    DIVER_STATE_NULL_1  = 5,
    DIVER_STATE_AIM_2   = 6,
    DIVER_STATE_SHOOT_2 = 7,
    DIVER_STATE_NULL_2  = 8,
    DIVER_STATE_DEATH   = 9,
    // clang-format on
} DIVER_STATE;

static const BITE m_DiverBite = {
    .pos = { .x = 17, .y = 164, .z = 44 },
    .mesh_num = 18,
};

static int32_t M_GetWaterSurface(
    const int32_t x, const int32_t y, const int32_t z, const int16_t room_num)
{
    const ROOM *room = Room_Get(room_num);
    const SECTOR *sector = Room_GetWorldSector(room, x, z);

    if ((room->flags & RF_UNDERWATER)) {
        while (sector->portal_room.sky != NO_ROOM) {
            room = Room_Get(sector->portal_room.sky);
            if (!(room->flags & RF_UNDERWATER)) {
                return sector->ceiling.height;
            }
            sector = Room_GetWorldSector(room, x, z);
        }
    } else {
        while (sector->portal_room.pit != NO_ROOM) {
            room = Room_Get(sector->portal_room.pit);
            if ((room->flags & RF_UNDERWATER)) {
                return sector->floor.height;
            }
            sector = Room_GetWorldSector(room, x, z);
        }
    }
    return NO_HEIGHT;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != DIVER_STATE_DEATH) {
            Item_SwitchToAnim(item, DIVER_DIE_ANIM, 0);
            item->current_anim_state = DIVER_STATE_DEATH;
            Carrier_TestItemDrops(item_num);
        }
        Creature_Float(item_num);
        return;
    }

    AI_INFO info;
    Creature_AIInfo(item, &info);
    Creature_Mood(item, &info, MOOD_BORED);

    bool shoot;
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    if (lara->water_status == LWS_ABOVE_WATER) {
        const GAME_VECTOR start = {
            .x = item->pos.x,
            .y = item->pos.y - STEP_L,
            .z = item->pos.z,
            .room_num = item->room_num,
        };
        GAME_VECTOR target = {
            .x = lara_item->pos.x,
            .y = lara_item->pos.y - (LARA_HEIGHT - 150),
            .z = lara_item->pos.z,
            .room_num = lara_item->room_num,
        };
        shoot = LOS_Check(&start, &target);

        if (shoot) {
            creature->target.x = lara_item->pos.x;
            creature->target.y = lara_item->pos.y;
            creature->target.z = lara_item->pos.z;
        }

        if (info.angle < -DIVER_FRONT_ARC || info.angle > DIVER_FRONT_ARC) {
            shoot = false;
        }
    } else if (info.angle > -DIVER_FRONT_ARC && info.angle < DIVER_FRONT_ARC) {
        const GAME_VECTOR start = {
            .x = item->pos.x,
            .y = item->pos.y,
            .z = item->pos.z,
            .room_num = item->room_num,
        };
        GAME_VECTOR target = {
            .x = lara_item->pos.x,
            .y = lara_item->pos.y,
            .z = lara_item->pos.z,
            .room_num = lara_item->room_num,
        };
        shoot = LOS_Check(&start, &target);
    } else {
        shoot = false;
    }

    int16_t head = 0;
    int16_t neck = 0;
    int16_t angle = Creature_Turn(item, creature->maximum_turn);
    int32_t water_level =
        M_GetWaterSurface(item->pos.x, item->pos.y, item->pos.z, item->room_num)
        + 512;

    switch (item->current_anim_state) {
    case DIVER_STATE_SWIM_1:
        creature->maximum_turn = DIVER_SWIM_TURN;
        if (shoot) {
            neck = -info.angle;
        }
        if (creature->target.y < water_level
            && item->pos.y < water_level + creature->lot.setup.fly) {
            item->goal_anim_state = DIVER_STATE_SWIM_2;
        } else if (creature->mood != MOOD_ESCAPE && shoot) {
            item->goal_anim_state = DIVER_STATE_AIM_1;
        }
        break;

    case DIVER_STATE_SWIM_2:
        creature->maximum_turn = DIVER_SWIM_TURN;
        if (shoot) {
            head = info.angle;
        }
        if (creature->target.y > water_level) {
            item->goal_anim_state = DIVER_STATE_SWIM_1;
        } else if (creature->mood != MOOD_ESCAPE && shoot) {
            item->goal_anim_state = DIVER_STATE_AIM_2;
        }
        break;

    case DIVER_STATE_SHOOT_1:
        if (shoot) {
            neck = -info.angle;
        }
        if (!creature->flags) {
            Creature_Effect(item, &m_DiverBite, Spawn_Harpoon);
            creature->flags = 1;
        }
        break;

    case DIVER_STATE_SHOOT_2:
        if (shoot) {
            head = info.angle;
        }
        if (!creature->flags) {
            Creature_Effect(item, &m_DiverBite, Spawn_Harpoon);
            creature->flags = 1;
        }
        break;

    case DIVER_STATE_AIM_1:
        creature->flags = 0;
        if (shoot) {
            neck = -info.angle;
        }
        if (!shoot || creature->mood == MOOD_ESCAPE
            || (creature->target.y < water_level
                && item->pos.y < water_level + creature->lot.setup.fly)) {
            item->goal_anim_state = DIVER_STATE_SWIM_1;
        } else {
            item->goal_anim_state = DIVER_STATE_SHOOT_1;
        }
        break;

    case DIVER_STATE_AIM_2:
        creature->flags = 0;
        if (shoot) {
            head = info.angle;
        }
        if (!shoot || creature->mood == MOOD_ESCAPE
            || creature->target.y > water_level) {
            item->goal_anim_state = DIVER_STATE_SWIM_2;
        } else {
            item->goal_anim_state = DIVER_STATE_SHOOT_2;
        }
        break;

    default:
        break;
    }

    Creature_Head(item, head);
    Creature_Neck(item, neck);

    Creature_Animate(item_num, angle, 0);

    switch (item->current_anim_state) {
    case DIVER_STATE_SWIM_1:
    case DIVER_STATE_AIM_1:
    case DIVER_STATE_SHOOT_1:
        Creature_Underwater(item, WALL_L / 2);
        break;

    default:
        item->pos.y = water_level - WALL_L / 2;
        break;
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = DIVER_HITPOINTS;
    obj->radius = DIVER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 50;
    obj->lot_setup = g_LOT_Flyer;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 10)->rot.y = true;
    Object_GetBone(obj, 14)->rot.z = true;
}

REGISTER_OBJECT(O_DIVER, M_Setup)
