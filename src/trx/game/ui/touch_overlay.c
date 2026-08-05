#include <trx/game/ui/touch_overlay.h>

#include <trx/config.h>
#include <trx/core/colors.h>
#include <trx/core/utils.h>
#include <trx/game/game/state.h>
#include <trx/game/input/backends/touch.h>
#include <trx/game/input/common.h>
#include <trx/game/lara/common.h>
#include <trx/game/objects.h>
#include <trx/game/output/const.h>
#include <trx/game/output/textures.h>
#include <trx/game/ui/draw.h>
#include <trx/game/viewport.h>

#include <SDL2/SDL_events.h>
#include <math.h>
#include <string.h>

// Touch positions identify each assignable location on the overlay:
//   0..3: virtual D-pad directions (UP, DOWN, LEFT, RIGHT).
//   4+:   action/system buttons, one per non-dpad button def.
#define M_POS_DPAD_UP 0
#define M_POS_DPAD_DOWN 1
#define M_POS_DPAD_LEFT 2
#define M_POS_DPAD_RIGHT 3
#define M_POS_BUTTON_BASE 4

// The D-pad occupies button def 0 but expands to positions 0..3; non-dpad
// button defs start at def index 1, which maps to position 4.
#define M_DEF_TO_POS(def_idx) ((def_idx) + M_POS_BUTTON_BASE - 1)
#define M_POS_TO_DEF(pos) ((pos) - M_POS_BUTTON_BASE + 1)

#define M_MAX_FINGERS 10
#define M_DPAD_DIR_THRESHOLD 0.38f
#define M_DPAD_THUMB_LIMIT 0.68f
#define M_DPAD_THUMB_RATIO 0.25f
#define M_DPAD_BASE_SCALE 1.2f
#define M_HIT_GENEROSITY 1.2f
#define M_FINGER_RADIUS 0.020f
#define M_BORDER_RATIO 0.004f
#define M_ICON_SCALE 1.4f
#define M_DRAW_WEAPON_ALT_OFFSET_X 0.21f
#define M_DRAW_WEAPON_ALT_OFFSET_Y 0.32f

// Sprite index offsets within O_ALPHABET for touch overlay icons.
// Must match mapping_touch.txt and text_autogen.def.
#define M_SPRITE_JUMP 782
#define M_SPRITE_CROUCH 783
#define M_SPRITE_WALK 784
#define M_SPRITE_RUN 785
#define M_SPRITE_DRAW_WEAPON 786
#define M_SPRITE_LOOK 787
#define M_SPRITE_ACTION 788
#define M_SPRITE_ROLL 789
#define M_SPRITE_MENU 790
#define M_SPRITE_PAUSE 791
#define M_SPRITE_BULLET 792
#define M_SPRITE_SWIM 793

#define M_NUM_BUTTON_DEFS ((int32_t)ARRAY_SIZE(m_ButtonDefs))
#define M_MAX_BUTTONS M_NUM_BUTTON_DEFS

// D-pad expands its single def into four positions; all other defs map 1:1.
#define M_NUM_POSITIONS (M_POS_BUTTON_BASE + M_NUM_BUTTON_DEFS - 1)

typedef enum {
    M_ANCHOR_BOTTOM_LEFT,
    M_ANCHOR_BOTTOM_RIGHT,
    M_ANCHOR_TOP_CENTER,
} M_TOUCH_ANCHOR;

typedef struct {
    INPUT_ROLE role;
    M_TOUCH_ANCHOR anchor;
    float offset_x;
    float offset_y;
    float radius;
    // Engine feature this button represents. The button is only shown when
    // the predicate returns true. nullptr means the button is not tied to
    // any feature and is always shown.
    bool (*is_feature_enabled)(void);
    bool is_dpad;
} M_TOUCH_BUTTON_DEF;

typedef struct {
    float cx;
    float cy;
    float radius;
    bool visible;
    bool active;
    INPUT_ROLE role;
    bool is_dpad;
    int32_t def_index;
} M_TOUCH_BUTTON;

