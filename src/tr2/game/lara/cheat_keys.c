#include "game/lara/cheat_keys.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/sound.h"
#include "global/vars.h"

typedef enum {
    CHEAT_INITIAL = 0,
    CHEAT_STEP_FORWARD = 1,
    CHEAT_STEP_FORWARD_STOP = 2,
    CHEAT_STEP_BACK = 3,
    CHEAT_STEP_BACK_STOP = 4,
    CHEAT_TURN_LEFT = 5,
    CHEAT_TURN_RIGHT = 6,
    CHEAT_TURN_STOP = 7,
    CHEAT_TURN_JUMP = 8,
} CHEAT_STATE;

static int32_t m_CheatState = 0;
static bool m_CheatFlare = false;
static int16_t m_CheatAngle = 0;
static int32_t m_CheatTurn = 0;

static void M_CompleteLevel(void);
static void M_GiveItems(void);
static void M_ExplodeLara(void);

static void M_CompleteLevel(void)
{
    Game_SetIsLevelComplete(true);
}

static void M_GiveItems(void)
{
    Inv_AddItem(O_HARPOON_ITEM);
    Inv_AddItem(O_GRENADE_ITEM);
    Inv_AddItem(O_M16_ITEM);
    Inv_AddItem(O_SHOTGUN_ITEM);
    Inv_AddItem(O_MAGNUM_ITEM);
    Inv_AddItem(O_UZI_ITEM);
    g_Lara.shotgun_ammo.ammo = 500;
    g_Lara.magnum_ammo.ammo = 500;
    g_Lara.uzi_ammo.ammo = 5000;
    g_Lara.grenade_ammo.ammo = 5000;
    g_Lara.m16_ammo.ammo = 5000;
    g_Lara.harpoon_ammo.ammo = 5000;
    for (int32_t i = 0; i < 50; i++) {
        Inv_AddItem(O_SMALL_MEDIPACK_ITEM);
        Inv_AddItem(O_LARGE_MEDIPACK_ITEM);
        Inv_AddItem(O_FLARE_ITEM);
    }
    Sound_Effect(SFX_LARA_HOLSTER, 0, SPM_ALWAYS);
}

static void M_ExplodeLara(void)
{
    Item_Explode(g_Lara.item_num, -1, 1);
    g_LaraItem->hit_points = 0;
    g_LaraItem->flags |= IF_ONE_SHOT;
}

void Lara_Cheat_CheckKeys(void)
{
    if (Game_IsInGym()) {
        return;
    }

    const LARA_STATE ls = g_LaraItem->current_anim_state;
    switch (m_CheatState) {
    case CHEAT_INITIAL:
        m_CheatState = ls == LS_WALK ? CHEAT_STEP_FORWARD : CHEAT_INITIAL;
        break;

    case CHEAT_STEP_FORWARD:
        m_CheatFlare = g_Lara.gun_type == LGT_FLARE;
        if (ls != LS_WALK) {
            m_CheatState =
                ls == LS_STOP ? CHEAT_STEP_FORWARD_STOP : CHEAT_INITIAL;
        }
        break;

    case CHEAT_STEP_FORWARD_STOP:
        if (ls != LS_STOP) {
            m_CheatState = ls == LS_BACK ? CHEAT_STEP_BACK : CHEAT_INITIAL;
        }
        break;

    case CHEAT_STEP_BACK:
        if (ls != LS_BACK) {
            m_CheatState = ls == LS_STOP ? CHEAT_STEP_BACK_STOP : CHEAT_INITIAL;
        }
        break;

    case CHEAT_STEP_BACK_STOP:
        if (ls != LS_STOP) {
            m_CheatTurn = 0;
            m_CheatAngle = g_LaraItem->rot.y;
            if (ls == LS_TURN_LEFT) {
                m_CheatState = CHEAT_TURN_LEFT;
            } else if (ls == LS_TURN_RIGHT) {
                m_CheatState = CHEAT_TURN_RIGHT;
            } else {
                m_CheatState = CHEAT_INITIAL;
            }
        }
        break;

    case CHEAT_TURN_LEFT:
        if (ls != LS_TURN_LEFT && ls != LS_FAST_TURN) {
            m_CheatState =
                m_CheatTurn < -94208 ? CHEAT_TURN_STOP : CHEAT_INITIAL;
        } else {
            m_CheatTurn += (int16_t)(g_LaraItem->rot.y - m_CheatAngle);
            m_CheatAngle = g_LaraItem->rot.y;
        }
        break;

    case CHEAT_TURN_RIGHT:
        if (ls != LS_TURN_RIGHT && ls != LS_FAST_TURN) {
            m_CheatState =
                m_CheatTurn > 94208 ? CHEAT_TURN_STOP : CHEAT_INITIAL;
        } else {
            m_CheatTurn += (int16_t)(g_LaraItem->rot.y - m_CheatAngle);
            m_CheatAngle = g_LaraItem->rot.y;
        }
        break;

    case CHEAT_TURN_STOP:
        if (ls != LS_STOP) {
            m_CheatState = ls == LS_COMPRESS ? CHEAT_TURN_JUMP : CHEAT_INITIAL;
        }
        break;

    case CHEAT_TURN_JUMP:
        if (g_LaraItem->fall_speed > 0) {
            if (m_CheatFlare) {
                m_CheatFlare = g_Lara.gun_type == LGT_FLARE;
            }

            if (ls == LS_JUMP_FORWARD && m_CheatFlare) {
                M_CompleteLevel();
            } else if (ls == LS_JUMP_BACK && m_CheatFlare) {
                M_GiveItems();
            } else if (ls == LS_JUMP_FORWARD || ls == LS_JUMP_BACK) {
                M_ExplodeLara();
            }
            m_CheatState = CHEAT_INITIAL;
        }
        break;

    default:
        m_CheatState = CHEAT_INITIAL;
        break;
    }
}
