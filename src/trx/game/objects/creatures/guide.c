#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/creature.h>
#include <trx/game/fx/fire.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>
#include <trx/game/waypoint.h>

// clang-format off
#define M_WAYPOINT_UNSET (-1)
#define M_RADIUS         (STEP_L / 2) // = 128
#define M_HIT_POINTS     15
#define M_TORCH_BITS     0b000100'00000000'00000000
#define M_LIGHTER_BITS   0b000000'10000000'00000000
#define M_SCREAM_BITS    0b100000'00000000'00000000
#define M_SHIFT_SPEED    18
#define M_SHIFT_DIST     SQUARE(STEP_L / 2) // = 16384
#define M_WALK_DIST      SQUARE(WALL_L) // = 1048576
#define M_RUN_DIST       SQUARE(WALL_L * 2) // = 4194304
#define M_AWARE_DIST     SQUARE(WALL_L * 3) // = 9437184
#define M_ADJUST_TURN    (DEG_1 * 2) // = 364
#define M_WALK_TURN      (DEG_1 * 7) // = 1274
#define M_SHIFT_TURN     (DEG_1 * 10) // = 1820
#define M_RUN_TURN       (DEG_1 * 11) // = 2002
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_STAND         = 4,
    M_ANIM_BEGIN_CROUCH  = 57,
    M_ANIM_EXAMINE_SHORT = 61,
    // clang-format on
} M_ANIM;

typedef enum {
    // clang-format off
    M_STATE_NULL          = 0,
    M_STATE_STOP          = 1,
    M_STATE_TORCH_WALK    = 2,
    M_STATE_RUN           = 3,
    M_STATE_IGNITE_TORCH  = 11,
    M_STATE_TURN_LEFT     = 22,
    M_STATE_ATTACK_LOW    = 31,
    M_STATE_TURN_RIGHT    = 35,
    M_STATE_CROUCH        = 36,
    M_STATE_GRAB_TORCH    = 37,
    M_STATE_EXAMINE_SHORT = 38,
    M_STATE_EXAMINE_LONG  = 39,
    M_STATE_WALK          = 40,
    M_STATE_SHIFT_FRONT   = 41,
    M_STATE_SHIFT_BACK    = 42,
    M_STATE_TRIGGER_TRAP  = 43,
    // clang-format on
} M_STATE;

typedef enum {
    M_TORCH_NULL,
    M_TORCH_UNLIT,
    M_TORCH_LIT,
} M_TORCH_STATUS;

typedef struct {
    bool guides_lara;
    bool wraith_aware;
    M_TORCH_STATUS torch_status;
    int32_t swap_bits;
    int32_t waypoint;
} M_PRIV;

static const BITE m_Hit = {
    .pos = { .x = 0, .y = 20, .z = 200 },
    .mesh_num = 18,
};

static const BITE m_Light = {
    .pos = { .x = 30, .y = 80, .z = 50 },
    .mesh_num = 15,
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    SHOULD(JSON_READ(io, "torch_status", &p->torch_status));
    SHOULD(JSON_READ(io, "swap_bits", &p->swap_bits));
    SHOULD(JSON_READ(io, "waypoint", &p->waypoint));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "torch_status", p->torch_status);
    JSONW_WRITE(io, "swap_bits", p->swap_bits);
    JSONW_WRITE(io, "waypoint", p->waypoint);
}

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_STAND, 0);
    item->current_anim_state = M_STATE_STOP;
    item->goal_anim_state = M_STATE_STOP;

    M_PRIV *const p = item->priv;
    p->waypoint = M_WAYPOINT_UNSET;
    p->wraith_aware = false; // TODO: use Object_Get(O_WRAITH_1)->loaded
    if (p->wraith_aware) {
        p->torch_status = M_TORCH_LIT;
    } else {
        p->swap_bits = M_TORCH_BITS;
    }
}

static bool M_IsTargetable(const ITEM *const item)
{
    return false;
}

