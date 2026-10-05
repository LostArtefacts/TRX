#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_EQUIPPED_GUN_BITS     0b01111111'11000000'00010000 // = 0x7FC010
#define M_EQUIPPED_SWORD_1_BITS 0b01111110'00001000'10000000 // = 0x7E0880
#define M_EQUIPPED_SWORD_2_BITS 0b00000000'00001000'10000000 // = 0x000880
#define M_HOLSTERED_BITS        0b01111111'11001000'00000000 // = 0x7FC800
#define M_SWORD_TOUCH_BITS      0b00000001'11000000'00000000 // = 0x01C000
#define M_HIT_POINTS_1          25
#define M_HIT_POINTS_2          35
#define M_GUN_DAMAGE            15
#define M_SWORD_DAMAGE          120
#define M_PIVOT                 50
#define M_AMMO_QTY              24
#define M_RADIUS                (WALL_L / 10) // = 102
#define M_STEP_AHEAD            (WALL_L - 82) // = 942
#define M_WALK_DIST             SQUARE(WALL_L) // = 1048576
#define M_ATTACK_DIST           SQUARE(WALL_L * 2 / 3) // = 465124
#define M_HIT_DIST              SQUARE(WALL_L / 2) // = 262144
#define M_RUN_STOP_DIST         SQUARE((WALL_L / 2) + M_RADIUS) // = 376996
#define M_SOMERSAULT_DIST       SQUARE(WALL_L * 3) // = 9437184
#define M_SOMERSAULT_ANGLE      (DEG_45 / 2) // = 4096
#define M_JUMP_ROLL_ANGLE       (DEG_45 * 5 / 4) // = 10240
#define M_JUMP_ROLL_TURN        (DEG_45 * 7 / 4) // = 14336
#define M_WALK_TURN             (DEG_1 * 7) // = 1274
#define M_RUN_TURN              (DEG_1 * 11) // = 2002
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_RUN                 = 0,
    M_ANIM_SOMERSAULT_END      = 4,
    M_ANIM_STAND_IDLE          = 18,
    M_ANIM_STAND_TO_ROLL_LEFT  = 24,
    M_ANIM_CROUCH              = 29,
    M_ANIM_STAND_DEATH         = 45,
    M_ANIM_STAND_TO_JUMP_RIGHT = 47,
    M_ANIM_JUMP_FORWARD_START  = 55,
    M_ANIM_MONKEY_TO_FREEFALL  = 59,
    M_ANIM_CLIMB_4_CLICKS      = 62,
    M_ANIM_CLIMB_3_CLICKS      = 63,
    M_ANIM_CLIMB_2_CLICKS      = 64,
    M_ANIM_JUMP_DOWN_4_CLICKS  = 65,
    M_ANIM_JUMP_DOWN_3_CLICKS  = 66,
    M_ANIM_BLIND               = 68,
    // clang-format on
} M_ANIM;

typedef enum {
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_UNUSED_1,
    M_STATE_DODGE_START_1,
    M_STATE_STALK,
    M_STATE_DODGE_START_2,
    M_STATE_UNUSED_2,
    M_STATE_DODGE,
    M_STATE_DODGE_END,
    M_STATE_DRAW_GUN,
    M_STATE_HOLSTER_GUN,
    M_STATE_DRAW_SWORD,
    M_STATE_HOLSTER_SWORD,
    M_STATE_SHOOT,
    M_STATE_SWORD_HIT_FRONT,
    M_STATE_SWORD_HIT_RIGHT,
    M_STATE_SWORD_HIT_LEFT,
    M_STATE_MONKEY_GRAB,
    M_STATE_MONKEY_IDLE,
    M_STATE_MONKEY_FORWARD,
    M_STATE_MONKEY_PUSH,
    M_STATE_MONKEY_FALL_LAND,
    M_STATE_ROLL_LEFT,
    M_STATE_JUMP_RIGHT,
    M_STATE_STAND_TO_CROUCH,
    M_STATE_CROUCH,
    M_STATE_CROUCH_PICKUP,
    M_STATE_CROUCH_STAND,
    M_STATE_WALK_SWORD_HIT_RIGHT,
    M_STATE_SOMERSAULT,
    M_STATE_AIM,
    M_STATE_DEATH,
    M_STATE_JUMP_FORWARD_1_BLOCK,
    M_STATE_JUMP_FORWARD_FALL,
    M_STATE_MONKEY_TO_FREEFALL,
    M_STATE_FREEFALL,
    M_STATE_FREEFALL_LAND_DEATH,
    M_STATE_JUMP_FORWARD_2_BLOCKS,
    M_STATE_CLIMB_4_CLICKS,
    M_STATE_CLIMB_3_CLICKS,
    M_STATE_CLIMB_2_CLICKS,
    M_STATE_JUMP_DOWN_4_CLICKS,
    M_STATE_JUMP_DOWN_3_CLICKS,
    M_STATE_BLIND,
} M_STATE;

