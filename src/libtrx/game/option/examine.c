#include "game/option/examine.h"

#include "game/input.h"
#include "game/objects/names.h"
#include "game/scaler.h"
#include "game/ui.h"
#include "game/viewport.h"

typedef struct {
    GAME_OBJECT_ID object_id;
    struct {
        bool is_ready;
        UI_TEXT_DIALOG_STATE *state;
    } ui;
} M_PRIV;

static M_PRIV m_Priv = {};

static int32_t M_GetMaxRows(void);
static void M_Init(M_PRIV *p, GAME_OBJECT_ID obj_id);
static void M_Close(M_PRIV *p);

static int32_t M_GetMaxRows(void)
{
    const int32_t res_h =
        Scaler_CalcInverse(Viewport_GetHeight(VIEWPORT_UI), SCALER_TARGET_TEXT);
    if (res_h <= 240) {
        return 5;
    } else if (res_h <= 384) {
        return 7;
    } else {
        return 12;
    }
}

static void M_Init(M_PRIV *const p, const GAME_OBJECT_ID obj_id)
{
    p->object_id = obj_id;
    p->ui.is_ready = true;
    p->ui.state = UI_TextDialog_Init(
        UI_GetCanvasWidth() * 2.0 / 3.0f, M_GetMaxRows(), false);
}

static void M_Close(M_PRIV *const p)
{
    if (p->ui.is_ready) {
        UI_TextDialog_Free(p->ui.state);
        p->ui.state = nullptr;
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
    UI_TextDialog_Control(p->ui.state);

    if (g_InputDB.look) {
        g_InputDB.menu_back = true;
    }
    if (g_InputDB.menu_back || g_InputDB.menu_confirm) {
        M_Close(p);
    }
}

void Option_Examine_Draw(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->ui.is_ready) {
        UI_TextDialog(
            p->ui.state, Object_GetName(p->object_id),
            Object_GetDescription(p->object_id));
    }
}

void Option_Examine_Close(void)
{
    M_PRIV *const p = &m_Priv;
    M_Close(p);
}