static void M_ControlTorch(const ITEM *const item)
{
    const int32_t rnd = Random_GetControl();
    const RGB_888 color = {
        .r = 255 - ((rnd >> 4) & 0x1F),
        .g = 192 - ((rnd >> 6) & 0x1F),
        .b = rnd & 0x3F,
    };
    XYZ_32 pos = m_Hit.pos;
    Collide_GetJointAbsPosition(item, &pos, m_Hit.mesh_num);
    Output_AddDynamicLightRGB(pos, 15, color);
    FX_Fire_Add(pos, 0, item->room_num, 0);
    Sparks_TriggerFireFlame((XYZ_32) { pos.x, pos.y - 40, pos.z }, -1, 7);
    Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &item->pos, SPM_NORMAL);

    if (Item_TestAnimEqual(item, M_ANIM_EXAMINE_SHORT)
        && Item_TestFrameRange(item, 33, 41)) {
        pos.x += (rnd & 0x3F) - 32;
        pos.y += ((rnd >> 3) & 0x3F) - 128;
        pos.z += ((rnd >> 6) & 0x3F) - 32;
        Sparks_TriggerFireFlame(pos, -1, 1);
    }
}

static AI_INFO M_GetAIInfo(const ITEM *const item)
{
    ITEM *const lara_item = Lara_GetItem();
    AI_INFO info = {};
    int32_t x = lara_item->pos.x - item->pos.x;
    int32_t z = lara_item->pos.z - item->pos.z;

    info.angle = Math_Atan(z, x) - item->rot.y;
    info.ahead = info.angle > -DEG_90 && info.angle < DEG_90;

    if (z > 32000 || z < -32000 || x > 32000 || x < -32000) {
        info.distance = INT32_MAX;
    } else {
        info.distance = SQUARE(x) + SQUARE(z);
    }

    x = ABS(x);
    z = ABS(z);

    if (x > z) {
        info.x_angle = Math_Atan(x + (z >> 1), item->pos.y - lara_item->pos.y);
    } else {
        info.x_angle = Math_Atan(z + (x >> 1), item->pos.y - lara_item->pos.y);
    }

    return info;
}

static ITEM *M_GetTarget(const ITEM *const item, const AI_INFO *const lara_info)
{
    const M_PRIV *const p = item->priv;
    if (p->wraith_aware
        || (item->current_anim_state > M_STATE_RUN
            && item->current_anim_state != M_STATE_ATTACK_LOW)) {
        return nullptr;
    }

    ITEM *target = nullptr;
    int32_t best_dist = INT32_MAX;
    const int32_t item_num = Item_GetIndex(item);
    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const baddie = LOT_GetBaddieSlot(i);
        if (baddie->item_num == NO_ITEM || baddie->item_num == item_num) {
            continue;
        }

        ITEM *const candidate = Item_Get(baddie->item_num);
        const XYZ_32 delta = {
            .x = ABS(candidate->pos.x - item->pos.x),
            .y = ABS(candidate->pos.y - item->pos.y),
            .z = ABS(candidate->pos.z - item->pos.z),
        };
        if (candidate->object_id == item->object_id || delta.y > WALL_L / 2) {
            continue;
        }

        const int32_t dist = (delta.x > 32000 || delta.z > 32000)
            ? INT32_MAX
            : (SQUARE(delta.x) + SQUARE(delta.z));
        if (dist < best_dist && dist < M_RUN_DIST
            && (delta.y < STEP_L || lara_info->distance < M_RUN_DIST
                || candidate->object_id == O_JACKAL)) {
            target = candidate;
            best_dist = dist;
        }
    }

    return target;
}

static void M_GrabTorch(const ITEM *const guide)
{
    const ROOM *const room = Room_Get(guide->room_num);
    int16_t next_item_num = room->item_num;
    while (next_item_num != NO_ITEM) {
        ITEM *const item = Item_Get(next_item_num);
        next_item_num = item->next_item;

        if (ROUND_TO_SECTOR(guide->pos.x) != ROUND_TO_SECTOR(item->pos.x)
            || ROUND_TO_SECTOR(guide->pos.z) != ROUND_TO_SECTOR(item->pos.z)) {
            continue;
        }

        if (ObjectFamily_Has(item->object_id, OBJ_FAMILY_GENERIC_ANIMATING)) {
            item->mesh_bits &= ~2;
            break;
        }
    }
}

