#pragma once

#include <trx/config/enum.h>
#include <trx/core/colors.h>
#include <trx/core/math/types.h>
#include <trx/game/objects/ids.h>
#include <trx/game/output/draw.h>

typedef struct {
    RGBA_8888 background[2]; // TS_BACKGROUND: [top, bottom]
    RGBA_8888 background_heavy[2]; // TS_BACKGROUND_HEAVY: [top, bottom]
    RGBA_8888 outline_light;
    RGBA_8888 outline_dark;
} UI_MENU_COLORS_PC;

typedef struct {
    RGBA_8888 background_edge;
    RGBA_8888 background_center;
    RGBA_8888 background_heavy_edge;
    RGBA_8888 background_heavy_center;
    RGBA_8888 heading_edge;
    RGBA_8888 heading_center;
    RGBA_8888 requested_edge;
    RGBA_8888 requested_center;
    RGBA_8888 requested_outline_ch;
    RGBA_8888 requested_outline_cv;
    RGBA_8888 requested_outline_edge;
    RGBA_8888 outline_tl;
    RGBA_8888 outline_tr;
    RGBA_8888 outline_bl;
    RGBA_8888 outline_br;
    RGBA_8888 heading_outline;
} UI_MENU_COLORS_PS1;

// The colors of the game's menus in one style, which the UI_STYLE passed
// alongside them selects.
typedef union {
    UI_MENU_COLORS_PC pc;
    UI_MENU_COLORS_PS1 ps1;
} UI_MENU_COLORS;

// Schedule deferred UI draw operations to be executed by UI_Draw().
// These record drawing commands during UI_EndScene instead of issuing them
// immediately.
void UI_ScheduleDrawTextBackground(
    UI_STYLE ui_style, const UI_MENU_COLORS *colors, int32_t sx, int32_t sy,
    int32_t z, int32_t w, int32_t h, TEXT_STYLE text_style);
void UI_ScheduleDrawTextOutline(
    UI_STYLE ui_style, const UI_MENU_COLORS *colors, int32_t sx, int32_t sy,
    int32_t z, int32_t w, int32_t h, TEXT_STYLE text_style);
void UI_ScheduleDrawScreenSprite(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprite_idx, const RGBA_F colors[4]);
// Draw an already resolved image file in a screen box beneath all other canvas
// content.
void UI_ScheduleDrawImage(
    const char *path, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
    float opacity);
// Covers the whole screen in black, from 0 for clear to 1 for black.
void UI_ScheduleDrawScreenBlack(float opacity);
// Draws through the overlay between what was scheduled before it and what is
// scheduled after it. `draw` receives a copy of `args`.
void UI_ScheduleDrawOverlay(
    void (*draw)(const void *args), const void *args, size_t args_size);
// Draws the overlay's snapshot over the whole screen.
void UI_ScheduleDrawSnapshot(float opacity);
void UI_ScheduleDrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, RGBA_8888 color);
void UI_ScheduleDrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, RGBA_8888 tl,
    RGBA_8888 tr, RGBA_8888 bl, RGBA_8888 br);
void UI_ScheduleDrawHorizontalLine(
    UI_STYLE ui_style, const UI_MENU_COLORS *colors, int32_t x0, int32_t x1,
    int32_t y, int32_t z);
void UI_ScheduleDrawScreenCircle(
    int32_t cx, int32_t cy, int32_t r_inner, int32_t r_outer, int32_t z,
    RGBA_8888 color);
// Draws a sprite stretched over a box, ignoring the edges the sprite declares.
void UI_ScheduleDrawScreenSpriteRect(
    int32_t sprite_idx, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
    int32_t z, RGBA_F color);
void UI_ScheduleDrawScreenLine(
    float x0, float y0, float x1, float y1, float width, int32_t z,
    RGBA_8888 color);