typedef struct {
    SDL_FingerID id;
    float x;
    float y;
    bool active;
} M_FINGER_STATE;

static bool M_IsSprintEnabled(void);
static bool M_IsCrouchEnabled(void);

// Button layout matching the WebGL implementation.
// Sizes/offsets are fractions of viewport min(width, height).
// Draw-weapon's offset is resolved at layout time: when sprint is enabled it
// moves out of the way so sprint can take the slot right of jump.
// clang-format off
static const M_TOUCH_BUTTON_DEF m_ButtonDefs[] = {
    // D-pad
    { .role = INPUT_ROLE_UP,          .anchor = M_ANCHOR_BOTTOM_LEFT,  .offset_x = 0.15f, .offset_y = 0.15f, .radius = 0.11f, .is_dpad = true },

    // Main action buttons
    { .role = INPUT_ROLE_JUMP,        .anchor = M_ANCHOR_BOTTOM_RIGHT, .offset_x = 0.10f, .offset_y = 0.10f, .radius = 0.065f },
    { .role = INPUT_ROLE_ACTION,      .anchor = M_ANCHOR_BOTTOM_RIGHT, .offset_x = 0.21f, .offset_y = 0.21f, .radius = 0.065f },

    // Orbit buttons around ACTION
    { .role = INPUT_ROLE_SLOW,        .anchor = M_ANCHOR_BOTTOM_RIGHT, .offset_x = 0.32f, .offset_y = 0.21f, .radius = 0.035f },
    { .role = INPUT_ROLE_LOOK,        .anchor = M_ANCHOR_BOTTOM_RIGHT, .offset_x = 0.29f, .offset_y = 0.13f, .radius = 0.035f },
    { .role = INPUT_ROLE_ROLL,        .anchor = M_ANCHOR_BOTTOM_RIGHT, .offset_x = 0.21f, .offset_y = 0.10f, .radius = 0.035f },
    { .role = INPUT_ROLE_DRAW_WEAPON, .anchor = M_ANCHOR_BOTTOM_RIGHT, .offset_x = 0.10f, .offset_y = 0.21f, .radius = 0.035f },
    { .role = INPUT_ROLE_SPRINT,      .anchor = M_ANCHOR_BOTTOM_RIGHT, .offset_x = 0.10f, .offset_y = 0.21f, .radius = 0.035f, .is_feature_enabled = M_IsSprintEnabled },
    { .role = INPUT_ROLE_CROUCH,      .anchor = M_ANCHOR_BOTTOM_RIGHT, .offset_x = 0.13f, .offset_y = 0.29f, .radius = 0.035f, .is_feature_enabled = M_IsCrouchEnabled },

    // Top bar
    { .role = INPUT_ROLE_INVENTORY,   .anchor = M_ANCHOR_TOP_CENTER,   .offset_x = -0.05f, .offset_y = 0.04f, .radius = 0.03f },
    { .role = INPUT_ROLE_PAUSE,       .anchor = M_ANCHOR_TOP_CENTER,   .offset_x = 0.05f, .offset_y = 0.04f, .radius = 0.03f },
};
// clang-format on

static bool m_Visible = false;
static M_TOUCH_BUTTON m_Buttons[M_MAX_BUTTONS];
static int32_t m_NumButtons = 0;
static M_FINGER_STATE m_Fingers[M_MAX_FINGERS];
static float m_FingerRadius = 0.0f;
static float m_BorderThickness = 0.0f;

static SDL_FingerID m_DpadFingerId = -1;
static bool m_DpadActive = false;
static float m_DpadThumbX = 0.0f;
static float m_DpadThumbY = 0.0f;

static bool m_SelectionMode = false;
static int32_t m_SelectedPosition = -1;
static bool m_WasVisibleBeforeSelection = false;
static int32_t m_SelectionFingerCount = 0;

