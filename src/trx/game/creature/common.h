#pragma once

#include <trx/game/collision.h>
#include <trx/game/creature/types.h>

#define AI_OBJECT_FLAGS_SPENT 255

void Creature_Initialise(int16_t item_num);
bool Creature_Activate(int16_t item_num);
void Creature_AIInfo(ITEM *item, AI_INFO *info);
bool Creature_EnsureHabitat(
    int16_t item_num, int32_t *wh, const HYBRID_INFO *info);
void Creature_Mood(const ITEM *item, const AI_INFO *info, bool violent);
void Creature_UpdateMood(const ITEM *item, const AI_INFO *info, bool violent);
void Creature_ApplyMood(const ITEM *item, const AI_INFO *info, bool violent);

int16_t Creature_Turn(ITEM *item, int16_t max_turn);

// Turns the item's own facing towards an angle, by no more than max_turn.
// Creature_Turn steers a creature that is walking somewhere; this one turns
// on the spot.
void Creature_TurnTo(ITEM *item, int16_t angle, int16_t max_turn);

// Eases the item towards another's position and facing, by velocity and by no
// more than max_turn. True once it has arrived.
bool Creature_MoveTo(
    ITEM *item, const ITEM *target, int32_t velocity, int16_t angle,
    int16_t max_turn);
void Creature_Tilt(ITEM *item, int16_t angle);
void Creature_Head(ITEM *item, int16_t required);
void Creature_Neck(ITEM *item, int16_t required);
void Creature_Joint(ITEM *item, int16_t joint, int16_t required);

void Creature_Float(int16_t item_num);
void Creature_Underwater(ITEM *item, int32_t depth);

bool Creature_CanSeeEnemy(const ITEM *item, const AI_INFO *info);
bool Creature_CanTargetEnemy(const ITEM *item, const AI_INFO *info);
void Creature_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
bool Creature_Animate(int16_t item_num, int16_t angle, int16_t tilt);

void Creature_SpecialKill(
    ITEM *item, int32_t kill_anim, int32_t kill_state, int32_t lara_kill_state);
void Creature_TestBoxDamage(int16_t item_num);
void Creature_Die(int16_t item_num, bool explode);
int32_t Creature_Vault(
    int16_t item_num, int16_t angle, int32_t vault, int32_t shift);

void Creature_Reset(void);
bool Creature_AreAlliesHostile(void);
void Creature_SetAlliesHostile(bool enable);
void Creature_Hurt(ITEM *item, int32_t damage);
bool Creature_IsHostile(const ITEM *item);
bool Creature_IsAlly(const ITEM *item);
bool Creature_IsAllyTargetingEnemy(const ITEM *item);
void Creature_AddAlly(OBJECT_ID obj_id);
void Creature_AddAllyTargetingEnemy(OBJECT_ID obj_id);

int16_t Creature_Effect(
    const ITEM *item, const BITE *bite,
    int16_t (*spawn)(
        int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
        int16_t room_num));

bool Creature_Shoot(
    ITEM *item, const AI_INFO *info, const CREATURE_GUN *gun,
    int16_t extra_rotation, int32_t damage);

int16_t Creature_AIGuard(CREATURE *creature);
void Creature_GetAITarget(CREATURE *creature);

// The AI object a TR4 level marks with this OCB, or nullptr where it places
// none. Where a level places several with the same OCB, the last one answers.
ITEM *Creature_FindAIObjectByOCB(int32_t ocb);

// The AI object a creature walks to: one of the given object, carrying the
// given OCB, still placed, and in a ground zone the creature can reach. Points
// the creature's enemy at it and hands it back, or leaves the enemy alone and
// answers nullptr where the level places none.
//
// TR1-3 match an AI object by the tag in its rotation; TR4 matches by the OCB.
ITEM *Creature_FindAITargetObject(
    CREATURE *creature, OBJECT_ID object_id, int32_t ocb);

// The flags word the level gave an AI object, which TR4 uses as a mode rather
// than as the bitfield an item's own flags are. Anything that is not an AI
// object answers 0, because a creature's enemy is as often Lara or a live
// item, and their flags mean something else entirely. A spent one answers 255,
// the value the original writes over its flags with.
int32_t Creature_GetAIObjectFlags(const ITEM *item);

// Whether an AI object has been used up, so the search passes over it. The
// original says so by writing over the object's own flags word, which is level
// data rather than somewhere to keep a latch.
bool Creature_IsAIObjectSpent(const ITEM *item);
void Creature_SetAIObjectSpent(const ITEM *item);
void Creature_ResetAIObjectsSpent(void);
