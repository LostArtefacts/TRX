#include "game/ui/dialogs/base_passport.h"

#include "game/inventory.h"
#include "game/scaler.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/requester.h"
#include "game/ui/elements/resize.h"
#include "game/viewport.h"

static int32_t M_GetVisibleRows(void)
{
    if (TR_VERSION == 2) {
        return 10;
    } else {
        const int32_t res_h = Scaler_CalcInverse(
            Viewport_GetHeight(VIEWPORT_UI), SCALER_TARGET_TEXT);
        if (res_h <= 240) {
            return 5;
        } else if (res_h <= 384) {
            return 7;
        } else if (res_h <= 480) {
            return 10;
        } else {
            return 12;
        }
    }
}

void UI_BasePassportDialog_Init(
    UI_REQUESTER_STATE *const req, const size_t max_rows)
{
    UI_Requester_Init(req, M_GetVisibleRows(), max_rows, true);
    req->row_pad = 4.0f;
    req->row_spacing = TR_VERSION == 1 ? 2.0f : 3.0f;
    req->show_arrows = TR_VERSION == 1;
    req->reserve_space = true;
}

void UI_BasePassportDialog_Control(UI_REQUESTER_STATE *const req)
{
    UI_Requester_SetVisibleRows(req, M_GetVisibleRows());
}

void UI_BeginBasePassportDialog(void)
{
    const float modal_y = TR_VERSION == 1
        ? (g_Inv_Mode == INV_TITLE_MODE ? 0.72f : 0.55f)
        : (g_Inv_Mode == INV_TITLE_MODE ? 0.8f : 0.65f);
    UI_BeginModal(0.5f, modal_y);
    UI_BeginResize(300.0f, -1.0f);
}

void UI_EndBasePassportDialog(void)
{
    UI_EndResize();
    UI_EndModal();
}
