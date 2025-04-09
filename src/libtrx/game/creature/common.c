#include "game/creature.h"
#include "game/lara/common.h"
#include "game/random.h"
#include "game/rooms.h"
#include "log.h"
#include "utils.h"

#define M_MAX_DISTANCE (WALL_L * 30)
#define M_ATTACK_RANGE SQUARE(WALL_L * 3) // = 0x900000 = 9437184
#define M_ESCAPE_CHANCE 2048
#define M_RECOVER_CHANCE 256
#define M_MAX_TILT (3 * DEG_1) // = 546
#define M_MAX_HEAD_CHANGE (5 * DEG_1) // = 910
#if TR_VERSION == 1
    #define M_HEAD_ARC FRONT_ARC
#elif TR_VERSION >= 2
    #define M_HEAD_ARC 0x3000 // = 12288
#endif

static ITEM *M_ChooseEnemy(const ITEM *item);
static bool M_SwitchToWater(
    int16_t item_num, const int32_t *wh, const HYBRID_INFO *info);
static bool M_SwitchToLand(
    int16_t item_num, const int32_t *wh, const HYBRID_INFO *info);
static bool M_TestSwitchOrKill(int16_t item_num, GAME_OBJECT_ID target_id);

#if TR_VERSION == 2
extern void Creature_GetBaddieTarget(int16_t item_num, bool goody);
#endif

static ITEM *M_ChooseEnemy(const ITEM *const item)
{
    CREATURE *const creature = item->data;
    switch (item->object_id) {
#if TR_VERSION == 2
    case O_BANDIT_1:
    case O_BANDIT_2:
        Creature_GetBaddieTarget(creature->item_num, false);
        break;

    case O_MONK_1:
    case O_MONK_2:
        Creature_GetBaddieTarget(creature->item_num, true);
        break;
#endif

    default:
        creature->enemy = Lara_GetItem();
        break;
    }

    if (creature->enemy != nullptr) {
        return creature->enemy;
    }
    return Lara_GetItem();
}

static bool M_SwitchToWater(
    const int16_t item_num, const int32_t *const wh,
    const HYBRID_INFO *const info)
{
    if (*wh == NO_HEIGHT) {
        return false;
    }

    ITEM *const item = Item_Get(item_num);

    if (item->hit_points <= 0) {
        // Dead land creatures should remain in their pose permanently.
        return false;
    }

    // The land creature is alive and the room has been flooded. Switch to the
    // water creature.
    if (!M_TestSwitchOrKill(item_num, info->water.id)) {
        return false;
    }

    item->object_id = info->water.id;
    Item_SwitchToAnim(item, info->water.active_anim, 0);
    item->current_anim_state = Item_GetAnim(item)->current_anim_state;
    item->goal_anim_state = item->current_anim_state;
    item->pos.y = *wh;

    return true;
}

static bool M_SwitchToLand(
    const int16_t item_num, const int32_t *const wh,
    const HYBRID_INFO *const info)
{
    if (*wh != NO_HEIGHT) {
        return false;
    }

    if (!M_TestSwitchOrKill(item_num, info->land.id)) {
        return false;
    }

    ITEM *const item = Item_Get(item_num);

    // Switch to the land creature regardless of death state.
    item->object_id = info->land.id;
    item->rot.x = 0;

    if (item->hit_points > 0) {
        Item_SwitchToAnim(item, info->land.active_anim, 0);
        item->current_anim_state = Item_GetAnim(item)->current_anim_state;
        item->goal_anim_state = item->current_anim_state;

    } else {
        Item_SwitchToAnim(item, info->land.death_anim, -1);
        item->current_anim_state = info->land.death_state;
        item->goal_anim_state = item->current_anim_state;

        int16_t room_num = item->room_num;
        const SECTOR *const sector =
            Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
        item->floor =
            Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
        item->pos.y = item->floor;

        if (item->room_num != room_num) {
            Item_NewRoom(item_num, room_num);
        }
    }

    return true;
}

static bool M_TestSwitchOrKill(
    const int16_t item_num, const GAME_OBJECT_ID target_id)
{
    if (Object_Get(target_id)->loaded) {
        return true;
    }

    LOG_WARNING(
        "Object %d is not loaded; item %d cannot be converted.", target_id,
        item_num);
    Item_Kill(item_num);
    return false;
}

void Creature_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->rot.y += (Random_GetControl() - DEG_90) >> 1;
    item->collidable = 1;
    item->data = nullptr;
}

bool Creature_Activate(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_INVISIBLE) {
        return true;
    }

    if (!LOT_EnableBaddieAI(item_num, false)) {
        return false;
    }

    item->status = IS_ACTIVE;
    return true;
}