// Dark centre of each button.
static const RGBA_8888 m_DiscNormal = { .r = 30, .g = 30, .b = 30, .a = 120 };
static const RGBA_8888 m_DiscActive = { .r = 30, .g = 30, .b = 30, .a = 210 };
// Bright outline ring.
static const RGBA_8888 m_RingNormal = {
    .r = 255, .g = 255, .b = 255, .a = 120
};
static const RGBA_8888 m_RingActive = {
    .r = 255, .g = 255, .b = 255, .a = 220
};
// Icon glyph tint.
static const RGBA_8888 m_BorderNormal = {
    .r = 255, .g = 255, .b = 255, .a = 113
};
static const RGBA_8888 m_BorderActive = {
    .r = 255, .g = 255, .b = 255, .a = 239
};

static bool M_IsSprintEnabled(void)
{
    return g_Config.gameplay.enable_sprint;
}

static bool M_IsCrouchEnabled(void)
{
    return g_Config.gameplay.enable_crawling;
}

static bool M_IsDefEnabled(const M_TOUCH_BUTTON_DEF *const def)
{
    return def->is_feature_enabled == nullptr || def->is_feature_enabled();
}

// Draw-weapon's offsets are context-dependent: it moves to make room for
// sprint when that feature is enabled.
static void M_GetDefOffset(
    const M_TOUCH_BUTTON_DEF *const def, float *const out_x, float *const out_y)
{
    if (def->role == INPUT_ROLE_DRAW_WEAPON && M_IsSprintEnabled()) {
        *out_x = M_DRAW_WEAPON_ALT_OFFSET_X;
        *out_y = M_DRAW_WEAPON_ALT_OFFSET_Y;
        return;
    }
    *out_x = def->offset_x;
    *out_y = def->offset_y;
}

static void M_ComputeButtonLayout(void)
{
    const int32_t vw = Viewport_GetWidth(VIEWPORT_UI);
    const int32_t vh = Viewport_GetHeight(VIEWPORT_UI);
    if (vw <= 0 || vh <= 0) {
        m_NumButtons = 0;
        return;
    }

    const float ref = (float)(vw < vh ? vw : vh);
    const float btn_scale = g_Config.input.touch_button_scale;
    const float dpad_scale = g_Config.input.touch_dpad_scale;

    m_FingerRadius = M_FINGER_RADIUS * ref * btn_scale;
    m_BorderThickness = ref * M_BORDER_RATIO * btn_scale;

    m_NumButtons = 0;
    for (int32_t i = 0; i < M_NUM_BUTTON_DEFS; i++) {
        const M_TOUCH_BUTTON_DEF *def = &m_ButtonDefs[i];
        if (!M_IsDefEnabled(def)) {
            continue;
        }

        const float scale =
            def->is_dpad ? (dpad_scale * M_DPAD_BASE_SCALE) : btn_scale;

        float offset_x;
        float offset_y;
        M_GetDefOffset(def, &offset_x, &offset_y);

        M_TOUCH_BUTTON *btn = &m_Buttons[m_NumButtons];
        btn->def_index = i;
        btn->role = def->role;
        btn->is_dpad = def->is_dpad;
        btn->radius = def->radius * ref * scale;
        btn->active = false;
        btn->visible = true;

        float ax, ay;
        switch (def->anchor) {
        case M_ANCHOR_BOTTOM_LEFT:
            ax = 0.0f;
            ay = (float)vh;
            btn->cx = ax + offset_x * ref * scale;
            btn->cy = ay - offset_y * ref * scale;
            break;
        case M_ANCHOR_BOTTOM_RIGHT:
            ax = (float)vw;
            ay = (float)vh;
            btn->cx = ax - offset_x * ref * scale;
            btn->cy = ay - offset_y * ref * scale;
            break;
        case M_ANCHOR_TOP_CENTER:
            ax = (float)vw / 2.0f;
            ay = 0.0f;
            btn->cx = ax + offset_x * ref * scale;
            btn->cy = ay + offset_y * ref * scale;
            break;
        }

        m_NumButtons++;
    }
}