typedef struct {
    bool seeks_equipment;
    int16_t equipment_room_num;
    int32_t swap_bits;
    int32_t gun_damage;
    int32_t sword_damage;
    int16_t ammo;
    int32_t ocb;
    int16_t ai_x1_ocb;
    int16_t x_shift;
} M_PRIV;

static const LARA_STATE_ID m_MonkeyStates[] = {
    // clang-format off
    LS_MONKEY_IDLE,
    LS_MONKEY_FORWARD,
    LS_MONKEY_LEFT,
    LS_MONKEY_RIGHT,
    LS_MONKEY_ROLL,
    LS_MONKEY_TURN_LEFT,
    LS_MONKEY_TURN_RIGHT,
    NO_CATALOG_ID,
    // clang-format on
};

static const CREATURE_GUN m_Gun = {
    .muzzle = { .pos = { 0, -16, 200 }, .mesh_num = 11 },
    .tr3_flash = { .pos = { 0, 180, 30 }, .mesh_num = 11 },
    .tr3_enemy_flash = true,
    .tr3_flash_rot_x = -DEG_90,
    .tr3_flash_shade = 600,
};

static const BITE m_Blade = {
    .pos = {},
    .mesh_num = 15,
};

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ(io, "equipment_room_num", &p->equipment_room_num));
    MUST(JSON_READ(io, "ammo", &p->ammo));
    MUST(JSON_READ(io, "swap_bits", &p->swap_bits));
    MUST(JSON_READ(io, "x_shift", &p->x_shift));
    MUST(JSON_READ(io, "ocb", &p->ocb));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "equipment_room_num", p->equipment_room_num);
    JSONW_WRITE(io, "ammo", p->ammo);
    JSONW_WRITE(io, "swap_bits", p->swap_bits);
    JSONW_WRITE(io, "x_shift", p->x_shift);
    JSONW_WRITE(io, "ocb", p->ocb);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    // Creature_Initialise may rotate the enemy but initialisation animations
    // that follow from the OCB may rely on the original angle.
    const int16_t yaw = item->rot.y;
    Creature_Initialise(item_num);

    p->equipment_room_num = NO_ROOM;
    if (item->object_id == O_HENCHMAN_1) {
        p->swap_bits = M_EQUIPPED_GUN_BITS;
        p->ammo = M_AMMO_QTY;
    } else {
        p->swap_bits = M_EQUIPPED_SWORD_2_BITS;
    }

    // The OCB's upper digits were used in OG for baddie spawn chains. TRX
    // handles those in Lua, but the lower digits still contain a one-shot
    // control action for Desert Railroad, so the raw OCB is retained for now.
    // TODO: revisit and split the remaining OCB encodings into properties.
    p->ocb = M_GetOCB(item);

    int32_t init_mode = p->ocb % 1000;
    if (init_mode > 9 && init_mode < 20) {
        p->ammo += M_AMMO_QTY;
        p->ocb -= 10;
        init_mode -= 10;
    }

    M_ANIM anim_idx = M_ANIM_RUN;
    if (init_mode == 0 || (init_mode > 4 && init_mode < 7)) {
        anim_idx = M_ANIM_STAND_IDLE;
    } else if (init_mode == 1) {
        anim_idx = M_ANIM_STAND_TO_JUMP_RIGHT;
    } else if (init_mode == 2) {
        anim_idx = M_ANIM_STAND_TO_ROLL_LEFT;
    } else if (init_mode == 3) {
        anim_idx = M_ANIM_CROUCH;
    } else if (init_mode == 4) {
        anim_idx = M_ANIM_CLIMB_4_CLICKS;
        item->rot.y = yaw;
        item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, STEP_L);
    } else if (init_mode > 100) {
        anim_idx = M_ANIM_CROUCH;
        item->rot.y = yaw;
        item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, STEP_L);
        p->ai_x1_ocb = init_mode;
    }

    Item_SwitchToAnim(item, anim_idx, 0);
    const ANIM *const anim = Item_GetAnim(item);
    item->current_anim_state = anim->current_anim_state;
    item->goal_anim_state = anim->current_anim_state;
}

static bool M_IsEquipmentItem(const ITEM *const item)
{
    return item != nullptr
        && (item->object_id == O_SMALL_MEDIPACK_ITEM
            || item->object_id == O_UZIS_AMMO_ITEM);
}