void Creature_AIInfo(ITEM *const item, AI_INFO *const info)
{
    CREATURE *const creature = item->data;
    if (creature == nullptr) {
        return;
    }

    ITEM *const enemy = M_ChooseEnemy(item);
    const int16_t *const zone = Box_GetLotZone(&creature->lot);

    {
        const ROOM *const room = Room_Get(item->room_num);
        item->box_num =
            Room_GetWorldSector(room, item->pos.x, item->pos.z)->box;
        info->zone_num = zone[item->box_num];
    }

    {
        const ROOM *const room = Room_Get(enemy->room_num);
        enemy->box_num =
            Room_GetWorldSector(room, enemy->pos.x, enemy->pos.z)->box;
        info->enemy_zone_num = zone[enemy->box_num];
    }

    if (((Box_GetBox(enemy->box_num)->overlap_index & creature->lot.block_mask)
         != 0)
        || (creature->lot.node[item->box_num].search_num
            == (creature->lot.search_num | BOX_BLOCKED_SEARCH))) {
        info->enemy_zone_num |= BOX_BLOCKED;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    const int32_t z = enemy->pos.z
        - ((obj->pivot_length * Math_Cos(item->rot.y)) >> W2V_SHIFT)
        - item->pos.z;
    const int32_t x = enemy->pos.x
        - ((obj->pivot_length * Math_Sin(item->rot.y)) >> W2V_SHIFT)
        - item->pos.x;
    const int16_t angle = Math_Atan(z, x);

    if (creature->enemy == nullptr) {
        info->distance = 0x7FFFFFFF;
    } else if (ABS(x) > M_MAX_DISTANCE || ABS(z) > M_MAX_DISTANCE) {
        info->distance = SQUARE(M_MAX_DISTANCE);
    } else {
        info->distance = SQUARE(x) + SQUARE(z);
    }

    info->angle = angle - item->rot.y;
    info->enemy_facing = angle - enemy->rot.y + DEG_180;
    info->ahead = info->angle > -FRONT_ARC && info->angle < FRONT_ARC;
    info->bite = info->ahead && ABS(enemy->pos.y - item->pos.y) <= STEP_L
        && (TR_VERSION == 1 || enemy->hit_points > 0);
}

bool Creature_EnsureHabitat(
    const int16_t item_num, int32_t *const wh, const HYBRID_INFO *const info)
{
    // Test the environment for a hybrid creature. Record the water height and
    // return whether or not a type conversion has taken place.
    const ITEM *const item = Item_Get(item_num);
    *wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);

    return item->object_id == info->land.id
        ? M_SwitchToWater(item_num, wh, info)
        : M_SwitchToLand(item_num, wh, info);
}