static int32_t M_GetGlyphIndex(const INPUT_ROLE role)
{
    // clang-format off
    switch (role) {
    case INPUT_ROLE_JUMP: {
        const LARA_INFO *const lara = Lara_GetLaraInfo();
        if (Game_IsPlaying() && lara != nullptr
            && (lara->water_status == LWS_UNDERWATER
                || lara->water_status == LWS_SURFACE
                || lara->water_status == LWS_CHEAT)) {
            return M_SPRITE_SWIM;
        }
        return M_SPRITE_JUMP;
    }
    case INPUT_ROLE_SLOW:        return M_SPRITE_WALK;
    case INPUT_ROLE_SPRINT:      return M_SPRITE_RUN;
    case INPUT_ROLE_CROUCH:      return M_SPRITE_CROUCH;
    case INPUT_ROLE_DRAW_WEAPON: return M_SPRITE_DRAW_WEAPON;
    case INPUT_ROLE_LOOK:        return M_SPRITE_LOOK;
    case INPUT_ROLE_ACTION: {
        const LARA_INFO *const lara = Lara_GetLaraInfo();
        if (Game_IsPlaying() && lara != nullptr
            && (lara->gun_status == LGS_READY || lara->gun_status == LGS_DRAW
                || lara->gun_status == LGS_UNDRAW
                || lara->water_status == LWS_CHEAT)) {
            return M_SPRITE_BULLET;
        }
        return M_SPRITE_ACTION;
    }
    case INPUT_ROLE_ROLL:        return M_SPRITE_ROLL;
    case INPUT_ROLE_INVENTORY:   return M_SPRITE_MENU;
    case INPUT_ROLE_PAUSE:       return M_SPRITE_PAUSE;
    default:                     return -1;
    }
    // clang-format on
}

// Draw a touch sprite from O_ALPHABET centered at (cx, cy), scaled to fit
// within a square of side `diameter` pixels.
static void M_DrawTouchSprite(
    const int32_t cx, const int32_t cy, const int32_t z,
    const int32_t glyph_idx, const int32_t diameter, const RGBA_F colors[4])
{
    if (glyph_idx < 0 || diameter <= 0) {
        return;
    }
    const OBJECT *const alphabet = Object_Get(O_ALPHABET);
    if (alphabet == nullptr || !alphabet->loaded) {
        return;
    }
    const int32_t sprite_idx = alphabet->mesh_idx + glyph_idx;
    const SPRITE_TEXTURE *const spr = Output_GetSpriteTexture(sprite_idx);
    if (spr == nullptr) {
        return;
    }
    const int32_t sw = spr->x1 - spr->x0;
    const int32_t sh = spr->y1 - spr->y0;
    if (sw <= 0 || sh <= 0) {
        return;
    }
    const int32_t scale =
        (int32_t)((float)diameter * PHD_ONE / (sw > sh ? sw : sh));
    const int32_t scale_h = scale;
    const int32_t scale_v = scale;
    const int32_t sx =
        cx - (int32_t)((float)scale_h * (spr->x0 + spr->x1) / 2.0f / PHD_ONE);
    const int32_t sy =
        cy - (int32_t)((float)scale_v * (spr->y0 + spr->y1) / 2.0f / PHD_ONE);
    UI_ScheduleDrawScreenSprite(
        sx, sy, z, scale_h, scale_v, sprite_idx, colors);
}

static RGBA_8888 M_ApplyOpacity(const RGBA_8888 color, const float opacity)
{
    const float a = (float)color.a * opacity;
    return (RGBA_8888) {
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = (uint8_t)(a < 0.0f ? 0.0f : (a > 255.0f ? 255.0f : a)),
    };
}