static void M_TargetEquipment(const ITEM *const item, CREATURE *const creature)
{
    M_PRIV *const p = item->priv;
    if (item->room_num == p->equipment_room_num || !p->seeks_equipment) {
        return;
    }

    const ROOM *const room = Room_Get(item->room_num);
    int16_t item_num = room->item_num;
    while (item_num != NO_ITEM) {
        ITEM *const target_item = Item_Get(item_num);
        item_num = target_item->next_item;
        if (!M_IsEquipmentItem(target_item) || !target_item->is_visible) {
            continue;
        }

        if (Creature_SameZone(creature, target_item)) {
            creature->enemy = target_item;
            break;
        }
    }

    p->equipment_room_num = item->room_num;
}

static void M_HandleDeath(ITEM *const item, CREATURE *const creature)
{
    creature->lot.is_jumping = false;
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    item->floor = Room_GetHeight(sector, item->pos);

    switch (item->current_anim_state) {
    case M_STATE_MONKEY_GRAB:
    case M_STATE_MONKEY_IDLE:
    case M_STATE_MONKEY_FORWARD:
        Item_SwitchToAnim(item, M_ANIM_MONKEY_TO_FREEFALL, 0);
        item->current_anim_state = M_STATE_MONKEY_TO_FREEFALL;
        item->speed = 0;
        break;

    case M_STATE_DEATH:
        item->gravity = true;
        creature->lot.is_jumping = true;
        if (item->pos.y >= item->floor) {
            item->pos.y = item->floor;
            item->fall_speed = 0;
            item->gravity = false;
        }
        break;

    case M_STATE_MONKEY_TO_FREEFALL:
        item->goal_anim_state = M_STATE_FREEFALL;
        item->gravity = false;
        break;

    case M_STATE_FREEFALL:
        item->gravity = true;
        if (item->pos.y >= item->floor) {
            item->pos.y = item->floor;
            item->fall_speed = 0;
            item->gravity = false;
            item->goal_anim_state = M_STATE_FREEFALL_LAND_DEATH;
        }
        break;

    case M_STATE_FREEFALL_LAND_DEATH:
        item->pos.y = item->floor;
        break;

    default:
        Item_SwitchToAnim(item, M_ANIM_STAND_DEATH, 0);
        item->current_anim_state = M_STATE_DEATH;
        creature->lot.is_jumping = true;
        break;
    }
}

static void M_ProbeJumpRoll(
    const ITEM *const item, bool *const can_jump, bool *const can_roll)
{
    int16_t room_num = item->room_num;
    XYZ_32 pos =
        XYZ_32_OffsetYaw(item->pos, item->rot.y + DEG_45, M_STEP_AHEAD);
    const SECTOR *sector = Room_GetSector(pos, &room_num);
    int32_t h1 = Room_GetHeight(sector, pos);

    room_num = item->room_num;
    pos = XYZ_32_OffsetYaw(
        item->pos, item->rot.y + M_JUMP_ROLL_TURN, M_STEP_AHEAD);
    sector = Room_GetSector(pos, &room_num);
    int32_t h2 = Room_GetHeight(sector, pos);

    *can_jump =
        ABS(h2 - item->pos.y) <= STEP_L && h1 + (STEP_L * 2) < item->pos.y;

    room_num = item->room_num;
    pos = XYZ_32_OffsetYaw(item->pos, item->rot.y - DEG_45, M_STEP_AHEAD);
    sector = Room_GetSector(pos, &room_num);
    h1 = Room_GetHeight(sector, pos);

    room_num = item->room_num;
    pos = XYZ_32_OffsetYaw(
        item->pos, item->rot.y - M_JUMP_ROLL_TURN, M_STEP_AHEAD);
    sector = Room_GetSector(pos, &room_num);
    h2 = Room_GetHeight(sector, pos);

    *can_roll =
        ABS(h2 - item->pos.y) <= STEP_L && h1 + (STEP_L * 2) < item->pos.y;
}

static bool M_TestMonkeyHeight(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, item->pos);
    const int32_t ceiling = Room_GetCeiling(sector, item->pos);
    return ceiling == height - WALL_L * 3 / 2;
}

static bool M_TouchesDeadlyFloor(const ITEM *const item)
{
    // TODO: replace with train GF flag during Desert Railroad implementation.
    const bool is_train = GF_GetCurrentLevel()->num == 12;
    return is_train && item->pos.y > -STEP_L;
}

