#include "game/option/option_examine.h"

#include "game/input.h"

#include <libtrx/game/objects/names.h>
#include <libtrx/game/ui2.h>

#define MAX_LINES 10

typedef struct {
    struct {
        bool is_ready;
        UI2_EXAMINE_ITEM_STATE state;
    } ui;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_Init(M_PRIV *p, GAME_OBJECT_ID obj_id);
static void M_Shutdown(M_PRIV *p);

static void M_Init(M_PRIV *const p, const GAME_OBJECT_ID obj_id)
{
    p->ui.is_ready = true;
    UI2_ExamineItem_Init(
        &p->ui.state, Object_GetName(obj_id), Object_GetDescription(obj_id),
        MAX_LINES);
}

static void M_Shutdown(M_PRIV *const p)
{
    if (p->ui.is_ready) {
        UI2_ExamineItem_Free(&p->ui.state);
        p->ui.is_ready = false;
    }
}

bool Option_Examine_CanExamine(const GAME_OBJECT_ID obj_id)
{
    return Object_GetDescription(obj_id) != nullptr;
}

bool Option_Examine_IsActive(void)
{
    const M_PRIV *const p = &m_Priv;
    return p->ui.is_ready;
}

void Option_Examine_Control(const GAME_OBJECT_ID obj_id, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }

    if (!p->ui.is_ready) {
        M_Init(p, obj_id);
    }
    UI2_ExamineItem_Control(&p->ui.state);

    if (g_InputDB.menu_back || g_InputDB.menu_confirm) {
        M_Shutdown(p);
    }
}

void Option_Examine_Draw(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->ui.is_ready) {
        UI2_ExamineItem(&p->ui.state);
    }
}

void Option_Examine_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    M_Shutdown(p);
}
