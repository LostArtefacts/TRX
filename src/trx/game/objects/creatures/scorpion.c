// Scorpion enemy implementation for TRX, adapted from TOMB4 scorpion
// Supports both large scorpion (O_SCORPION) and small scorpion (O_SMALL_SCORPION)

#include <stdbool.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/anims.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>
#include <trx/game/sound.h>
#include <trx/core/utils.h>
#include <trx/game/const.h>

// Scorpion states (matching TOMB4 animation states where possible)
typedef enum {
    SC_STATE_IDLE = 1,          // idle
    SC_STATE_WALK = 2,          // walking
    SC_STATE_ATTACK_PREP = 3,   // preparing to attack (turning)
    SC_STATE_PINCER_ATTACK = 4, // pincer attack
    SC_STATE_STINGER_ATTACK = 5, // stinger attack
    SC_STATE_DEATH_PREP = 6,    // death preparation
    SC_STATE_DEATH = 7,         // death
    SC_STATE_TARGET_LOST = 8    // target lost, wandering
} SCORPION_STATE;

// Scorpion bite definitions (stinger and pincer)
// Mesh numbers are placeholders; adjust to match the actual model
static const BITE m_ScorpionStingerBite = {
    .pos = { .x = 0, .y = 0, .z = 70 }, // approximate length
    .mesh_num = 1
};

static const BITE m_ScorpionPincerBite = {
    .pos = { .x = 0, .y = 0, .z = 50 },
    .mesh_num = 2
};

// Default values (can be overridden via object properties)
#define SCORPION_HIT_POINTS        200
#define SCORPION_DAMAGE_PINCER     120
#define SCORPION_DAMAGE_STINGER    120
#define SCORPION_DAMAGE_ENEMY      15   // damage to other enemies
#define SCORPION_TURN_SPEED        (DEG_1 * 8)   // base turn speed
#define SCORPION_ATTACK_RANGE      SQUARE(WALL_L * 2)   // aggro range
#define SCORPION_TARGET_LOST_RANGE SQUARE(WALL_L * 8)   // give up chase

// Small scorpion values
#define SCORPION_SMALL_HIT_POINTS  80
#define SCORPION_SMALL_DAMAGE      20
#define SCORPION_SMALL_TURN_SPEED  (DEG_1 * 4)
#define SCORPION_SMALL_ATTACK_RANGE SQUARE(WALL_L)