static bool M_Vault(ITEM *const item, const int16_t angle)
{
    const int32_t vault_result =
        Creature_Vault(Item_GetIndex(item), angle, 2, 260);
    switch (vault_result) {
    case -4:
        Item_SwitchToAnim(item, M_ANIM_JUMP_DOWN_4_CLICKS, 0);
        item->current_anim_state = M_STATE_JUMP_DOWN_4_CLICKS;
        return true;
    case -3:
        Item_SwitchToAnim(item, M_ANIM_JUMP_DOWN_3_CLICKS, 0);
        item->current_anim_state = M_STATE_JUMP_DOWN_3_CLICKS;
        return true;
    case 2:
        Item_SwitchToAnim(item, M_ANIM_CLIMB_2_CLICKS, 0);
        item->current_anim_state = M_STATE_CLIMB_2_CLICKS;
        return true;
    case 3:
        Item_SwitchToAnim(item, M_ANIM_CLIMB_3_CLICKS, 0);
        item->current_anim_state = M_STATE_CLIMB_3_CLICKS;
        return true;
    case 4:
        Item_SwitchToAnim(item, M_ANIM_CLIMB_4_CLICKS, 0);
        item->current_anim_state = M_STATE_CLIMB_4_CLICKS;
        return true;
    default:
        return false;
    }
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    CREATURE *const creature = item->creature_data;
    int16_t angle = 0;
    int16_t tilt = 0;
    int16_t head = 0;
    int16_t torso_x = 0;
    int16_t torso_y = 0;

    if (p->ocb % 1000 != 0) {
        creature->lot.is_jumping = true;
        creature->maximum_turn = 0;

        if (p->ocb % 1000 > 100) {
            p->x_shift = -80;
            Creature_FindAITargetObject(creature, O_AI_X1, p->ai_x1_ocb);
        }
        p->ocb = 1000 * (p->ocb / 1000);
    }

    CREATURE_PROBE probe = {};
    Creature_ProbeAhead(item, M_STEP_AHEAD, &probe);
    if (creature->enemy != nullptr
        && item->box_num == creature->enemy->box_num) {
        probe.jump_ahead = false;
        probe.long_jump_ahead = false;
    }

    M_TargetEquipment(item, creature);

    if (item->hit_points <= 0) {
        M_HandleDeath(item, creature);
        goto finish;
    }

    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item->ai_bits != 0) {
        Creature_GetAITarget(creature);
    } else if (creature->enemy == nullptr) {
        creature->enemy = lara_item;
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);

    AI_INFO lara_info = {};
    if (creature->enemy == lara_item) {
        lara_info.angle = info.angle;
        lara_info.ahead = info.ahead;
        lara_info.distance = info.distance;
    } else {
        const XYZ_32 delta = XYZ_32_Subtract(lara_item->pos, item->pos);
        lara_info.angle = Math_Atan(delta.z, delta.x) - item->rot.y;
        lara_info.ahead = lara_info.angle > -DEG_90 && lara_info.angle < DEG_90;
        lara_info.distance = SQUARE(delta.x) + SQUARE(delta.z);
    }

    Creature_UpdateMood(item, &info, true);
    if (info.bite && Lara_Vehicle_IsMounted()) {
        creature->mood = MOOD_ESCAPE;
    }
    Creature_ApplyMood(item, &info, true);
    angle = Creature_Turn(item, creature->maximum_turn);

    ITEM *const enemy = creature->enemy;
    if (item->hit_status
        || ((lara_info.distance < M_WALK_DIST
             || Creature_CanSeeEnemy(item, &lara_info))
            && ABS(lara_item->pos.y - item->pos.y) < WALL_L)) {
        creature->alerted = true;
    }
    creature->enemy = enemy;

    bool can_jump = false;
    bool can_roll = false;
    if (lara->target == item && lara_info.distance > M_STEP_AHEAD
        && ABS(lara_info.angle) < M_JUMP_ROLL_ANGLE) {
        M_ProbeJumpRoll(item, &can_jump, &can_roll);
    }

    const int16_t frame_num = Item_GetRelativeFrame(item);

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        creature->lot.is_jumping = false;
        creature->lot.is_monkeying = false;
        creature->flags = 0;
        creature->maximum_turn = 0;
        head = info.angle >> 1;

        if (info.ahead && item->ai_bits != AI_GUARD) {
            torso_y = info.angle >> 1;
            torso_x = info.x_angle;
        }

        if ((item->ai_bits & AI_GUARD) != 0) {
            head = Creature_AIGuard(creature);
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            p->swap_bits == M_EQUIPPED_SWORD_2_BITS && lara->target == item
            && lara_info.ahead && lara_info.distance > M_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_DODGE_START_1;
        } else if (Creature_CanTargetEnemy(item, &info) && p->ammo > 0) {
            if (p->swap_bits == M_EQUIPPED_GUN_BITS) {
                item->goal_anim_state = M_STATE_AIM;
            } else if (
                p->swap_bits == M_EQUIPPED_SWORD_1_BITS
                || p->swap_bits == M_EQUIPPED_SWORD_2_BITS) {
                item->goal_anim_state = M_STATE_HOLSTER_SWORD;
            } else {
                item->goal_anim_state = M_STATE_DRAW_GUN;
            }
        } else if (item->ai_bits == AI_MODIFY) {
            item->goal_anim_state = M_STATE_STOP;
            if (item->floor > item->pos.y + STEP_L * 3) {
                item->ai_bits &= ~AI_MODIFY;
            }
        } else if (probe.jump_ahead || probe.long_jump_ahead) {
            creature->maximum_turn = 0;
            creature->lot.is_jumping = true;
            Item_SwitchToAnim(item, M_ANIM_JUMP_FORWARD_START, 0);
            item->current_anim_state = M_STATE_JUMP_FORWARD_1_BLOCK;
            item->goal_anim_state = probe.long_jump_ahead
                ? M_STATE_JUMP_FORWARD_2_BLOCKS
                : M_STATE_JUMP_FORWARD_1_BLOCK;
        } else if (M_IsEquipmentItem(enemy) && info.distance < M_HIT_DIST) {
            item->goal_anim_state = M_STATE_STAND_TO_CROUCH;
            item->required_anim_state = M_STATE_CROUCH_PICKUP;
        } else if (p->swap_bits == M_EQUIPPED_GUN_BITS && p->ammo < 1) {
            item->goal_anim_state = M_STATE_HOLSTER_GUN;
        } else if (creature->monkey_ahead) {
            if (M_TestMonkeyHeight(item)) {
                if (p->swap_bits == M_HOLSTERED_BITS) {
                    item->goal_anim_state = M_STATE_MONKEY_GRAB;
                } else if (p->swap_bits == M_EQUIPPED_GUN_BITS) {
                    item->goal_anim_state = M_STATE_HOLSTER_GUN;
                } else {
                    item->goal_anim_state = M_STATE_HOLSTER_SWORD;
                }
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
        } else if (can_roll) {
            creature->maximum_turn = 0;
            item->goal_anim_state = M_STATE_ROLL_LEFT;
        } else if (can_jump) {
            creature->maximum_turn = 0;
            item->goal_anim_state = M_STATE_JUMP_RIGHT;
        } else if (p->swap_bits == M_HOLSTERED_BITS) {
            item->goal_anim_state = M_STATE_DRAW_SWORD;
        } else if (
            enemy != nullptr && enemy->hit_points > 0
            && info.distance < M_ATTACK_DIST) {
            if (p->swap_bits == M_EQUIPPED_GUN_BITS) {
                item->goal_anim_state = M_STATE_HOLSTER_GUN;
            } else if (info.distance >= M_HIT_DIST) {
                item->goal_anim_state = M_STATE_SWORD_HIT_FRONT;
            } else if ((Random_GetControl() & 1) != 0) {
                item->goal_anim_state = M_STATE_SWORD_HIT_LEFT;
            } else {
                item->goal_anim_state = M_STATE_SWORD_HIT_RIGHT;
            }
        } else {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_WALK:
        creature->lot.is_jumping = false;
        creature->lot.is_monkeying = false;
        creature->maximum_turn = M_WALK_TURN;
        creature->flags = 0;
        if (lara_info.ahead) {
            head = lara_info.angle;
        } else if (info.ahead) {
            head = info.angle;
        }

        if (Creature_CanTargetEnemy(item, &info) && p->ammo > 0) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (probe.jump_ahead || probe.long_jump_ahead) {
            creature->maximum_turn = 0;
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->reached_goal || creature->monkey_ahead) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            p->ammo > 0 || p->swap_bits == M_EQUIPPED_SWORD_1_BITS
            || p->swap_bits == M_EQUIPPED_SWORD_2_BITS) {
            if (info.ahead && info.distance < M_HIT_DIST) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (info.bite && info.distance < M_ATTACK_DIST) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (info.bite && info.distance < M_WALK_DIST) {
                item->goal_anim_state = M_STATE_WALK_SWORD_HIT_RIGHT;
            } else if (can_roll || can_jump) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (
                creature->mood == MOOD_ATTACK && !creature->jump_ahead
                && info.distance > M_WALK_DIST) {
                item->goal_anim_state = M_STATE_RUN;
            }
        } else {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_RUN:
        creature->maximum_turn = M_RUN_TURN;
        tilt = angle / 2;
        if (info.ahead) {
            head = info.angle;
        }

        if (item->object_id == O_HENCHMAN_2 && Item_TestFrameEqual(item, 11)
            && probe.far_height == probe.near_height
            && ABS(probe.near_height - item->pos.y) < STEPUP_HEIGHT
            && ((ABS(info.angle) < M_SOMERSAULT_ANGLE
                 && info.distance < M_SOMERSAULT_DIST)
                || probe.mid_height >= probe.near_height + (STEP_L * 2))) {
            item->goal_anim_state = M_STATE_SOMERSAULT;
            creature->maximum_turn = 0;
        } else if (Creature_CanTargetEnemy(item, &info) && p->ammo > 0) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            probe.jump_ahead || probe.long_jump_ahead || creature->monkey_ahead
            || item->ai_bits == AI_GUARD) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.distance < M_RUN_STOP_DIST || creature->jump_ahead) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.distance < M_WALK_DIST) {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_DODGE:
        creature->maximum_turn = 0;
        Creature_TurnTo(item, info.angle, M_RUN_TURN);
        if (lara_info.distance < M_ATTACK_DIST || lara->target != item) {
            item->goal_anim_state = M_STATE_DODGE_END;
        }
        break;

    case M_STATE_DRAW_GUN:
        if (Item_TestFrameEqual(item, 21)) {
            p->swap_bits = M_EQUIPPED_GUN_BITS;
        }
        break;

    case M_STATE_HOLSTER_GUN:
        if (Item_TestFrameEqual(item, 20)) {
            p->swap_bits = M_HOLSTERED_BITS;
        }
        break;

    case M_STATE_DRAW_SWORD:
        if (Item_TestFrameEqual(item, 12)) {
            if (item->object_id == O_HENCHMAN_1) {
                p->swap_bits = M_EQUIPPED_SWORD_1_BITS;
            } else {
                p->swap_bits = M_EQUIPPED_SWORD_2_BITS;
            }
        }
        break;

    case M_STATE_HOLSTER_SWORD:
        if (Item_TestFrameEqual(item, 22)) {
            p->swap_bits = M_HOLSTERED_BITS;
        }
        break;

    case M_STATE_SHOOT:
        if (info.ahead) {
            torso_y = info.angle;
            torso_x = info.x_angle;
        }
        Creature_TurnTo(item, info.angle, M_WALK_TURN);

        if (frame_num < 13 && (frame_num & 1) == 0) {
            if ((item->ai_bits & AI_MODIFY) == 0) {
                p->ammo--;
            }
            if (!Creature_Shoot(item, &info, &m_Gun, torso_y, p->gun_damage)) {
                item->goal_anim_state = M_STATE_STOP;
            }
        }
        break;

    case M_STATE_SWORD_HIT_FRONT:
    case M_STATE_SWORD_HIT_RIGHT:
    case M_STATE_SWORD_HIT_LEFT:
    case M_STATE_WALK_SWORD_HIT_RIGHT:
        creature->maximum_turn = 0;
        if (info.ahead) {
            torso_y = info.angle;
            torso_x = info.x_angle;
        }

        if (item->current_anim_state == M_STATE_SWORD_HIT_RIGHT
            && info.distance < M_HIT_DIST) {
            item->goal_anim_state = M_STATE_SWORD_HIT_LEFT;
        }

        if (item->current_anim_state != M_STATE_SWORD_HIT_FRONT
            || frame_num < 12) {
            if (ABS(info.angle) < M_WALK_TURN) {
                item->rot.y += info.angle;
            } else if (info.angle < 0) {
                item->rot.y -= M_WALK_TURN;
            } else {
                item->rot.y += M_WALK_TURN;
            }
        }

        if (creature->flags == 0 && (item->touch_bits & M_SWORD_TOUCH_BITS) != 0
            && frame_num > 13 && frame_num < 21) {
            Lara_TakeDamage(p->sword_damage, true);
            Creature_EffectEx(item, &m_Blade, 10, item->rot.y, Spawn_Blood);
            creature->flags = 1;
        }

        if (Item_TestFrameEqual(item, -2)) {
            creature->flags = 0;
        }
        break;

    case M_STATE_MONKEY_IDLE:
        torso_x = 0;
        torso_y = 0;
        creature->maximum_turn = 0;
        creature->flags = 0;

        if (lara_info.ahead && lara_info.distance < M_ATTACK_DIST
            && Lara_HasState(m_MonkeyStates)) {
            item->goal_anim_state = M_STATE_MONKEY_PUSH;
        } else if (
            item->box_num == creature->lot.target_box
            || !creature->monkey_ahead) {
            if (M_TestMonkeyHeight(item)) {
                item->goal_anim_state = M_STATE_MONKEY_FALL_LAND;
                creature->lot.is_jumping = false;
                creature->lot.is_monkeying = false;
            } else {
                item->goal_anim_state = M_STATE_MONKEY_FORWARD;
            }
        } else {
            item->goal_anim_state = M_STATE_MONKEY_FORWARD;
        }
        break;

    case M_STATE_MONKEY_FORWARD:
        torso_x = 0;
        torso_y = 0;
        creature->lot.is_jumping = true;
        creature->lot.is_monkeying = true;
        creature->flags = 0;
        creature->maximum_turn = M_WALK_TURN;

        if ((item->box_num == creature->lot.target_box
             || !creature->monkey_ahead)
            && M_TestMonkeyHeight(item)) {
            item->goal_anim_state = M_STATE_MONKEY_IDLE;
        }

        if (lara_info.ahead && lara_info.distance < M_ATTACK_DIST
            && Lara_HasState(m_MonkeyStates)) {
            item->goal_anim_state = M_STATE_MONKEY_IDLE;
        }
        break;

    case M_STATE_MONKEY_PUSH:
        creature->maximum_turn = M_WALK_TURN;
        if (creature->flags == 0 && item->touch_bits != 0) {
            Item_SwitchToAnim(lara_item, LA(LA_JUMP_UP), 9);
            lara_item->current_anim_state = LS(LS_JUMP_UP);
            lara_item->goal_anim_state = LS(LS_JUMP_UP);
            lara_item->gravity = true;
            lara_item->speed = 2;
            lara_item->fall_speed = 1;
            lara_item->pos.y += STEP_L * 3 / 4;
            lara->gun_status = LGS_ARMLESS;
            creature->flags = 1;
        }
        break;

    case M_STATE_ROLL_LEFT:
    case M_STATE_JUMP_RIGHT:
        creature->alerted = false;
        creature->maximum_turn = 0;
        item->ai_bits |= AI_GUARD;
        break;

    case M_STATE_CROUCH:
        if (p->x_shift != 0) {
            if (info.distance < M_ATTACK_DIST) {
                item->goal_anim_state = M_STATE_CROUCH_STAND;
                creature->enemy = nullptr;
            }
        } else if (M_IsEquipmentItem(enemy) && info.distance < M_HIT_DIST) {
            item->goal_anim_state = M_STATE_CROUCH_PICKUP;
        } else if (creature->alerted) {
            item->goal_anim_state = M_STATE_CROUCH_STAND;
        }
        break;

    case M_STATE_CROUCH_PICKUP:
        Creature_TurnTo(item, info.angle, M_RUN_TURN);
        if (!Item_TestFrameEqual(item, 9)
            || !M_IsEquipmentItem(creature->enemy)) {
            break;
        }

        if (creature->enemy->room_num == NO_ROOM || !creature->enemy->is_visible
            || creature->enemy->clear_body) {
            creature->enemy = nullptr;
            break;
        }

        if (creature->enemy->object_id == O_SMALL_MEDIPACK_ITEM) {
            item->hit_points += item->max_hit_points / 2;
            CLAMPG(item->hit_points, item->max_hit_points);
        } else {
            p->ammo += M_AMMO_QTY;
        }

        Item_Destroy(Item_GetIndex(creature->enemy));
        for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
            CREATURE *const slot = LOT_GetBaddieSlot(i);
            if (slot->item_num != NO_ITEM && slot->item_num != item_num
                && slot->enemy == creature->enemy) {
                slot->enemy = nullptr;
            }
        }

        creature->enemy = nullptr;
        break;

    case M_STATE_SOMERSAULT:
        if (Item_TestAnimEqual(item, M_ANIM_SOMERSAULT_END)) {
            Creature_TurnTo(item, info.angle, M_WALK_TURN);
        } else if (Item_TestAnimEqual(item, M_ANIM_STAND_IDLE)) {
            creature->lot.is_jumping = true;
        }
        break;

    case M_STATE_AIM:
        creature->maximum_turn = 0;
        if (info.ahead) {
            torso_y = info.angle;
            torso_x = info.x_angle;
        }
        Creature_TurnTo(item, info.angle, M_WALK_TURN);

        if (Creature_CanTargetEnemy(item, &info) && p->ammo > 0) {
            item->goal_anim_state = M_STATE_SHOOT;
        } else {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_JUMP_FORWARD_1_BLOCK:
    case M_STATE_JUMP_FORWARD_2_BLOCKS:
        if (p->x_shift < 0
            && !Item_TestAnimEqual(item, M_ANIM_JUMP_FORWARD_START)) {
            p->x_shift += 2;
        }
        break;

    case M_STATE_BLIND:
        if (lara->blind_timer == 0 && (Random_GetControl() & 0x7F) == 0) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;
    }