// The light a model is drawn in: an ambient light, up to three lights shining
// from a direction in a colour of their own, and a colour some of the meshes,
// one bit each, take from every light in place of the light's own. A direction
// runs across, down and away from the viewer, and points to where the light
// comes from.
#define UI_MODEL_LIGHTS 3
typedef struct {
    RGB_F ambient;
    int32_t light_count;
    struct {
        XYZ_32 dir;
        RGB_F color;
    } lights[UI_MODEL_LIGHTS];
    uint32_t lit_mask;
    RGB_F colors[32];
} UI_MODEL_LIGHT;

// Draws an object's model fitted into a screen box, turned by the rotation and
// lit by the light given.
// A held model is first tilted and turned as the game holds up a pickup; one
// that is not stands as it was built. Either is tilted towards the viewer by
// the tilt first.
// It stands between what was scheduled before it and what is scheduled after
// it. An object the level did not load, or one with no model, draws nothing.
void UI_ScheduleDrawModel(
    OBJECT_ID object_id, uint32_t mesh_mask, int32_t sx, int32_t sy, int32_t w,
    int32_t h, XYZ_16 rot, bool held, int16_t tilt,
    const UI_MODEL_LIGHT *light);

// Where a model stands in a world of its own, the camera that sees it, and the
// frame it is posed in. The model is placed by turns, moves and scales applied
// in order from the world's origin, as the engine places one, and seen through
// the camera's field of view.
#define UI_MODEL_STEPS 16
#define UI_MODEL_BONES 8
typedef enum {
    UI_MODEL_STEP_MOVE,
    UI_MODEL_STEP_TURN,
    // Scales the model by vec.x, in 1/1024ths.
    UI_MODEL_STEP_SCALE,
} UI_MODEL_STEP;

typedef struct {
    XYZ_32 camera_pos;
    XYZ_32 camera_target;
    int16_t fov;
    int32_t step_count;
    struct {
        UI_MODEL_STEP kind;
        XYZ_32 vec;
    } steps[UI_MODEL_STEPS];
    // The frame of its first animation that the model is posed in, and the
    // frame it blends towards by the blend, from 0 to 1.
    int32_t frame;
    int32_t next_frame;
    float blend;
    // Turns that bones take on top of the frame, about the axes each names.
    int32_t bone_count;
    struct {
        int32_t bone;
        XYZ_BOOL axes;
        XYZ_16 rot;
    } bones[UI_MODEL_BONES];
    // Whether the model is drawn about its pivot, without its frame offset.
    bool at_pivot;
} UI_MODEL_WORLD;

// Draws an object's model where it stands in a world of its own, as that
// world's camera sees it over the whole screen, lit by the light given with
// its directions in that world. It stands in the scene behind the whole
// interface, over what went into the scene before it.
void UI_ScheduleDrawModelInWorld(
    OBJECT_ID object_id, uint32_t mesh_mask, const UI_MODEL_WORLD *world,
    const UI_MODEL_LIGHT *light);
// Draws an image behind the whole interface as the engine draws its
// backgrounds, fitted to the screen's shape and, where mono, without its
// colour, then darkens it by the dim. Without an image the screen behind the
// interface is black.
void UI_ScheduleDrawBackground(const char *path, bool mono, float dim);
// Draws a backdrop behind the whole interface as the engine draws the ones
// behind its menus, in a style, by how far it has come in, from 0 to 1. A
// picture takes the path of its image.
void UI_ScheduleDrawBackdrop(
    BACKGROUND_TYPE style, float progress, const char *path);
// Darkens the scene behind the whole interface by the opacity, as the engine
// darkens it behind a screen and fades it to black.
void UI_ScheduleDrawDarken(float opacity);
// Draws a snapshot slot of the overlay over the whole screen, between what was
// scheduled before it and what is scheduled after it.
void UI_ScheduleDrawSnapshotSlot(int32_t slot, float opacity);

void UI_InitDraw(void);
void UI_ShutdownDraw(void);

// Execute all scheduled UI draw operations in order.
void UI_Draw(void);

// Draws the mask Lara looks through while she holds binoculars to her eyes,
// over a level that is being played.
void UI_DrawBinocularMask(void);