// Helper to check if this is the small scorpion variant
static bool IsSmallScorpion(const ITEM *const item)
{
    return (item->object_id == O_SMALL_SCORPION);
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const scorpion = item->creature_data;
    const bool is_small = IsSmallScorpion(item);
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    int16_t head = 0;
    int16_t turn_angle = 0;

    // If the scorpion is dead, we set up the death state and skip AI
    if (item->hit_points <= 0) {
        // If we are not already dying or dead, set to death prep
        if (item->current_anim_state != SC_STATE_DEATH_PREP &&
            item->current_anim_state != SC_STATE_DEATH) {
            item->current_anim_state = SC_STATE_DEATH_PREP;
            // Note: We rely on the state machine below to handle the death animation transition
        }
        // Set head and turn_angle to 0 for dead
        head = 0;
        turn_angle = 0;
    } else {
        // Alive: update AI and state machine
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, true); // violent = true for aggressive creatures

        // Determine turn speed based on size
        const int16_t turn_speed = is_small ? SCORPION_SMALL_TURN_SPEED : SCORPION_TURN_SPEED;

        // State machine
        switch (item->current_anim_state) {
            case SC_STATE_IDLE:
                scorpion->flags = 0; // reset attack flag
                turn_angle = Creature_Turn(item, turn_speed);
                if (info.distance > SCORPION_TARGET_LOST_RANGE) {
                    // Target too far, wander
                    item->goal_anim_state = SC_STATE_TARGET_LOST;
                } else if (info.bite) {
                    // Within biting range, choose attack type
                    if (Random_GetControl() & 1) {
                        item->goal_anim_state = SC_STATE_STINGER_ATTACK;
                    } else {
                        item->goal_anim_state = SC_STATE_PINCER_ATTACK;
                    }
                } else if (!info.ahead) {
                    // Target not in front, walk to intercept
                    item->goal_anim_state = SC_STATE_WALK;
                } else {
                    // Face target but stay idle
                    item->goal_anim_state = SC_STATE_IDLE;
                }
                break;

            case SC_STATE_WALK:
                turn_angle = Creature_Turn(item, turn_speed);
                if (info.distance > SCORPION_TARGET_LOST_RANGE) {
                    item->goal_anim_state = SC_STATE_TARGET_LOST;
                } else if (info.distance < SCORPION_ATTACK_RANGE) {
                    // Close enough, prepare to attack
                    item->goal_anim_state = SC_STATE_IDLE; // will re-evaluate next frame
                } else {
                    item->goal_anim_state = SC_STATE_WALK;
                }
                break;

            case SC_STATE_TARGET_LOST:
                turn_angle = Creature_Turn(item, turn_speed);
                // Wander when target lost; return to idle if target reappears
                if (info.distance < SCORPION_TARGET_LOST_RANGE) {
                    item->goal_anim_state = SC_STATE_IDLE;
                } else {
                    item->goal_anim_state = SC_STATE_TARGET_LOST;
                }
                break;

            case SC_STATE_PINCER_ATTACK:
            case SC_STATE_STINGER_ATTACK:
                // During attack, keep turning towards target (faster turn)
                turn_angle = Creature_Turn(item, turn_speed * 2);

                // Check if we hit something with our bite
                if (!scorpion->flags && item->touch_bits != 0) {
                    ITEM *enemy = scorpion->enemy;
                    if (enemy && enemy != Lara_GetItem()) {
                        // Hit another enemy
                        enemy->hit_points -= SCORPION_DAMAGE_ENEMY;
                        if (enemy->hit_points <= 0) {
                            enemy->hit_status = 1;
                        }
                        scorpion->flags = 1;
                        Creature_Effect(item,
                            (item->current_anim_state == SC_STATE_PINCER_ATTACK) ?
                                &m_ScorpionPincerBite : &m_ScorpionStingerBite,
                            Spawn_Blood);
                    } else if (enemy == Lara_GetItem()) {
                        // Hit Lara
                        const int32_t damage = is_small ? SCORPION_SMALL_DAMAGE :
                            (item->current_anim_state == SC_STATE_PINCER_ATTACK ?
                                SCORPION_DAMAGE_PINCER : SCORPION_DAMAGE_STINGER);
                        Lara_TakeDamage(damage, true);
                        scorpion->flags = 1;
                        Creature_Effect(item,
                            (item->current_anim_state == SC_STATE_PINCER_ATTACK) ?
                                &m_ScorpionPincerBite : &m_ScorpionStingerBite,
                            Spawn_Blood);

                        // Apply poison for stinger attack (like TOMB4)
                        if (item->current_anim_state == SC_STATE_STINGER_ATTACK) {
                            lara_info->poison.target += 2048;
                        }
                    }
                }

                // Return to idle after attack animation finishes
                // Check if current animation has reached its end frame
                {
                    const OBJECT *obj = Object_Get(item->object_id);
                    const ANIM *anim = Object_GetAnim(obj, item->anim_num);
                    // Safety check for null anim
                    if (anim && item->frame_num == anim->frame_end) {
                        item->goal_anim_state = SC_STATE_IDLE;
                    }
                    // Additional safety: if we've been in attack state too long, return to idle
                    else if (!anim || item->frame_num < anim->frame_base) {
                        item->goal_anim_state = SC_STATE_IDLE;
                    }
                }
                break;

            case SC_STATE_DEATH_PREP:
                // Play death animation; transition to DEATH when finished
                turn_angle = Creature_Turn(item, 0); // no turning during death prep
                {
                    const OBJECT *obj = Object_Get(item->object_id);
                    const ANIM *anim = Object_GetAnim(obj, item->anim_num);
                    // Safety check for null anim
                    if (anim && item->frame_num == anim->frame_end) {
                        item->goal_anim_state = SC_STATE_DEATH;
                        scorpion->maximum_turn = 0; // stop turning
                    }
                    // Additional safety: if we've been in death prep too long, force death state
                    else if (!anim || item->frame_num < anim->frame_base) {
                        item->goal_anim_state = SC_STATE_DEATH;
                        scorpion->maximum_turn = 0; // stop turning
                    }
                }
                break;

            case SC_STATE_DEATH:
                // Stay dead, no turning
                turn_angle = 0;
                break;

            default:
                // Fallback to idle
                turn_angle = Creature_Turn(item, turn_speed);
                item->goal_anim_state = SC_STATE_IDLE;
                break;
        }
    }

    Creature_Tilt(item, head);
    Creature_Head(item, head);
    Creature_Animate(item_num, turn_angle, 0);
}

static void M_SetupLarge(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = Creature_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    // Set physical properties
    obj->radius = WALL_L / 4;         // placeholder
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->lot_setup = LOT_Setup(LOT_SETUP_QUADRUPED); // scorpions have many legs
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    // Define object properties that can be overridden in levels
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT("max_hit_points", SCORPION_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT("pincer_damage", SCORPION_DAMAGE_PINCER, "Damage dealt by pincer attack."),
        OBJECT_PROPERTY_INT("stinger_damage", SCORPION_DAMAGE_STINGER, "Damage dealt by stinger attack."));
}

static void M_SetupSmall(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = Creature_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    // Set physical properties (can be different for small scorpion)
    obj->radius = WALL_L / 6;         // smaller
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->lot_setup = LOT_Setup(LOT_SETUP_QUADRUPED);
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    // Define object properties that can be overridden in levels
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT("max_hit_points", SCORPION_SMALL_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT("pincer_damage", SCORPION_SMALL_DAMAGE, "Damage dealt by pincer attack."),
        OBJECT_PROPERTY_INT("stinger_damage", SCORPION_SMALL_DAMAGE, "Damage dealt by stinger attack."));
}

REGISTER_OBJECT(O_SCORPION, M_SetupLarge)
REGISTER_OBJECT(O_SMALL_SCORPION, M_SetupSmall)