finish:
    Creature_Tilt(item, tilt);
    Creature_Joint(item, 0, torso_y);
    Creature_Joint(item, 1, torso_x);
    Creature_Joint(item, 2, head);

    if (M_TouchesDeadlyFloor(item)) {
        Item_TakeFatalDamage(item, nullptr);
        p->x_shift = 0;
        item->pos.x += Output_GetUVRotateSpeed() << 5;
    } else if (p->x_shift < 0) {
        item->pos.x += p->x_shift;
    }

    switch (item->current_anim_state) {
    case M_STATE_JUMP_FORWARD_1_BLOCK:
    case M_STATE_JUMP_FORWARD_2_BLOCKS:
    case M_STATE_CLIMB_4_CLICKS:
    case M_STATE_CLIMB_3_CLICKS:
    case M_STATE_CLIMB_2_CLICKS:
    case M_STATE_JUMP_DOWN_4_CLICKS:
    case M_STATE_JUMP_DOWN_3_CLICKS:
    case M_STATE_SOMERSAULT:
    case M_STATE_MONKEY_FORWARD:
    case M_STATE_BLIND:
    case M_STATE_DEATH:
        Creature_Animate(item_num, angle, 0);
        break;

    default:
        if (lara->blind_timer > 100) {
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_BLIND, Random_GetControl() & 7);
            item->current_anim_state = M_STATE_BLIND;
        } else if (M_Vault(item, angle)) {
            creature->maximum_turn = 0;
        }
        break;
    }
}

