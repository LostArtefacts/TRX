#include <trx/game/lara/cheat_keys.h>

#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/sound.h>
#include <trx/version.h>

#define M_MIN_TURN 94208

typedef enum {
    // clang-format off
    CHEAT_INITIAL,
    CHEAT_STEP_FORWARD,
    CHEAT_STEP_FORWARD_STOP,
    CHEAT_STEP_BACK,
    CHEAT_STEP_BACK_STOP,
    CHEAT_TURN_LEFT,
    CHEAT_TURN_RIGHT,
    CHEAT_TURN_STOP,
    CHEAT_FINAL_JUMP,
    // clang-format on
} M_CHEAT_STATE;

static int32_t m_CheatState = CHEAT_INITIAL;
static LARA_GUN_TYPE m_InitialGunType = LGT_UNARMED;
static LARA_GUN_STATE m_InitialGunState = LGS_ARMLESS;
static int16_t m_CheatAngle = 0;
static int32_t m_CheatTurn = 0;

static void M_CompleteLevel(void)
{
    Game_SetIsLevelComplete(true);
}

static void M_GiveItems(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const info = Gun_Registry_GetByIndex(i);
        const LARA_GUN_TYPE gun_type = info->gun_type;
        if (info->cheat_key_ammo == 0) {
            continue;
        }
        if (Lara_Cheat_GiveGun(gun_type, false)) {
            Inv_SetAmmo(gun_type, info->cheat_key_ammo);
        }
    }
    Inv_AddItemNTimes(O_SMALL_MEDIPACK_ITEM, 50);
    Inv_AddItemNTimes(O_LARGE_MEDIPACK_ITEM, 50);
    if (Gun_Registry_Get(Gun_GetFlareType())->is_available) {
        Inv_AddItemNTimes(O_FLARE_ITEM, 50);
    }
    Sound_Effect(SFX_LARA_HOLSTER, nullptr, SPM_ALWAYS);
}

static void M_ExplodeLara(void)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    ITEM *const lara_item = Lara_GetItem();
    Item_Shatter(lara_info->item_num, -1, 1);
    Sound_Effect(SFX_EXPLOSION_1, &lara_item->pos, SPM_NORMAL);
    Lara_Kill();
    Item_SetVisible(lara_item, false);
    lara_item->is_collidable = false;
    lara_item->trigger.spent = true;
}

static bool M_ProcessOutcome(
    const LARA_INFO *const lara_info, const ITEM *const lara_item)
{
    if (lara_item->fall_speed <= 0) {
        return false;
    }

    const LARA_STATE_SLOT state = lara_item->current_anim_state;

    switch (g_TRVersion) {
    case 1:
        if (state == LS(LS_JUMP_FORWARD)) {
            M_CompleteLevel();
        } else if (state == LS(LS_JUMP_BACK)) {
            M_GiveItems();
        } else if (state == LS(LS_SWAN_DIVE)) {
            M_ExplodeLara();
        }
        break;

    case 2:
        if (Gun_IsFlareType(m_InitialGunType)
            && lara_info->gun_type == m_InitialGunType
            && lara_info->gun_status == m_InitialGunState) {
            if (state == LS(LS_JUMP_FORWARD)) {
                M_CompleteLevel();
            } else if (state == LS(LS_JUMP_BACK)) {
                M_GiveItems();
            }
        } else if (state == LS(LS_JUMP_FORWARD) || state == LS(LS_JUMP_BACK)) {
            M_ExplodeLara();
        }
        break;

    case 3:
        if (m_InitialGunType == Gun_GetDefaultType()
            && m_InitialGunState == LGS_READY
            && lara_info->gun_type == m_InitialGunType
            && lara_info->gun_status == m_InitialGunState) {
            if (state == LS(LS_JUMP_FORWARD)) {
                M_CompleteLevel();
            } else if (state == LS(LS_JUMP_BACK)) {
                M_GiveItems();
            }
        } else if (state == LS(LS_JUMP_FORWARD) || state == LS(LS_JUMP_BACK)) {
            M_ExplodeLara();
        }
    }

    return true;
}

static LARA_STATE_SLOT M_GetBackstepState(void)
{
    return g_TRVersion == 3 ? LS(LS_CROUCH_IDLE) : LS(LS_WALK_BACK);
}

void Lara_Cheat_CheckKeys(void)
{
    if (Game_IsInGym()) {
        return;
    }

    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_STATE_SLOT ls = lara_item->current_anim_state;
    const LARA_STATE_SLOT backstep_state = M_GetBackstepState();

    switch (m_CheatState) {
    case CHEAT_INITIAL:
        m_CheatState = ls == LS(LS_WALK) ? CHEAT_STEP_FORWARD : CHEAT_INITIAL;
        break;

    case CHEAT_STEP_FORWARD:
        m_InitialGunType = lara_info->gun_type;
        m_InitialGunState = lara_info->gun_status;
        if (ls != LS(LS_WALK)) {
            m_CheatState =
                ls == LS(LS_STOP) ? CHEAT_STEP_FORWARD_STOP : CHEAT_INITIAL;
        }
        break;

    case CHEAT_STEP_FORWARD_STOP:
        if (ls != LS(LS_STOP)) {
            m_CheatState =
                ls == backstep_state ? CHEAT_STEP_BACK : CHEAT_INITIAL;
        }
        break;

    case CHEAT_STEP_BACK:
        if (ls != backstep_state) {
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
                ls == LS(LS_COMPRESS) ? CHEAT_FINAL_JUMP : CHEAT_INITIAL;
        }
        break;

    case CHEAT_FINAL_JUMP:
        if (M_ProcessOutcome(lara_info, lara_item)) {
            m_CheatState = CHEAT_INITIAL;
        }
        break;

    default:
        m_CheatState = CHEAT_INITIAL;
        break;
    }
}
