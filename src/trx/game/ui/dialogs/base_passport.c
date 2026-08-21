#include <trx/game/ui/dialogs/base_passport.h>

#include <trx/core/utils.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/vars.h>
#include <trx/game/ui/elements/modal.h>
#include <trx/game/ui/elements/requester.h>
#include <trx/game/ui/elements/resize.h>
#include <trx/game/ui/scaler.h>
#include <trx/version.h>

// A list shorter than this is awkward to browse, so a dialog that cannot fit
// this many rows is drawn smaller instead of losing further rows.
#define M_MIN_VISIBLE_ROWS 5

// As many rows as the originals show, however much room a large screen leaves.
#define M_MAX_VISIBLE_ROWS 10

static float M_GetAvailableHeight(void)
{
    return UI_GetSafeCanvasHeight() / UI_Scaler_GetTextScale();
}

static int32_t M_GetVisibleRows(const UI_REQUESTER_STATE *const req)
{
    int32_t rows = UI_Requester_GetRowsForHeight(
        req, M_GetAvailableHeight() - req->footer_height);
    CLAMP(rows, M_MIN_VISIBLE_ROWS, M_MAX_VISIBLE_ROWS);
    return rows;
}

static float M_GetFitScale(const UI_REQUESTER_STATE *const req)
{
    const float natural_height =
        UI_Requester_GetHeight(req, req->scroll.vis_items) + req->footer_height;
    const float available = M_GetAvailableHeight();
    if (natural_height <= available || natural_height <= 0.0f) {
        return 1.0f;
    }
    return available / natural_height;
}

void UI_BasePassportDialog_Init(
    UI_REQUESTER_STATE *const req, const size_t max_rows,
    const float footer_height)
{
    UI_Requester_Init(req, 0, max_rows, true);
    req->row_pad = 4.0f;
    req->row_spacing = g_TRVersion == 1 ? 2.0f : 3.0f;
    req->show_arrows = true;
    req->reserve_space = true;
    req->footer_height = footer_height;
    UI_BasePassportDialog_Control(req);
}

void UI_BasePassportDialog_Control(UI_REQUESTER_STATE *const req)
{
    UI_Requester_SetVisibleRows(req, M_GetVisibleRows(req));
}

void UI_BeginBasePassportDialog(const UI_REQUESTER_STATE *const req)
{
    const float modal_y = g_InvRing_Mode == INV_TITLE_MODE ? 0.98f : 0.67f;
    UI_BeginModal(0.5f, modal_y);
    UI_Scaler_PushTextScale(M_GetFitScale(req));
    UI_BeginResizeEx((UI_RESIZE_SETTINGS) {
        .w = 300.0f,
        .h = -1.0f,
        .align_h = 0.5f,
    });
}

void UI_EndBasePassportDialog(void)
{
    UI_EndResize();
    UI_Scaler_PopTextScale();
    UI_EndModal();
}