static bool M_IsDodgingShots(const ITEM *const item)
{
    if (item->current_anim_state != M_STATE_DODGE) {
        if ((Random_GetControl() & 1) == 0) {
            return false;
        }
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->target != item) {
        return false;
    }

    // TODO: TR1-3 guns?
    switch (lara->gun_type) {
    case LGT_PISTOLS:
    case LGT_SHOTGUN:
    case LGT_UZIS:
        return true;
    default:
        return false;
    }
}

static bool M_GunHitHenchman2(
    ITEM *const item, const GAME_VECTOR *const start,
    const GAME_VECTOR *const hit_pos, int32_t *const damage)
{
    if (!M_IsDodgingShots(item)) {
        return true;
    }

    if (damage != nullptr) {
        *damage = 0;
    }

    Sound_Effect(SFX_SWORD_RICOCHET, &item->pos, SPM_NORMAL);
    if (start != nullptr) {
        Spawn_RicochetRay(*start, *hit_pos, 3, false);
    } else {
        Spawn_Ricochet(*hit_pos, false);
    }

    return false;
}

static bool M_Draw(const ITEM *const item)
{
    const OBJECT_ID obj_id =
        item->object_id == O_HENCHMAN_1 ? O_MESH_SWAP_3 : O_MESH_SWAP_2;
    const OBJECT *const swap = Object_Get(obj_id);
    if (!swap->loaded
        || swap->mesh_count < Object_Get(item->object_id)->mesh_count) {
        return Object_DrawAnimatingItem(item);
    }

    const M_PRIV *const p = item->priv;
    return Object_DrawAnimatingItemWithSwap(
        item, swap, item->mesh_bits & ~p->swap_bits);
}