static void M_DrawButton(const M_TOUCH_BUTTON *const btn, const int32_t z)
{
    if (!btn->visible) {
        return;
    }

    const float opacity = g_Config.input.touch_opacity;
    const RGBA_8888 disc_raw = btn->active ? m_DiscActive : m_DiscNormal;
    const RGBA_8888 ring_raw = btn->active ? m_RingActive : m_RingNormal;
    const RGBA_8888 border_raw = btn->active ? m_BorderActive : m_BorderNormal;

    const int32_t cx = (int32_t)roundf(btn->cx);
    const int32_t cy = (int32_t)roundf(btn->cy);
    const int32_t r = (int32_t)roundf(btn->radius);
    const int32_t r_inner = r - (int32_t)roundf(m_BorderThickness);

    UI_ScheduleDrawScreenCircle(
        cx, cy, 0, r, z, M_ApplyOpacity(disc_raw, opacity));
    UI_ScheduleDrawScreenCircle(
        cx, cy, r_inner, r, z, M_ApplyOpacity(ring_raw, opacity));

    const int32_t pos = M_DEF_TO_POS(btn->def_index);
    const INPUT_ROLE bound_role = Touch_GetPositionRole(pos);
    const int32_t glyph_idx = M_GetGlyphIndex(bound_role);
    const int32_t icon_d = (int32_t)((float)r * M_ICON_SCALE);
    const float icon_a = ((float)border_raw.a / 255.0f) * opacity;
    const RGBA_F icon_f = {
        .r = ((float)border_raw.r / 255.0f) * icon_a,
        .g = ((float)border_raw.g / 255.0f) * icon_a,
        .b = ((float)border_raw.b / 255.0f) * icon_a,
        .a = icon_a,
    };
    const RGBA_F icon_colors[4] = { icon_f, icon_f, icon_f, icon_f };
    M_DrawTouchSprite(cx, cy, z + 1, glyph_idx, icon_d, icon_colors);
}

static void M_DrawDpad(const int32_t z)
{
    const M_TOUCH_BUTTON *dpad = nullptr;
    for (int32_t i = 0; i < m_NumButtons; i++) {
        if (m_Buttons[i].is_dpad) {
            dpad = &m_Buttons[i];
            break;
        }
    }
    if (dpad == nullptr || !dpad->visible) {
        return;
    }

    const float opacity = g_Config.input.touch_opacity;
    const RGBA_8888 bg_disc = M_ApplyOpacity(m_DiscNormal, opacity);
    const RGBA_8888 bg_ring = M_ApplyOpacity(m_RingNormal, opacity);
    const RGBA_8888 thumb_col = M_ApplyOpacity(
        (RGBA_8888) { .r = 200, .g = 200, .b = 200, .a = 180 }, opacity);

    const int32_t cx = (int32_t)roundf(dpad->cx);
    const int32_t cy = (int32_t)roundf(dpad->cy);
    const int32_t r = (int32_t)roundf(dpad->radius);
    const int32_t border = (int32_t)roundf(m_BorderThickness);

    UI_ScheduleDrawScreenCircle(cx, cy, 0, r, z, bg_disc);
    UI_ScheduleDrawScreenCircle(cx, cy, r - border, r, z, bg_ring);

    const int32_t dz_r =
        (int32_t)roundf((float)r * g_Config.input.touch_dpad_deadzone);
    UI_ScheduleDrawScreenCircle(cx, cy, dz_r - border, dz_r, z, bg_ring);

    const int32_t thumb_r = (int32_t)roundf((float)r * M_DPAD_THUMB_RATIO);
    const int32_t tx = cx + (int32_t)roundf(m_DpadThumbX * (float)r);
    const int32_t ty = cy + (int32_t)roundf(m_DpadThumbY * (float)r);
    UI_ScheduleDrawScreenCircle(tx, ty, 0, thumb_r, z + 1, thumb_col);
}

static void M_ResetDpad(void)
{
    m_DpadActive = false;
    m_DpadFingerId = -1;
    m_DpadThumbX = 0.0f;
    m_DpadThumbY = 0.0f;
    Touch_SetPositionState(M_POS_DPAD_UP, false);
    Touch_SetPositionState(M_POS_DPAD_DOWN, false);
    Touch_SetPositionState(M_POS_DPAD_LEFT, false);
    Touch_SetPositionState(M_POS_DPAD_RIGHT, false);
}