static void M_IgniteTorch(const ITEM *const item)
{
    const int32_t rnd = Random_GetControl();
    XYZ_32 pos = m_Light.pos;
    Collide_GetJointAbsPosition(item, &pos, m_Light.mesh_num);

    M_PRIV *const p = item->priv;
    const int16_t frame = Item_GetRelativeFrame(item);

    if (frame == 32) {
        p->swap_bits |= M_LIGHTER_BITS;
    } else if (frame == 216) {
        p->swap_bits &= ~M_LIGHTER_BITS;
    } else if ((frame > 79 && frame < 84) || (frame > 159 && frame < 164)) {
        const RGB_888 color = {
            .r = rnd & 0x1F,
            .g = 96 - ((rnd >> 6) & 0x1F),
            .b = 128 - ((rnd >> 4) & 0x1F),
        };
        Output_AddDynamicLightRGB(pos, 10, color);
        Sparks_TriggerFlareSparks(pos, (XYZ_32) { -1, -1, 0 }, true);
    } else if ((frame > 83 && frame < 94) || (frame > 163 && frame < 181)) {
        const RGB_888 color = {
            .r = 192 - ((rnd >> 4) & 0x1F),
            .g = 128 - ((rnd >> 6) & 0x1F),
            .b = rnd & 0x1F,
        };
        const XYZ_32 flame_pos = {
            .x = (rnd & 0x3F) + pos.x - 64,
            .y = ((rnd >> 5) & 0x3F) + pos.y - 96,
            .z = ((rnd >> 10) & 0x3F) + pos.z - 64,
        };
        Output_AddDynamicLightRGB(
            (XYZ_32) { pos.x - 32, pos.y - 64, pos.z - 32 }, 10, color);
        Sparks_TriggerFireFlame(flame_pos, -1, 7);

        if (frame > 163) {
            p->torch_status = M_TORCH_LIT;
        }
    }
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (p->waypoint == M_WAYPOINT_UNSET) {
        p->waypoint = item->ai_ocb;
    }

    if (p->torch_status == M_TORCH_LIT) {
        M_ControlTorch(item);
    }

    item->ai_bits = AI_FOLLOW;
    CREATURE *const creature = (CREATURE *)item->creature_data;
    Creature_GetAITarget(creature);
    Creature_FindAITargetObject(creature, O_AI_FOLLOW, p->waypoint);

    const AI_INFO lara_info = M_GetAIInfo(item);
    ITEM *const target = M_GetTarget(item, &lara_info);
    ITEM *enemy = creature->enemy;
    if (target != nullptr) {
        creature->enemy = target;
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);
    Creature_Mood(item, &info, true);
    const int16_t angle = Creature_Turn(item, creature->maximum_turn);

    if (target != nullptr) {
        creature->enemy = enemy;
        enemy = target;
    }

    const ITEM *const lara_item = Lara_GetItem();
    const int16_t frame_num = Item_GetRelativeFrame(item);
    int16_t tilt = 0;
    int16_t head = 0;
    int16_t torso_x = 0;
    int16_t torso_y = 0;

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        creature->flags = 0;
        creature->maximum_turn = 0;
        head = info.angle >> 1;

        if (lara_info.ahead) {
            torso_x = lara_info.x_angle >> 1;
            torso_y = lara_info.angle >> 1;
            head = lara_info.angle >> 1;
        } else if (info.ahead) {
            torso_x = info.x_angle >> 1;
            torso_y = info.angle >> 1;
            head = info.angle >> 1;
        }

        if (p->wraith_aware) {
            if (p->waypoint == 5 || p->waypoint == 6) {
                if (p->waypoint == 5) {
                    item->goal_anim_state = M_STATE_TORCH_WALK;
                }
                break;
            }
        }

        if (item->required_anim_state != M_STATE_NULL) {
            item->goal_anim_state = item->required_anim_state;
        } else if (
            Waypoint_Get() < p->waypoint && p->torch_status == M_TORCH_LIT) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (!creature->reached_goal || target != nullptr) {
            if (p->swap_bits == M_TORCH_BITS) {
                item->goal_anim_state = M_STATE_WALK;
            } else if (target != nullptr && info.distance < M_WALK_DIST) {
                if (info.bite) {
                    item->goal_anim_state = M_STATE_ATTACK_LOW;
                }
            } else if (enemy != lara_item || info.distance > M_RUN_DIST) {
                item->goal_anim_state = M_STATE_TORCH_WALK;
            }
        } else if (Creature_GetAIObjectFlags(enemy) == 0) {
            creature->reached_goal = false;
            creature->enemy = nullptr;
            item->ai_bits = AI_FOLLOW;
            p->waypoint++;
        } else if (info.distance > M_SHIFT_DIST) {
            creature->maximum_turn = 0;
            if (info.ahead) {
                item->required_anim_state = M_STATE_SHIFT_FRONT;
            } else {
                item->required_anim_state = M_STATE_SHIFT_BACK;
            }
        } else {
            switch (Creature_GetAIObjectFlags(enemy)) {
            case 2:
                item->goal_anim_state = M_STATE_EXAMINE_SHORT;
                item->required_anim_state = M_STATE_EXAMINE_SHORT;
                break;

            case 4:
                if (lara_info.distance < M_RUN_DIST) {
                    item->goal_anim_state = M_STATE_CROUCH;
                    item->required_anim_state = M_STATE_TRIGGER_TRAP;
                }
                break;

            case 16:
                if (lara_info.distance < M_RUN_DIST) {
                    item->goal_anim_state = M_STATE_CROUCH;
                    item->required_anim_state = M_STATE_CROUCH;
                }
                break;

            case 32:
                item->goal_anim_state = M_STATE_GRAB_TORCH;
                item->required_anim_state = M_STATE_GRAB_TORCH;
                break;

            case 40:
                if (lara_info.distance < M_RUN_DIST) {
                    item->goal_anim_state = M_STATE_EXAMINE_LONG;
                    item->required_anim_state = M_STATE_EXAMINE_LONG;
                }
                break;

            case 62:
                Item_RemoveSimulated(item_num);
                Item_SetVisible(item, false);
                LOT_DisableBaddieAI(item_num);
                break;
            }
        }
        break;

    case M_STATE_TORCH_WALK:
        creature->maximum_turn = M_WALK_TURN;
        if (lara_info.ahead) {
            head = lara_info.angle;
        } else if (info.ahead) {
            head = info.angle;
        }

        if (p->wraith_aware && p->waypoint == 5) {
            p->waypoint = 6;
            item->goal_anim_state = M_STATE_STOP;
        } else if (p->torch_status == M_TORCH_UNLIT) {
            item->goal_anim_state = M_STATE_STOP;
            item->required_anim_state = M_STATE_IGNITE_TORCH;
        } else if (creature->reached_goal) {
            if (Creature_GetAIObjectFlags(enemy) == 0) {
                creature->reached_goal = false;
                creature->enemy = nullptr;
                item->ai_bits = AI_FOLLOW;
                p->waypoint++;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else if (Waypoint_Get() < p->waypoint) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            target == nullptr
            || (info.distance >= 0x200000
                && ((p->swap_bits & M_TORCH_BITS) != 0
                    || info.distance >= M_AWARE_DIST))) {
            if (enemy == lara_item) {
                if (info.distance < M_RUN_DIST) {
                    item->goal_anim_state = M_STATE_STOP;
                } else if (info.distance > M_WALK_DIST) {
                    item->goal_anim_state = M_STATE_RUN;
                }
            } else if (
                Waypoint_Get() > p->waypoint
                && lara_info.distance > M_RUN_DIST) {
                item->goal_anim_state = M_STATE_RUN;
            }
        } else {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_RUN:
        if (info.ahead) {
            head = info.angle;
        }

        creature->maximum_turn = M_RUN_TURN;
        tilt = angle / 2;

        if (info.distance < M_RUN_DIST || Waypoint_Get() < p->waypoint) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->reached_goal) {
            if (Creature_GetAIObjectFlags(enemy) == 0) {
                creature->reached_goal = false;
                creature->enemy = nullptr;
                item->ai_bits = AI_FOLLOW;
                p->waypoint++;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else if (
            target != nullptr && (p->swap_bits & M_TORCH_BITS) == 0
            && info.distance < M_AWARE_DIST) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_IGNITE_TORCH:
        M_IgniteTorch(item);
        break;

    case M_STATE_TURN_LEFT:
        creature->maximum_turn = 0;
        if (lara_info.angle < -256) {
            item->rot.y -= 399;
        }
        break;

    case M_STATE_TURN_RIGHT:
        creature->maximum_turn = 0;
        if (lara_info.angle > 256) {
            item->rot.y += 399;
        }
        break;

    case M_STATE_ATTACK_LOW:
        creature->maximum_turn = 0;
        if (info.ahead) {
            torso_x = info.x_angle >> 1;
            torso_y = info.angle >> 1;
            head = info.angle >> 1;
        }

        if (ABS(info.angle) < M_WALK_TURN) {
            item->rot.y += info.angle;
        } else if (info.angle < 0) {
            item->rot.y -= M_WALK_TURN;
        } else {
            item->rot.y += M_WALK_TURN;
        }

        if (creature->flags != 0 || enemy == nullptr) {
            break;
        }

        if (Item_TestFrameRange(item, 16, 25)
            && XYZ_32_IsNearby(enemy->pos, item->pos, WALL_L / 2)) {
            Item_TakeDamage(enemy, 20, IDF_NONE, item);
            if (enemy->hit_points <= 0) {
                item->ai_bits = AI_FOLLOW;
            }

            creature->flags = 1;
            Creature_EffectEx(item, &m_Hit, 8, -1, Spawn_Blood);
        }
        break;

    case M_STATE_CROUCH:
    case M_STATE_TRIGGER_TRAP:
        if (enemy != nullptr) {
            const int16_t dy = enemy->rot.y - item->rot.y;
            if (dy > M_ADJUST_TURN) {
                item->rot.y += M_ADJUST_TURN;
            } else if (dy < -M_ADJUST_TURN) {
                item->rot.y -= M_ADJUST_TURN;
            }
        }

        if (item->required_anim_state == M_STATE_TRIGGER_TRAP) {
            item->goal_anim_state = M_STATE_TRIGGER_TRAP;
        } else if (
            !Item_TestAnimEqual(item, M_ANIM_BEGIN_CROUCH)
            && Item_TestFrameEqual(item, -20)) {
            item->goal_anim_state = M_STATE_STOP;
            Room_TestTriggers(item);
            creature->reached_goal = false;
            creature->enemy = nullptr;
            item->ai_bits = AI_FOLLOW;
            p->waypoint++;
        }
        break;

    case M_STATE_GRAB_TORCH:
        p->torch_status = M_TORCH_UNLIT;
        if (frame_num == 0) {
            item->pos = enemy->pos;
            item->rot = enemy->rot;
            creature->reached_goal = false;
            creature->enemy = nullptr;
            item->ai_bits = AI_FOLLOW;
            p->waypoint++;
        } else if (frame_num == 35) {
            p->swap_bits &= ~M_TORCH_BITS;
            M_GrabTorch(item);
        }
        break;

    case M_STATE_EXAMINE_SHORT:
        if (frame_num == 0) {
            item->pos = enemy->pos;
        } else if (frame_num == 42) {
            Room_TestTriggers(item);
            item->rot.y = enemy->rot.y;
            creature->reached_goal = false;
            creature->enemy = nullptr;
            item->ai_bits = AI_FOLLOW;
            p->waypoint++;
        } else if (frame_num < 42) {
            const int16_t dy = enemy->rot.y - item->rot.y;
            if (dy > M_ADJUST_TURN) {
                item->rot.y += M_ADJUST_TURN;
            } else if (dy < -M_ADJUST_TURN) {
                item->rot.y -= M_ADJUST_TURN;
            }
        }
        break;

    case M_STATE_EXAMINE_LONG:
        if (frame_num < 20) {
            const int16_t dy = enemy->rot.y - item->rot.y;
            if (dy > M_ADJUST_TURN) {
                item->rot.y += M_ADJUST_TURN;
            } else if (dy < -M_ADJUST_TURN) {
                item->rot.y -= M_ADJUST_TURN;
            }
        } else if (frame_num == 20) {
            item->goal_anim_state = M_STATE_STOP;
            Room_TestTriggers(item);
            creature->reached_goal = false;
            creature->enemy = nullptr;
            item->ai_bits = AI_FOLLOW;
            p->waypoint++;
        } else if (frame_num == 70 && p->waypoint == 14) {
            // XXX: OG hard-coded a check for room number 70; the waypoint index
            // is used instead.
            item->required_anim_state = M_STATE_RUN;
            p->swap_bits |= M_SCREAM_BITS;
            Sound_Effect(SFX_GUIDE_SCARE, &item->pos, SPM_NORMAL);
        }
        break;

    case M_STATE_WALK:
        creature->maximum_turn = M_WALK_TURN;
        if (lara_info.ahead) {
            head = lara_info.angle;
        } else if (info.ahead) {
            head = info.angle;
        }

        if (creature->reached_goal) {
            if (Creature_GetAIObjectFlags(enemy) == 0) {
                creature->reached_goal = false;
                creature->enemy = nullptr;
                item->ai_bits = AI_FOLLOW;
                p->waypoint++;
                break;
            }

            if (Creature_GetAIObjectFlags(enemy) == 42) {
                Room_TestTriggers(enemy);
                creature->reached_goal = false;
                creature->enemy = nullptr;
                item->ai_bits = AI_FOLLOW;
                p->waypoint++;
            } else if (p->guides_lara) {
                item->goal_anim_state = M_STATE_STOP;
            } else {
                Item_Destroy(item_num);
                LOT_DisableBaddieAI(item_num);
                Item_SetVisible(item, false);
            }
        }
        break;

    case M_STATE_SHIFT_FRONT:
    case M_STATE_SHIFT_BACK:
        creature->maximum_turn = 0;
        Creature_MoveTo(
            item, enemy, M_SHIFT_SPEED, enemy->rot.y - item->rot.y,
            M_SHIFT_TURN);
        break;
    }

    Creature_Tilt(item, tilt);
    Creature_Joint(item, 0, torso_y);
    Creature_Joint(item, 1, torso_x);
    Creature_Joint(item, 2, head);
    Creature_Joint(item, 3, torso_x);
    Creature_Animate(item_num, angle, 0);
}

static bool M_Draw(const ITEM *const item)
{
    const OBJECT *const swap = Object_Get(O_MESH_SWAP_2);
    if (!swap->loaded
        || swap->mesh_count != Object_Get(item->object_id)->mesh_count) {
        return Object_DrawAnimatingItem(item);
    }

    const M_PRIV *const p = item->priv;
    return Object_DrawAnimatingItemWithSwap(
        item, swap, item->mesh_bits & ~p->swap_bits);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = Creature_Collision;
    obj->is_targetable_func = M_IsTargetable;
    obj->control_func = M_Control;
    obj->draw_func = M_Draw;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;
    obj->radius = M_RADIUS;
    obj->intelligent = true;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_anim = true;
    obj->save_flags = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    Object_GetBone(obj, 6)->rot.x = true;
    Object_GetBone(obj, 6)->rot.y = true;
    Object_GetBone(obj, 20)->rot.x = true;
    Object_GetBone(obj, 20)->rot.y = true;

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, guides_lara, true,
            "Whether he guides Lara or navigates the level on his own."));
}

REGISTER_OBJECT(O_GUIDE, M_Setup)