void Creature_Mood(
    const ITEM *const item, const AI_INFO *const info, const bool violent)
{
    CREATURE *const creature = item->data;
    if (creature == nullptr) {
        return;
    }

    LOT_INFO *const lot = &creature->lot;
    const ITEM *enemy = TR_VERSION >= 2 ? creature->enemy : Lara_GetItem();
    if (lot->node[item->box_num].search_num
        == (lot->search_num | BOX_BLOCKED_SEARCH)) {
        lot->required_box = NO_BOX;
    }

    if (creature->mood != MOOD_ATTACK && lot->required_box != NO_BOX
        && !Box_ValidBox(item, info->zone_num, lot->target_box)) {
        if (info->zone_num == info->enemy_zone_num) {
            creature->mood = MOOD_BORED;
        }
        lot->required_box = NO_BOX;
    }

    const MOOD_TYPE mood = creature->mood;
    if (enemy == nullptr) {
        creature->mood = MOOD_BORED;
        enemy = Lara_GetItem();
    } else if (enemy->hit_points <= 0) {
        creature->mood = MOOD_BORED;
    } else if (violent) {
        switch (mood) {
        case MOOD_BORED:
        case MOOD_STALK:
            if (info->zone_num == info->enemy_zone_num) {
                creature->mood = MOOD_ATTACK;
            } else if (item->hit_status) {
                creature->mood = MOOD_ESCAPE;
            }
            break;

        case MOOD_ATTACK:
            if (info->zone_num != info->enemy_zone_num) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_num == info->enemy_zone_num) {
                creature->mood = MOOD_ATTACK;
            }
            break;
        }
    } else {
        switch (mood) {
        case MOOD_BORED:
        case MOOD_STALK:
            if (item->hit_status
                && (Random_GetControl() < M_ESCAPE_CHANCE
                    || info->zone_num != info->enemy_zone_num)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_num == info->enemy_zone_num) {
                if (info->distance < M_ATTACK_RANGE
                    || (creature->mood == MOOD_STALK
                        && lot->required_box == NO_BOX)) {
                    creature->mood = MOOD_ATTACK;
                } else {
                    creature->mood = MOOD_STALK;
                }
            }
            break;

        case MOOD_ATTACK:
            if (item->hit_status
                && (Random_GetControl() < M_ESCAPE_CHANCE
                    || info->zone_num != info->enemy_zone_num)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_num != info->enemy_zone_num) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_num == info->enemy_zone_num
                && Random_GetControl() < M_RECOVER_CHANCE) {
                creature->mood = MOOD_STALK;
            }
            break;
        }
    }

    if (mood != creature->mood) {
        if (mood == MOOD_ATTACK) {
            Box_TargetBox(lot, lot->target_box);
        }
        lot->required_box = NO_BOX;
    }

    switch (creature->mood) {
    case MOOD_BORED: {
        const int16_t box_num =
            lot->node[lot->zone_count * Random_GetControl() / 0x7FFF].box_num;
        if (Box_ValidBox(item, info->zone_num, box_num)) {
            if (Box_StalkBox(item, enemy, box_num)
                && (TR_VERSION == 1
                    || (creature->enemy != nullptr && enemy->hit_points > 0))) {
                Box_TargetBox(lot, box_num);
                creature->mood = MOOD_STALK;
            } else if (lot->required_box == NO_BOX) {
                Box_TargetBox(lot, box_num);
            }
        }
        break;
    }

    case MOOD_ATTACK:
        if (TR_VERSION >= 2
            || Random_GetControl() < Object_Get(item->object_id)->smartness) {
            lot->target = enemy->pos;
            lot->required_box = enemy->box_num;
            if (lot->fly != 0
                && Lara_GetLaraInfo()->water_status == LWS_ABOVE_WATER) {
                lot->target.y += Item_GetBestFrame(enemy)->bounds.min.y;
            }
        }
        break;

    case MOOD_ESCAPE: {
        const int16_t box_num =
            lot->node[lot->zone_count * Random_GetControl() / 0x7FFF].box_num;
        if (Box_ValidBox(item, info->zone_num, box_num)
            && lot->required_box == NO_BOX) {
            if (Box_EscapeBox(item, enemy, box_num)) {
                Box_TargetBox(lot, box_num);
            } else if (
                info->zone_num == info->enemy_zone_num
                && Box_StalkBox(item, enemy, box_num)) {
                Box_TargetBox(lot, box_num);
                creature->mood = MOOD_STALK;
            }
        }
        break;
    }

    case MOOD_STALK: {
        if (lot->required_box == NO_BOX
            || !Box_StalkBox(item, enemy, lot->required_box)) {
            const int16_t box_num =
                lot->node[lot->zone_count * Random_GetControl() / 0x7FFF]
                    .box_num;
            if (Box_ValidBox(item, info->zone_num, box_num)) {
                if (Box_StalkBox(item, enemy, box_num)) {
                    Box_TargetBox(lot, box_num);
                } else if (lot->required_box == NO_BOX) {
                    Box_TargetBox(lot, box_num);
                    if (info->zone_num != info->enemy_zone_num) {
                        creature->mood = MOOD_BORED;
                    }
                }
            }
        }
        break;
    }
    }

    if (lot->target_box == NO_BOX) {
        Box_TargetBox(lot, item->box_num);
    }
    Box_CalculateTarget(&creature->target, item, lot);
}

int16_t Creature_Turn(ITEM *const item, int16_t max_turn)
{
    const CREATURE *const creature = item->data;
    if (creature == nullptr || item->speed == 0 || max_turn == 0) {
        return 0;
    }

    const int32_t dx = creature->target.x - item->pos.x;
    const int32_t dz = creature->target.z - item->pos.z;

    int16_t angle = Math_Atan(dz, dx) - item->rot.y;
    if (angle > FRONT_ARC || angle < -FRONT_ARC) {
        const int32_t range = (item->speed << 14) / max_turn;
        if (SQUARE(dx) + SQUARE(dz) < SQUARE(range)) {
            max_turn /= 2;
        }
    }

    CLAMP(angle, -max_turn, max_turn);
    item->rot.y += angle;
    return angle;
}

void Creature_Tilt(ITEM *const item, int16_t angle)
{
    angle = angle * 4 - item->rot.z;
    CLAMP(angle, -M_MAX_TILT, M_MAX_TILT);
    item->rot.z += angle;
}

void Creature_Head(ITEM *const item, const int16_t required)
{
    CREATURE *const creature = item->data;
    if (creature == nullptr) {
        return;
    }

    int16_t change = required - creature->head_rotation;
    CLAMP(change, -M_MAX_HEAD_CHANGE, M_MAX_HEAD_CHANGE);

    creature->head_rotation += change;
    CLAMP(creature->head_rotation, -M_HEAD_ARC, M_HEAD_ARC);
}