static void M_UpdateDpadFromFinger(
    const M_TOUCH_BUTTON *const dpad, const float fx, const float fy)
{
    const float dx = fx - dpad->cx;
    const float dy = fy - dpad->cy;
    const float dist = sqrtf(dx * dx + dy * dy);
    const float r = dpad->radius;

    const float nx = dist > 0.0f ? dx / dist : 0.0f;
    const float ny = dist > 0.0f ? dy / dist : 0.0f;
    const float clamp =
        (dist < r * M_DPAD_THUMB_LIMIT) ? dist : r * M_DPAD_THUMB_LIMIT;
    m_DpadThumbX = (nx * clamp) / r;
    m_DpadThumbY = (ny * clamp) / r;

    if (dist < r * g_Config.input.touch_dpad_deadzone) {
        Touch_SetPositionState(M_POS_DPAD_UP, false);
        Touch_SetPositionState(M_POS_DPAD_DOWN, false);
        Touch_SetPositionState(M_POS_DPAD_LEFT, false);
        Touch_SetPositionState(M_POS_DPAD_RIGHT, false);
        return;
    }

    Touch_SetPositionState(M_POS_DPAD_UP, ny < -M_DPAD_DIR_THRESHOLD);
    Touch_SetPositionState(M_POS_DPAD_DOWN, ny > M_DPAD_DIR_THRESHOLD);
    Touch_SetPositionState(M_POS_DPAD_LEFT, nx < -M_DPAD_DIR_THRESHOLD);
    Touch_SetPositionState(M_POS_DPAD_RIGHT, nx > M_DPAD_DIR_THRESHOLD);
}

static M_TOUCH_BUTTON *M_FindDpad(void)
{
    for (int32_t i = 0; i < m_NumButtons; i++) {
        if (m_Buttons[i].is_dpad && m_Buttons[i].visible) {
            return &m_Buttons[i];
        }
    }
    return nullptr;
}

static bool M_IsInDpad(
    const M_TOUCH_BUTTON *const dpad, const float px, const float py)
{
    const float dx = px - dpad->cx;
    const float dy = py - dpad->cy;
    return dx * dx + dy * dy
        <= dpad->radius * dpad->radius * M_HIT_GENEROSITY * M_HIT_GENEROSITY;
}

static void M_SyncButtonStates(void)
{
    for (int32_t i = 0; i < m_NumButtons; i++) {
        M_TOUCH_BUTTON *btn = &m_Buttons[i];
        if (btn->is_dpad) {
            continue;
        }
        btn->active = false;
    }

    // Each active finger activates all buttons whose circle overlaps the
    // finger's contact circle (radius m_FingerRadius), allowing a single
    // finger to press two adjacent buttons at once.
    for (int32_t f = 0; f < M_MAX_FINGERS; f++) {
        if (!m_Fingers[f].active) {
            continue;
        }
        if (m_DpadActive && m_Fingers[f].id == m_DpadFingerId) {
            continue;
        }
        const float fx = m_Fingers[f].x;
        const float fy = m_Fingers[f].y;
        for (int32_t i = 0; i < m_NumButtons; i++) {
            M_TOUCH_BUTTON *btn = &m_Buttons[i];
            if (!btn->visible || btn->is_dpad) {
                continue;
            }
            const float dx = fx - btn->cx;
            const float dy = fy - btn->cy;
            const float hit_r = btn->radius * M_HIT_GENEROSITY + m_FingerRadius;
            if (dx * dx + dy * dy <= hit_r * hit_r) {
                btn->active = true;
            }
        }
    }

    for (int32_t i = 0; i < m_NumButtons; i++) {
        M_TOUCH_BUTTON *btn = &m_Buttons[i];
        if (btn->is_dpad) {
            continue;
        }
        const int32_t pos = M_DEF_TO_POS(btn->def_index);
        Touch_SetPositionState(pos, btn->active);
    }
}

static M_FINGER_STATE *M_FindFinger(const SDL_FingerID id)
{
    for (int32_t i = 0; i < M_MAX_FINGERS; i++) {
        if (m_Fingers[i].active && m_Fingers[i].id == id) {
            return &m_Fingers[i];
        }
    }
    return nullptr;
}