static void M_SetupCommon(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->collision_func = Creature_Collision;
    obj->control_func = M_Control;
    obj->draw_func = M_Draw;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = M_PIVOT;
    obj->smartness = 0x7FFF;
    obj->lot_setup = LOT_Setup(LOT_SETUP_ACROBAT);

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->intelligent = true;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    Object_GetBone(obj, 7)->rot.x = true;
    Object_GetBone(obj, 7)->rot.y = true;
    Object_GetBone(obj, 22)->rot.x = true;
    Object_GetBone(obj, 22)->rot.y = true;
}

static void M_Setup1(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    M_SetupCommon(obj);

    const OBJECT *const ref_obj = Object_Get(O_HENCHMAN_2);
    if (ref_obj->loaded) {
        obj->frame_base = ref_obj->frame_base;
        obj->anim_idx = ref_obj->anim_idx;
    }

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS_1),
        OBJECT_PROPERTY(
            M_PRIV, seeks_equipment, true,
            "Whether the henchman will seek small medipack and uzi ammo "
            "pickups to replenish his kit."),
        OBJECT_PROPERTY(
            M_PRIV, gun_damage, M_GUN_DAMAGE,
            "Damage dealt by the henchman's gunshot."),
        OBJECT_PROPERTY(
            M_PRIV, sword_damage, M_SWORD_DAMAGE,
            "Damage dealt by the henchman's sword."));
}

static void M_Setup2(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    M_SetupCommon(obj);
    obj->gun_hit_func = M_GunHitHenchman2;

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS_2),
        OBJECT_PROPERTY(
            M_PRIV, seeks_equipment, true,
            "Whether the henchman will seek small medipack and uzi ammo "
            "pickups to replenish his kit."),
        OBJECT_PROPERTY(
            M_PRIV, gun_damage, M_GUN_DAMAGE,
            "Damage dealt by the henchman's gunshot."),
        OBJECT_PROPERTY(
            M_PRIV, sword_damage, M_SWORD_DAMAGE,
            "Damage dealt by the henchman's sword."));
}

REGISTER_OBJECT(O_HENCHMAN_1, M_Setup1)
REGISTER_OBJECT(O_HENCHMAN_2, M_Setup2)
