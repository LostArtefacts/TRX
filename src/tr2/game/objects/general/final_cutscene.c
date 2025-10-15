#include "game/game.h"
#include "game/objects/general/combat_end.h"

#include <libtrx/game/const.h>
#include <libtrx/game/objects.h>

#define M_CUTSCENE_DURATION (11.5 * LOGIC_FPS)

static int32_t m_FadeTimer = -1;

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (CombatEnd_IsComplete()) {
        item->status = IS_ACTIVE;
        Item_Animate(item);

        if (m_FadeTimer == -1) {
            m_FadeTimer = M_CUTSCENE_DURATION;
        } else if (m_FadeTimer == 1) {
            Game_FadeToBlack(3 * LOGIC_FPS);
            m_FadeTimer--;
        } else if (m_FadeTimer > 0) {
            m_FadeTimer--;
        }
    } else {
        item->status = IS_INVISIBLE;
        m_FadeTimer = -1;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_CUT_SHOTGUN, M_Setup)