static M_FINGER_STATE *M_AllocFinger(void)
{
    for (int32_t i = 0; i < M_MAX_FINGERS; i++) {
        if (!m_Fingers[i].active) {
            return &m_Fingers[i];
        }
    }
    return nullptr;
}

static int32_t M_HitTestPosition(const float px, const float py)
{
    M_TOUCH_BUTTON *dpad = M_FindDpad();
    if (dpad != nullptr && M_IsInDpad(dpad, px, py)) {
        const float dx = px - dpad->cx;
        const float dy = py - dpad->cy;
        if (fabsf(dy) > fabsf(dx)) {
            return dy < 0 ? M_POS_DPAD_UP : M_POS_DPAD_DOWN;
        } else {
            return dx < 0 ? M_POS_DPAD_LEFT : M_POS_DPAD_RIGHT;
        }
    }

    for (int32_t i = 0; i < m_NumButtons; i++) {
        M_TOUCH_BUTTON *btn = &m_Buttons[i];
        if (!btn->visible || btn->is_dpad) {
            continue;
        }
        const float dx = px - btn->cx;
        const float dy = py - btn->cy;
        const float hit_r = btn->radius * M_HIT_GENEROSITY;
        if (dx * dx + dy * dy <= hit_r * hit_r) {
            return M_DEF_TO_POS(btn->def_index);
        }
    }
    return -1;
}

static void M_HandleFingerDown(const SDL_TouchFingerEvent *const ev)
{
    const int32_t vw = Viewport_GetWidth(VIEWPORT_UI);
    const int32_t vh = Viewport_GetHeight(VIEWPORT_UI);
    const float px = ev->x * vw;
    const float py = ev->y * vh;

    if (m_SelectionMode) {
        M_ComputeButtonLayout();
        m_SelectionFingerCount++;
        const int32_t pos = M_HitTestPosition(px, py);
        if (pos >= 0) {
            m_SelectedPosition = pos;
        }
        return;
    }

    M_FINGER_STATE *finger = M_AllocFinger();
    if (finger == nullptr) {
        return;
    }
    finger->id = ev->fingerId;
    finger->x = px;
    finger->y = py;
    finger->active = true;

    M_TOUCH_BUTTON *dpad = M_FindDpad();
    if (dpad != nullptr && !m_DpadActive && M_IsInDpad(dpad, px, py)) {
        m_DpadActive = true;
        m_DpadFingerId = ev->fingerId;
        M_UpdateDpadFromFinger(dpad, px, py);
    }

    M_SyncButtonStates();
}

static void M_HandleFingerMotion(const SDL_TouchFingerEvent *const ev)
{
    if (m_SelectionMode) {
        return;
    }

    const int32_t vw = Viewport_GetWidth(VIEWPORT_UI);
    const int32_t vh = Viewport_GetHeight(VIEWPORT_UI);
    const float px = ev->x * vw;
    const float py = ev->y * vh;

    M_FINGER_STATE *finger = M_FindFinger(ev->fingerId);
    if (finger == nullptr) {
        return;
    }
    finger->x = px;
    finger->y = py;

    if (m_DpadActive && ev->fingerId == m_DpadFingerId) {
        M_TOUCH_BUTTON *dpad = M_FindDpad();
        if (dpad != nullptr) {
            M_UpdateDpadFromFinger(dpad, px, py);
        }
    }

    M_SyncButtonStates();
}

static void M_HandleFingerUp(const SDL_TouchFingerEvent *const ev)
{
    if (m_SelectionMode) {
        if (m_SelectionFingerCount > 0) {
            m_SelectionFingerCount--;
        }
        return;
    }

    M_FINGER_STATE *finger = M_FindFinger(ev->fingerId);
    if (finger != nullptr) {
        finger->active = false;
    }

    if (m_DpadActive && ev->fingerId == m_DpadFingerId) {
        M_ResetDpad();
    }

    M_SyncButtonStates();
}

