#include <trx/game/lara/cheat_keys.h>

#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/sound.h>
#include <trx/version.h>

#define M_MIN_TURN 94208

typedef enum {
    // clang-format off
    CHEAT_INITIAL           = 0,
    CHEAT_STEP_FORWARD      = 1,
    CHEAT_STEP_FORWARD_STOP = 2,
    CHEAT_STEP_BACK         = 3,
    CHEAT_STEP_BACK_STOP    = 4,
    CHEAT_TURN_LEFT         = 5,
    CHEAT_TURN_RIGHT        = 6,
    CHEAT_TURN_STOP         = 7,
    CHEAT_TURN_JUMP         = 8,
    // clang-format on
} M_CHEAT_STATE;

static int32_t m_CheatState = CHEAT_INITIAL;
static LARA_GUN_TYPE m_InitialGunType = LGT_UNARMED;
static int16_t m_CheatAngle = 0;
static int32_t m_CheatTurn = 0;

static void M_CompleteLevel(void)
{
    Game_SetIsLevelComplete(true);
}

static void M_GiveItems(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (Lara_Cheat_GiveGun(LGT_SHOTGUN, false)) {
        lara_info->shotgun_ammo.ammo = 500;
    }
    if (Lara_Cheat_GiveGun(LGT_MAGNUMS, false)) {
        lara_info->magnum_ammo.ammo = 500;
    }
    if (Lara_Cheat_GiveGun(LGT_AUTOS, false)) {
        lara_info->autos_ammo.ammo = 500;
    }
    if (Lara_Cheat_GiveGun(LGT_DESERT_EAGLE, false)) {
        lara_info->desert_eagle_ammo.ammo = 500;
    }
    if (Lara_Cheat_GiveGun(LGT_UZIS, false)) {
        lara_info->uzi_ammo.ammo = 5000;
    }
    if (Lara_Cheat_GiveGun(LGT_HARPOON, false)) {
        lara_info->harpoon_ammo.ammo = 5000;
    }
    if (Lara_Cheat_GiveGun(LGT_GRENADE, false)) {
        lara_info->grenade_ammo.ammo = 5000;
    }
    if (Lara_Cheat_GiveGun(LGT_M16, false)) {
        lara_info->m16_ammo.ammo = 5000;
    }
    Inv_AddItemNTimes(O_SMALL_MEDIPACK_ITEM, 50);
    Inv_AddItemNTimes(O_LARGE_MEDIPACK_ITEM, 50);
    if (g_Weapons[LGT_FLARE].is_available) {
        Inv_AddItemNTimes(O_FLARE_ITEM, 50);
    }
    Sound_Effect(SFX_LARA_HOLSTER, nullptr, SPM_ALWAYS);
}

static void M_ExplodeLara(void)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    ITEM *const lara_item = Lara_GetItem();
    Item_Explode(lara_info->item_num, -1, 1);
    Sound_Effect(SFX_EXPLOSION_1, &lara_item->pos, SPM_NORMAL);
    lara_item->hit_points = 0;
    lara_item->flags |= IF_ONE_SHOT;
}

static bool M_ProcessOutcome(const ITEM *const lara_item)
{
    if (lara_item->fall_speed <= 0) {
        return false;
    }

    const LARA_STATE state = lara_item->current_anim_state;
    bool gun_status_check;
    bool explode_status_check;
    if (g_TRVersion == 1) {
        gun_status_check = true;
        explode_status_check = state == LS(LS_SWAN_DIVE);
    } else {
        const LARA_INFO *const lara_info = Lara_GetLaraInfo();
        gun_status_check = m_InitialGunType == LGT_FLARE
            && lara_info->gun_type == m_InitialGunType;
        explode_status_check =
            state == LS(LS_JUMP_FORWARD) || state == LS(LS_JUMP_BACK);
    }

    if (state == LS(LS_JUMP_FORWARD) && gun_status_check) {
        M_CompleteLevel();
    } else if (state == LS(LS_JUMP_BACK) && gun_status_check) {
        M_GiveItems();
    } else if (explode_status_check) {
        M_ExplodeLara();
    }
    return true;
}

void Lara_Cheat_CheckKeys(void)
{
    if (Game_IsInGym()) {
        return;
    }

    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_STATE ls = lara_item->current_anim_state;

    switch (m_CheatState) {
    case CHEAT_INITIAL:
        m_CheatState = ls == LS(LS_WALK) ? CHEAT_STEP_FORWARD : CHEAT_INITIAL;
        break;

    case CHEAT_STEP_FORWARD:
        m_InitialGunType = lara_info->gun_type;
        if (ls != LS(LS_WALK)) {
            m_CheatState =
                ls == LS(LS_STOP) ? CHEAT_STEP_FORWARD_STOP : CHEAT_INITIAL;
        }
        break;

    case CHEAT_STEP_FORWARD_STOP:
        if (ls != LS(LS_STOP)) {
            m_CheatState =
                ls == LS(LS_WALK_BACK) ? CHEAT_STEP_BACK : CHEAT_INITIAL;
        }
        break;

    case CHEAT_STEP_BACK:
        if (ls != LS(LS_WALK_BACK)) {
            m_CheatState =
                ls == LS(LS_STOP) ? CHEAT_STEP_BACK_STOP : CHEAT_INITIAL;
        }
        break;

    case CHEAT_STEP_BACK_STOP:
        if (ls != LS(LS_STOP)) {
            m_CheatTurn = 0;
            m_CheatAngle = lara_item->rot.y;
            if (ls == LS(LS_TURN_LEFT)) {
                m_CheatState = CHEAT_TURN_LEFT;
            } else if (ls == LS(LS_TURN_RIGHT)) {
                m_CheatState = CHEAT_TURN_RIGHT;
            } else {
                m_CheatState = CHEAT_INITIAL;
            }
        }
        break;

    case CHEAT_TURN_LEFT:
        if (ls != LS(LS_TURN_LEFT) && ls != LS(LS_FAST_TURN)) {
            m_CheatState =
                m_CheatTurn < -M_MIN_TURN ? CHEAT_TURN_STOP : CHEAT_INITIAL;
        } else {
            m_CheatTurn += (int16_t)(lara_item->rot.y - m_CheatAngle);
            m_CheatAngle = lara_item->rot.y;
        }
        break;

    case CHEAT_TURN_RIGHT:
        if (ls != LS(LS_TURN_RIGHT) && ls != LS(LS_FAST_TURN)) {
            m_CheatState =
                m_CheatTurn > M_MIN_TURN ? CHEAT_TURN_STOP : CHEAT_INITIAL;
        } else {
            m_CheatTurn += (int16_t)(lara_item->rot.y - m_CheatAngle);
            m_CheatAngle = lara_item->rot.y;
        }
        break;

    case CHEAT_TURN_STOP:
        if (ls != LS(LS_STOP)) {
            m_CheatState =
                ls == LS(LS_COMPRESS) ? CHEAT_TURN_JUMP : CHEAT_INITIAL;
        }
        break;

    case CHEAT_TURN_JUMP:
        if (M_ProcessOutcome(lara_item)) {
            m_CheatState = CHEAT_INITIAL;
        }
        break;

    default:
        m_CheatState = CHEAT_INITIAL;
        break;
    }
}