bool TouchOverlay_HasAnyFingerDown(void)
{
    if (m_SelectionMode) {
        return m_SelectionFingerCount > 0;
    }
    for (int32_t i = 0; i < M_MAX_FINGERS; i++) {
        if (m_Fingers[i].active) {
            return true;
        }
    }
    return false;
}

void TouchOverlay_Init(void)
{
    m_Visible = false;
    m_NumButtons = 0;
    m_DpadActive = false;
    m_DpadFingerId = -1;
    m_DpadThumbX = 0.0f;
    m_DpadThumbY = 0.0f;
    m_FingerRadius = 0.0f;
    m_BorderThickness = 0.0f;
    memset(m_Fingers, 0, sizeof(m_Fingers));
}

void TouchOverlay_Shutdown(void)
{
    m_Visible = false;
    m_NumButtons = 0;
}

void TouchOverlay_SetVisible(const bool visible)
{
    m_Visible = visible;
    if (!visible) {
        M_ResetDpad();
        for (int32_t i = 0; i < m_NumButtons; i++) {
            if (!m_Buttons[i].is_dpad) {
                Touch_SetPositionState(
                    M_DEF_TO_POS(m_Buttons[i].def_index), false);
            }
            m_Buttons[i].active = false;
        }
        memset(m_Fingers, 0, sizeof(m_Fingers));
    }
}

bool TouchOverlay_IsVisible(void)
{
    return m_Visible;
}

void TouchOverlay_Draw(void)
{
    if (!m_Visible) {
        return;
    }

    M_ComputeButtonLayout();
    if (m_NumButtons == 0) {
        return;
    }
    M_SyncButtonStates();

    const int32_t z = 0;

    M_DrawDpad(z);

    for (int32_t i = 0; i < m_NumButtons; i++) {
        if (!m_Buttons[i].is_dpad) {
            M_DrawButton(&m_Buttons[i], z);
        }
    }
}

bool TouchOverlay_ProcessEvent(const SDL_Event *const event)
{
    if (!m_Visible) {
        return false;
    }

    switch (event->type) {
    case SDL_FINGERDOWN:
        M_HandleFingerDown(&event->tfinger);
        return true;
    case SDL_FINGERMOTION:
        M_HandleFingerMotion(&event->tfinger);
        return true;
    case SDL_FINGERUP:
        M_HandleFingerUp(&event->tfinger);
        return true;
    default:
        return false;
    }
}

int32_t TouchOverlay_GetPositionCount(void)
{
    return M_NUM_POSITIONS;
}

INPUT_ROLE TouchOverlay_GetPositionDefaultRole(const int32_t position)
{
    if (position < 0 || position >= M_NUM_POSITIONS) {
        return (INPUT_ROLE)-1;
    }
    switch (position) {
    case M_POS_DPAD_UP:
        return INPUT_ROLE_UP;
    case M_POS_DPAD_DOWN:
        return INPUT_ROLE_DOWN;
    case M_POS_DPAD_LEFT:
        return INPUT_ROLE_LEFT;
    case M_POS_DPAD_RIGHT:
        return INPUT_ROLE_RIGHT;
    default: {
        const int32_t def_idx = M_POS_TO_DEF(position);
        if (def_idx >= 1 && def_idx < M_NUM_BUTTON_DEFS) {
            return m_ButtonDefs[def_idx].role;
        }
        return (INPUT_ROLE)-1;
    }
    }
}

void TouchOverlay_EnterSelectionMode(void)
{
    m_SelectionMode = true;
    m_SelectedPosition = -1;
    m_SelectionFingerCount = 0;
    m_WasVisibleBeforeSelection = m_Visible;
    m_Visible = true;
    M_ComputeButtonLayout();
}

void TouchOverlay_ExitSelectionMode(void)
{
    m_SelectionMode = false;
    m_SelectedPosition = -1;
    m_Visible = m_WasVisibleBeforeSelection;
}

int32_t TouchOverlay_GetSelectedPosition(void)
{
    const int32_t pos = m_SelectedPosition;
    m_SelectedPosition = -1;
    return pos;
}
