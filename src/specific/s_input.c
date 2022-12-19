#include "specific/s_input.h"

#include "game/input.h"
#include "log.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_stdinc.h>
#include <stdbool.h>
#include <stddef.h>

#define KEY_DOWN(a) (m_KeyboardState[(a)])

typedef union BIND_TYPE {
    SDL_GameControllerButton button;
    SDL_GameControllerAxis axis;
} BIND_TYPE;

typedef struct CONTROLLER_MAP {
    BUTTON_TYPE type;
    BIND_TYPE bind;
    int16_t axis_dir;
} CONTROLLER_MAP;

static INPUT_SCANCODE m_Layout[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF] = {
    // clang-format off
    // built-in controls
    {
        SDL_SCANCODE_UP,         // INPUT_ROLE_UP
        SDL_SCANCODE_DOWN,       // INPUT_ROLE_DOWN
        SDL_SCANCODE_LEFT,       // INPUT_ROLE_LEFT
        SDL_SCANCODE_RIGHT,      // INPUT_ROLE_RIGHT
        SDL_SCANCODE_DELETE,     // INPUT_ROLE_STEP_L
        SDL_SCANCODE_PAGEDOWN,   // INPUT_ROLE_STEP_R
        SDL_SCANCODE_RSHIFT,     // INPUT_ROLE_SLOW
        SDL_SCANCODE_RALT,       // INPUT_ROLE_JUMP
        SDL_SCANCODE_RCTRL,      // INPUT_ROLE_ACTION
        SDL_SCANCODE_SPACE,      // INPUT_ROLE_DRAW
        SDL_SCANCODE_KP_0,       // INPUT_ROLE_LOOK
        SDL_SCANCODE_END,        // INPUT_ROLE_ROLL
        SDL_SCANCODE_ESCAPE,     // INPUT_ROLE_OPTION
        SDL_SCANCODE_O,          // INPUT_ROLE_FLY_CHEAT,
        SDL_SCANCODE_I,          // INPUT_ROLE_ITEM_CHEAT,
        SDL_SCANCODE_L,          // INPUT_ROLE_LEVEL_SKIP_CHEAT,
        SDL_SCANCODE_TAB,        // INPUT_ROLE_TURBO_CHEAT,
        SDL_SCANCODE_P,          // INPUT_ROLE_PAUSE,
        SDL_SCANCODE_W,          // INPUT_ROLE_CAMERA_UP
        SDL_SCANCODE_S,          // INPUT_ROLE_CAMERA_DOWN
        SDL_SCANCODE_A,          // INPUT_ROLE_CAMERA_LEFT
        SDL_SCANCODE_D,          // INPUT_ROLE_CAMERA_RIGHT
        SDL_SCANCODE_SLASH,      // INPUT_ROLE_CAMERA_RESET
        SDL_SCANCODE_1,          // INPUT_ROLE_EQUIP_PISTOLS
        SDL_SCANCODE_2,          // INPUT_ROLE_EQUIP_SHOTGUN
        SDL_SCANCODE_3,          // INPUT_ROLE_EQUIP_MAGNUMS
        SDL_SCANCODE_4,          // INPUT_ROLE_EQUIP_UZIS
        SDL_SCANCODE_8,          // INPUT_ROLE_USE_SMALL_MEDI
        SDL_SCANCODE_9,          // INPUT_ROLE_USE_BIG_MEDI
        SDL_SCANCODE_F5,         // INPUT_ROLE_SAVE
        SDL_SCANCODE_F6,         // INPUT_ROLE_LOAD
        SDL_SCANCODE_F2,         // INPUT_ROLE_FPS
        SDL_SCANCODE_F3,         // INPUT_ROLE_BILINEAR
    },

    // custom user controls
    {
        SDL_SCANCODE_UP,         // INPUT_ROLE_UP
        SDL_SCANCODE_DOWN,       // INPUT_ROLE_DOWN
        SDL_SCANCODE_LEFT,       // INPUT_ROLE_LEFT
        SDL_SCANCODE_RIGHT,      // INPUT_ROLE_RIGHT
        SDL_SCANCODE_DELETE,     // INPUT_ROLE_STEP_L
        SDL_SCANCODE_PAGEDOWN,   // INPUT_ROLE_STEP_R
        SDL_SCANCODE_RSHIFT,     // INPUT_ROLE_SLOW
        SDL_SCANCODE_RALT,       // INPUT_ROLE_JUMP
        SDL_SCANCODE_RCTRL,      // INPUT_ROLE_ACTION
        SDL_SCANCODE_SPACE,      // INPUT_ROLE_DRAW
        SDL_SCANCODE_KP_0,       // INPUT_ROLE_LOOK
        SDL_SCANCODE_END,        // INPUT_ROLE_ROLL
        SDL_SCANCODE_ESCAPE,     // INPUT_ROLE_OPTION
        SDL_SCANCODE_O,          // INPUT_ROLE_FLY_CHEAT,
        SDL_SCANCODE_I,          // INPUT_ROLE_ITEM_CHEAT,
        SDL_SCANCODE_L,          // INPUT_ROLE_LEVEL_SKIP_CHEAT,
        SDL_SCANCODE_TAB,        // INPUT_ROLE_TURBO_CHEAT,
        SDL_SCANCODE_P,          // INPUT_ROLE_PAUSE,
        SDL_SCANCODE_W,          // INPUT_ROLE_CAMERA_UP
        SDL_SCANCODE_S,          // INPUT_ROLE_CAMERA_DOWN
        SDL_SCANCODE_A,          // INPUT_ROLE_CAMERA_LEFT
        SDL_SCANCODE_D,          // INPUT_ROLE_CAMERA_RIGHT
        SDL_SCANCODE_SLASH,      // INPUT_ROLE_CAMERA_RESET
        SDL_SCANCODE_1,          // INPUT_ROLE_EQUIP_PISTOLS
        SDL_SCANCODE_2,          // INPUT_ROLE_EQUIP_SHOTGUN
        SDL_SCANCODE_3,          // INPUT_ROLE_EQUIP_MAGNUMS
        SDL_SCANCODE_4,          // INPUT_ROLE_EQUIP_UZIS
        SDL_SCANCODE_8,          // INPUT_ROLE_USE_SMALL_MEDI
        SDL_SCANCODE_9,          // INPUT_ROLE_USE_BIG_MEDI
        SDL_SCANCODE_F5,         // INPUT_ROLE_SAVE
        SDL_SCANCODE_F6,         // INPUT_ROLE_LOAD
        SDL_SCANCODE_F2,         // INPUT_ROLE_FPS
        SDL_SCANCODE_F3,         // INPUT_ROLE_BILINEAR
    },

    {
        SDL_SCANCODE_UP,         // INPUT_ROLE_UP
        SDL_SCANCODE_DOWN,       // INPUT_ROLE_DOWN
        SDL_SCANCODE_LEFT,       // INPUT_ROLE_LEFT
        SDL_SCANCODE_RIGHT,      // INPUT_ROLE_RIGHT
        SDL_SCANCODE_DELETE,     // INPUT_ROLE_STEP_L
        SDL_SCANCODE_PAGEDOWN,   // INPUT_ROLE_STEP_R
        SDL_SCANCODE_RSHIFT,     // INPUT_ROLE_SLOW
        SDL_SCANCODE_RALT,       // INPUT_ROLE_JUMP
        SDL_SCANCODE_RCTRL,      // INPUT_ROLE_ACTION
        SDL_SCANCODE_SPACE,      // INPUT_ROLE_DRAW
        SDL_SCANCODE_KP_0,       // INPUT_ROLE_LOOK
        SDL_SCANCODE_END,        // INPUT_ROLE_ROLL
        SDL_SCANCODE_ESCAPE,     // INPUT_ROLE_OPTION
        SDL_SCANCODE_O,          // INPUT_ROLE_FLY_CHEAT,
        SDL_SCANCODE_I,          // INPUT_ROLE_ITEM_CHEAT,
        SDL_SCANCODE_L,          // INPUT_ROLE_LEVEL_SKIP_CHEAT,
        SDL_SCANCODE_TAB,        // INPUT_ROLE_TURBO_CHEAT,
        SDL_SCANCODE_P,          // INPUT_ROLE_PAUSE,
        SDL_SCANCODE_W,          // INPUT_ROLE_CAMERA_UP
        SDL_SCANCODE_S,          // INPUT_ROLE_CAMERA_DOWN
        SDL_SCANCODE_A,          // INPUT_ROLE_CAMERA_LEFT
        SDL_SCANCODE_D,          // INPUT_ROLE_CAMERA_RIGHT
        SDL_SCANCODE_SLASH,      // INPUT_ROLE_CAMERA_RESET
        SDL_SCANCODE_1,          // INPUT_ROLE_EQUIP_PISTOLS
        SDL_SCANCODE_2,          // INPUT_ROLE_EQUIP_SHOTGUN
        SDL_SCANCODE_3,          // INPUT_ROLE_EQUIP_MAGNUMS
        SDL_SCANCODE_4,          // INPUT_ROLE_EQUIP_UZIS
        SDL_SCANCODE_8,          // INPUT_ROLE_USE_SMALL_MEDI
        SDL_SCANCODE_9,          // INPUT_ROLE_USE_BIG_MEDI
        SDL_SCANCODE_F5,         // INPUT_ROLE_SAVE
        SDL_SCANCODE_F6,         // INPUT_ROLE_LOAD
        SDL_SCANCODE_F2,         // INPUT_ROLE_FPS
        SDL_SCANCODE_F3,         // INPUT_ROLE_BILINEAR
    },

    {
        SDL_SCANCODE_UP,         // INPUT_ROLE_UP
        SDL_SCANCODE_DOWN,       // INPUT_ROLE_DOWN
        SDL_SCANCODE_LEFT,       // INPUT_ROLE_LEFT
        SDL_SCANCODE_RIGHT,      // INPUT_ROLE_RIGHT
        SDL_SCANCODE_DELETE,     // INPUT_ROLE_STEP_L
        SDL_SCANCODE_PAGEDOWN,   // INPUT_ROLE_STEP_R
        SDL_SCANCODE_RSHIFT,     // INPUT_ROLE_SLOW
        SDL_SCANCODE_RALT,       // INPUT_ROLE_JUMP
        SDL_SCANCODE_RCTRL,      // INPUT_ROLE_ACTION
        SDL_SCANCODE_SPACE,      // INPUT_ROLE_DRAW
        SDL_SCANCODE_KP_0,       // INPUT_ROLE_LOOK
        SDL_SCANCODE_END,        // INPUT_ROLE_ROLL
        SDL_SCANCODE_ESCAPE,     // INPUT_ROLE_OPTION
        SDL_SCANCODE_O,          // INPUT_ROLE_FLY_CHEAT,
        SDL_SCANCODE_I,          // INPUT_ROLE_ITEM_CHEAT,
        SDL_SCANCODE_L,          // INPUT_ROLE_LEVEL_SKIP_CHEAT,
        SDL_SCANCODE_TAB,        // INPUT_ROLE_TURBO_CHEAT,
        SDL_SCANCODE_P,          // INPUT_ROLE_PAUSE,
        SDL_SCANCODE_W,          // INPUT_ROLE_CAMERA_UP
        SDL_SCANCODE_S,          // INPUT_ROLE_CAMERA_DOWN
        SDL_SCANCODE_A,          // INPUT_ROLE_CAMERA_LEFT
        SDL_SCANCODE_D,          // INPUT_ROLE_CAMERA_RIGHT
        SDL_SCANCODE_SLASH,      // INPUT_ROLE_CAMERA_RESET
        SDL_SCANCODE_1,          // INPUT_ROLE_EQUIP_PISTOLS
        SDL_SCANCODE_2,          // INPUT_ROLE_EQUIP_SHOTGUN
        SDL_SCANCODE_3,          // INPUT_ROLE_EQUIP_MAGNUMS
        SDL_SCANCODE_4,          // INPUT_ROLE_EQUIP_UZIS
        SDL_SCANCODE_8,          // INPUT_ROLE_USE_SMALL_MEDI
        SDL_SCANCODE_9,          // INPUT_ROLE_USE_BIG_MEDI
        SDL_SCANCODE_F5,         // INPUT_ROLE_SAVE
        SDL_SCANCODE_F6,         // INPUT_ROLE_LOAD
        SDL_SCANCODE_F2,         // INPUT_ROLE_FPS
        SDL_SCANCODE_F3,         // INPUT_ROLE_BILINEAR
    }
    // clang-format on
};

static CONTROLLER_MAP
    m_ControllerLayout[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF] = {
        // clang-format off
    {
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_UP}, 0 },        // INPUT_ROLE_UP
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_DOWN}, 0 },      // INPUT_ROLE_DOWN
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_LEFT}, 0 },      // INPUT_ROLE_LEFT
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT}, 0 },     // INPUT_ROLE_RIGHT
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_TRIGGERLEFT}, 1 },      // INPUT_ROLE_STEP_L
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_TRIGGERRIGHT}, 1 },     // INPUT_ROLE_STEP_R
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER}, 0 },  // INPUT_ROLE_SLOW
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_X}, 0 },              // INPUT_ROLE_JUMP
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_A}, 0 },              // INPUT_ROLE_ACTION
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_Y}, 0 },              // INPUT_ROLE_DRAW
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_LEFTSHOULDER}, 0 },   // INPUT_ROLE_LOOK
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_B}, 0 },              // INPUT_ROLE_ROLL
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_BACK}, 0 },           // INPUT_ROLE_OPTION
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_FLY_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_ITEM_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_LEVEL_SKIP_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_TURBO_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_START}, 0 },          // INPUT_ROLE_PAUSE,
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTY}, -1 },          // INPUT_ROLE_CAMERA_UP
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTY}, 1 },           // INPUT_ROLE_CAMERA_DOWN
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTX}, -1 },          // INPUT_ROLE_CAMERA_LEFT
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTX}, 1 },           // INPUT_ROLE_CAMERA_RIGHT
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_RIGHTSTICK}, 0 },     // INPUT_ROLE_CAMERA_RESET
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_PISTOLS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_SHOTGUN
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_MAGNUMS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_UZIS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_USE_SMALL_MEDI
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_USE_BIG_MEDI
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_SAVE
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_LOAD
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_FPS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_BILINEAR
    },

    {
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_UP}, 0 },        // INPUT_ROLE_UP
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_DOWN}, 0 },      // INPUT_ROLE_DOWN
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_LEFT}, 0 },      // INPUT_ROLE_LEFT
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT}, 0 },     // INPUT_ROLE_RIGHT
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_TRIGGERLEFT}, 1 },      // INPUT_ROLE_STEP_L
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_TRIGGERRIGHT}, 1 },     // INPUT_ROLE_STEP_R
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER}, 0 },  // INPUT_ROLE_SLOW
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_X}, 0 },              // INPUT_ROLE_JUMP
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_A}, 0 },              // INPUT_ROLE_ACTION
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_Y}, 0 },              // INPUT_ROLE_DRAW
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_LEFTSHOULDER}, 0 },   // INPUT_ROLE_LOOK
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_B}, 0 },              // INPUT_ROLE_ROLL
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_BACK}, 0 },           // INPUT_ROLE_OPTION
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_FLY_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_ITEM_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_LEVEL_SKIP_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_TURBO_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_START}, 0 },          // INPUT_ROLE_PAUSE,
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTY}, -1 },          // INPUT_ROLE_CAMERA_UP
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTY}, 1 },           // INPUT_ROLE_CAMERA_DOWN
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTX}, -1 },          // INPUT_ROLE_CAMERA_LEFT
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTX}, 1 },           // INPUT_ROLE_CAMERA_RIGHT
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_RIGHTSTICK}, 0 },     // INPUT_ROLE_CAMERA_RESET
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_PISTOLS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_SHOTGUN
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_MAGNUMS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_UZIS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_USE_SMALL_MEDI
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_USE_BIG_MEDI
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_SAVE
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_LOAD
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_FPS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_BILINEAR
    },

    {
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_UP}, 0 },        // INPUT_ROLE_UP
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_DOWN}, 0 },      // INPUT_ROLE_DOWN
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_LEFT}, 0 },      // INPUT_ROLE_LEFT
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT}, 0 },     // INPUT_ROLE_RIGHT
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_TRIGGERLEFT}, 1 },      // INPUT_ROLE_STEP_L
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_TRIGGERRIGHT}, 1 },     // INPUT_ROLE_STEP_R
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER}, 0 },  // INPUT_ROLE_SLOW
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_X}, 0 },              // INPUT_ROLE_JUMP
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_A}, 0 },              // INPUT_ROLE_ACTION
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_Y}, 0 },              // INPUT_ROLE_DRAW
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_LEFTSHOULDER}, 0 },   // INPUT_ROLE_LOOK
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_B}, 0 },              // INPUT_ROLE_ROLL
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_BACK}, 0 },           // INPUT_ROLE_OPTION
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_FLY_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_ITEM_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_LEVEL_SKIP_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_TURBO_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_START}, 0 },          // INPUT_ROLE_PAUSE,
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTY}, -1 },          // INPUT_ROLE_CAMERA_UP
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTY}, 1 },           // INPUT_ROLE_CAMERA_DOWN
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTX}, -1 },          // INPUT_ROLE_CAMERA_LEFT
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTX}, 1 },           // INPUT_ROLE_CAMERA_RIGHT
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_RIGHTSTICK}, 0 },     // INPUT_ROLE_CAMERA_RESET
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_PISTOLS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_SHOTGUN
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_MAGNUMS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_UZIS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_USE_SMALL_MEDI
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_USE_BIG_MEDI
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_SAVE
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_LOAD
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_FPS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_BILINEAR
    },

    {
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_UP}, 0 },        // INPUT_ROLE_UP
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_DOWN}, 0 },      // INPUT_ROLE_DOWN
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_LEFT}, 0 },      // INPUT_ROLE_LEFT
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT}, 0 },     // INPUT_ROLE_RIGHT
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_TRIGGERLEFT}, 1 },      // INPUT_ROLE_STEP_L
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_TRIGGERRIGHT}, 1 },     // INPUT_ROLE_STEP_R
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER}, 0 },  // INPUT_ROLE_SLOW
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_X}, 0 },              // INPUT_ROLE_JUMP
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_A}, 0 },              // INPUT_ROLE_ACTION
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_Y}, 0 },              // INPUT_ROLE_DRAW
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_LEFTSHOULDER}, 0 },   // INPUT_ROLE_LOOK
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_B}, 0 },              // INPUT_ROLE_ROLL
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_BACK}, 0 },           // INPUT_ROLE_OPTION
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_FLY_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_ITEM_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_LEVEL_SKIP_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_TURBO_CHEAT,
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_START}, 0 },          // INPUT_ROLE_PAUSE,
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTY}, -1 },          // INPUT_ROLE_CAMERA_UP
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTY}, 1 },           // INPUT_ROLE_CAMERA_DOWN
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTX}, -1 },          // INPUT_ROLE_CAMERA_LEFT
    { BT_AXIS,   {SDL_CONTROLLER_AXIS_RIGHTX}, 1 },           // INPUT_ROLE_CAMERA_RIGHT
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_RIGHTSTICK}, 0 },     // INPUT_ROLE_CAMERA_RESET
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_PISTOLS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_SHOTGUN
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_MAGNUMS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_EQUIP_UZIS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_USE_SMALL_MEDI
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_USE_BIG_MEDI
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_SAVE
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_LOAD
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_FPS
    { BT_BUTTON, {SDL_CONTROLLER_BUTTON_INVALID}, 0 },        // INPUT_ROLE_BILINEAR
    }
        // clang-format on
    };

const Uint8 *m_KeyboardState;
static SDL_GameController *m_Controller = NULL;
static const char *m_ControllerName = NULL;
static SDL_GameControllerType m_ControllerType = SDL_CONTROLLER_TYPE_UNKNOWN;

static const char *S_Input_GetScancodeName(INPUT_SCANCODE scancode);
static const char *S_Input_GetButtonName(SDL_GameControllerButton button);
static const char *S_Input_GetAxisName(
    SDL_GameControllerAxis axis, int16_t axis_dir);
static bool S_Input_KbdKey(INPUT_ROLE role, INPUT_LAYOUT layout);
static bool S_Input_Key(INPUT_ROLE role, INPUT_LAYOUT layout_num);
static bool S_Input_JoyBtn(SDL_GameControllerButton button);
static int16_t S_Input_JoyAxis(SDL_GameControllerAxis axis);
static INPUT_STATE S_Input_GetControllerState(INPUT_STATE state, INPUT_LAYOUT cntlr_layout_num);


static const char *S_Input_GetScancodeName(INPUT_SCANCODE scancode)
{
    // clang-format off
    switch (scancode) {
        case SDL_SCANCODE_LCTRL:              return "CTRL";
        case SDL_SCANCODE_RCTRL:              return "CTRL";
        case SDL_SCANCODE_RSHIFT:             return "SHIFT";
        case SDL_SCANCODE_LSHIFT:             return "SHIFT";
        case SDL_SCANCODE_RALT:               return "ALT";
        case SDL_SCANCODE_LALT:               return "ALT";
        case SDL_SCANCODE_LGUI:               return "WIN";
        case SDL_SCANCODE_RGUI:               return "WIN";

        case SDL_SCANCODE_LEFT:               return "LEFT";
        case SDL_SCANCODE_UP:                 return "UP";
        case SDL_SCANCODE_RIGHT:              return "RIGHT";
        case SDL_SCANCODE_DOWN:               return "DOWN";

        case SDL_SCANCODE_RETURN:             return "RET";
        case SDL_SCANCODE_ESCAPE:             return "ESC";
        case SDL_SCANCODE_BACKSPACE:          return "BKSP";
        case SDL_SCANCODE_TAB:                return "TAB";
        case SDL_SCANCODE_SPACE:              return "SPACE";
        case SDL_SCANCODE_CAPSLOCK:           return "CAPS";
        case SDL_SCANCODE_PRINTSCREEN:        return "PSCRN";
        case SDL_SCANCODE_SCROLLLOCK:         return "SCLK";
        case SDL_SCANCODE_PAUSE:              return "PAUSE";
        case SDL_SCANCODE_INSERT:             return "INS";
        case SDL_SCANCODE_HOME:               return "HOME";
        case SDL_SCANCODE_PAGEUP:             return "PGUP";
        case SDL_SCANCODE_DELETE:             return "DEL";
        case SDL_SCANCODE_END:                return "END";
        case SDL_SCANCODE_PAGEDOWN:           return "PGDN";

        case SDL_SCANCODE_A:                  return "A";
        case SDL_SCANCODE_B:                  return "B";
        case SDL_SCANCODE_C:                  return "C";
        case SDL_SCANCODE_D:                  return "D";
        case SDL_SCANCODE_E:                  return "E";
        case SDL_SCANCODE_F:                  return "F";
        case SDL_SCANCODE_G:                  return "G";
        case SDL_SCANCODE_H:                  return "H";
        case SDL_SCANCODE_I:                  return "I";
        case SDL_SCANCODE_J:                  return "J";
        case SDL_SCANCODE_K:                  return "K";
        case SDL_SCANCODE_L:                  return "L";
        case SDL_SCANCODE_M:                  return "M";
        case SDL_SCANCODE_N:                  return "N";
        case SDL_SCANCODE_O:                  return "O";
        case SDL_SCANCODE_P:                  return "P";
        case SDL_SCANCODE_Q:                  return "Q";
        case SDL_SCANCODE_R:                  return "R";
        case SDL_SCANCODE_S:                  return "S";
        case SDL_SCANCODE_T:                  return "T";
        case SDL_SCANCODE_U:                  return "U";
        case SDL_SCANCODE_V:                  return "V";
        case SDL_SCANCODE_W:                  return "W";
        case SDL_SCANCODE_X:                  return "X";
        case SDL_SCANCODE_Y:                  return "Y";
        case SDL_SCANCODE_Z:                  return "Z";

        case SDL_SCANCODE_0:                  return "0";
        case SDL_SCANCODE_1:                  return "1";
        case SDL_SCANCODE_2:                  return "2";
        case SDL_SCANCODE_3:                  return "3";
        case SDL_SCANCODE_4:                  return "4";
        case SDL_SCANCODE_5:                  return "5";
        case SDL_SCANCODE_6:                  return "6";
        case SDL_SCANCODE_7:                  return "7";
        case SDL_SCANCODE_8:                  return "8";
        case SDL_SCANCODE_9:                  return "9";

        case SDL_SCANCODE_MINUS:              return "-";
        case SDL_SCANCODE_EQUALS:             return "=";
        case SDL_SCANCODE_LEFTBRACKET:        return "[";
        case SDL_SCANCODE_RIGHTBRACKET:       return "]";
        case SDL_SCANCODE_BACKSLASH:          return "\\";
        case SDL_SCANCODE_NONUSHASH:          return "#";
        case SDL_SCANCODE_SEMICOLON:          return ";";
        case SDL_SCANCODE_APOSTROPHE:         return "'";
        case SDL_SCANCODE_GRAVE:              return "`";
        case SDL_SCANCODE_COMMA:              return ",";
        case SDL_SCANCODE_PERIOD:             return ".";
        case SDL_SCANCODE_SLASH:              return "/";

        case SDL_SCANCODE_F1:                 return "F1";
        case SDL_SCANCODE_F2:                 return "F2";
        case SDL_SCANCODE_F3:                 return "F3";
        case SDL_SCANCODE_F4:                 return "F4";
        case SDL_SCANCODE_F5:                 return "F5";
        case SDL_SCANCODE_F6:                 return "F6";
        case SDL_SCANCODE_F7:                 return "F7";
        case SDL_SCANCODE_F8:                 return "F8";
        case SDL_SCANCODE_F9:                 return "F9";
        case SDL_SCANCODE_F10:                return "F10";
        case SDL_SCANCODE_F11:                return "F11";
        case SDL_SCANCODE_F12:                return "F12";
        case SDL_SCANCODE_F13:                return "F13";
        case SDL_SCANCODE_F14:                return "F14";
        case SDL_SCANCODE_F15:                return "F15";
        case SDL_SCANCODE_F16:                return "F16";
        case SDL_SCANCODE_F17:                return "F17";
        case SDL_SCANCODE_F18:                return "F18";
        case SDL_SCANCODE_F19:                return "F19";
        case SDL_SCANCODE_F20:                return "F20";
        case SDL_SCANCODE_F21:                return "F21";
        case SDL_SCANCODE_F22:                return "F22";
        case SDL_SCANCODE_F23:                return "F23";
        case SDL_SCANCODE_F24:                return "F24";

        case SDL_SCANCODE_NUMLOCKCLEAR:       return "NMLK";
        case SDL_SCANCODE_KP_0:               return "PAD0";
        case SDL_SCANCODE_KP_1:               return "PAD1";
        case SDL_SCANCODE_KP_2:               return "PAD2";
        case SDL_SCANCODE_KP_3:               return "PAD3";
        case SDL_SCANCODE_KP_4:               return "PAD4";
        case SDL_SCANCODE_KP_5:               return "PAD5";
        case SDL_SCANCODE_KP_6:               return "PAD6";
        case SDL_SCANCODE_KP_7:               return "PAD7";
        case SDL_SCANCODE_KP_8:               return "PAD8";
        case SDL_SCANCODE_KP_9:               return "PAD9";
        case SDL_SCANCODE_KP_PERIOD:          return "PAD.";
        case SDL_SCANCODE_KP_DIVIDE:          return "PAD/";
        case SDL_SCANCODE_KP_MULTIPLY:        return "PAD*";
        case SDL_SCANCODE_KP_MINUS:           return "PAD-";
        case SDL_SCANCODE_KP_PLUS:            return "PAD+";
        case SDL_SCANCODE_KP_EQUALS:          return "PAD=";
        case SDL_SCANCODE_KP_EQUALSAS400:     return "PAD=";
        case SDL_SCANCODE_KP_COMMA:           return "PAD,";
        case SDL_SCANCODE_KP_ENTER:           return "ENTER";

        // extra keys
        case SDL_SCANCODE_APPLICATION:        return "MENU";
        case SDL_SCANCODE_POWER:              return "POWER";
        case SDL_SCANCODE_EXECUTE:            return "EXEC";
        case SDL_SCANCODE_HELP:               return "HELP";
        case SDL_SCANCODE_MENU:               return "MENU";
        case SDL_SCANCODE_SELECT:             return "SEL";
        case SDL_SCANCODE_STOP:               return "STOP";
        case SDL_SCANCODE_AGAIN:              return "AGAIN";
        case SDL_SCANCODE_UNDO:               return "UNDO";
        case SDL_SCANCODE_CUT:                return "CUT";
        case SDL_SCANCODE_COPY:               return "COPY";
        case SDL_SCANCODE_PASTE:              return "PASTE";
        case SDL_SCANCODE_FIND:               return "FIND";
        case SDL_SCANCODE_MUTE:               return "MUTE";
        case SDL_SCANCODE_VOLUMEUP:           return "VOLUP";
        case SDL_SCANCODE_VOLUMEDOWN:         return "VOLDN";
        case SDL_SCANCODE_ALTERASE:           return "ALTER";
        case SDL_SCANCODE_SYSREQ:             return "SYSRQ";
        case SDL_SCANCODE_CANCEL:             return "CNCEL";
        case SDL_SCANCODE_CLEAR:              return "CLEAR";
        case SDL_SCANCODE_PRIOR:              return "PRIOR";
        case SDL_SCANCODE_RETURN2:            return "RETURN";
        case SDL_SCANCODE_SEPARATOR:          return "SEP";
        case SDL_SCANCODE_OUT:                return "OUT";
        case SDL_SCANCODE_OPER:               return "OPER";
        case SDL_SCANCODE_CLEARAGAIN:         return "CLEAR";
        case SDL_SCANCODE_CRSEL:              return "CRSEL";
        case SDL_SCANCODE_EXSEL:              return "EXSEL";
        case SDL_SCANCODE_KP_00:              return "PAD00";
        case SDL_SCANCODE_KP_000:             return "PAD000";
        case SDL_SCANCODE_THOUSANDSSEPARATOR: return "TSEP";
        case SDL_SCANCODE_DECIMALSEPARATOR:   return "DSEP";
        case SDL_SCANCODE_CURRENCYUNIT:       return "CURU";
        case SDL_SCANCODE_CURRENCYSUBUNIT:    return "CURSU";
        case SDL_SCANCODE_KP_LEFTPAREN:       return "PAD(";
        case SDL_SCANCODE_KP_RIGHTPAREN:      return "PAD)";
        case SDL_SCANCODE_KP_LEFTBRACE:       return "PAD{";
        case SDL_SCANCODE_KP_RIGHTBRACE:      return "PAD}";
        case SDL_SCANCODE_KP_TAB:             return "PADT";
        case SDL_SCANCODE_KP_BACKSPACE:       return "PADBK";
        case SDL_SCANCODE_KP_A:               return "PADA";
        case SDL_SCANCODE_KP_B:               return "PADB";
        case SDL_SCANCODE_KP_C:               return "PADC";
        case SDL_SCANCODE_KP_D:               return "PADD";
        case SDL_SCANCODE_KP_E:               return "PADE";
        case SDL_SCANCODE_KP_F:               return "PADF";
        case SDL_SCANCODE_KP_XOR:             return "PADXR";
        case SDL_SCANCODE_KP_POWER:           return "PAD^";
        case SDL_SCANCODE_KP_PERCENT:         return "PAD%";
        case SDL_SCANCODE_KP_LESS:            return "PAD<";
        case SDL_SCANCODE_KP_GREATER:         return "PAD>";
        case SDL_SCANCODE_KP_AMPERSAND:       return "PAD&";
        case SDL_SCANCODE_KP_DBLAMPERSAND:    return "PAD&&";
        case SDL_SCANCODE_KP_VERTICALBAR:     return "PAD|";
        case SDL_SCANCODE_KP_DBLVERTICALBAR:  return "PAD||";
        case SDL_SCANCODE_KP_COLON:           return "PAD:";
        case SDL_SCANCODE_KP_HASH:            return "PAD#";
        case SDL_SCANCODE_KP_SPACE:           return "PADSP";
        case SDL_SCANCODE_KP_AT:              return "PAD@";
        case SDL_SCANCODE_KP_EXCLAM:          return "PAD!";
        case SDL_SCANCODE_KP_MEMSTORE:        return "PADMS";
        case SDL_SCANCODE_KP_MEMRECALL:       return "PADMR";
        case SDL_SCANCODE_KP_MEMCLEAR:        return "PADMC";
        case SDL_SCANCODE_KP_MEMADD:          return "PADMA";
        case SDL_SCANCODE_KP_MEMSUBTRACT:     return "PADM-";
        case SDL_SCANCODE_KP_MEMMULTIPLY:     return "PADM*";
        case SDL_SCANCODE_KP_MEMDIVIDE:       return "PADM/";
        case SDL_SCANCODE_KP_PLUSMINUS:       return "PAD+-";
        case SDL_SCANCODE_KP_CLEAR:           return "PADCL";
        case SDL_SCANCODE_KP_CLEARENTRY:      return "PADCL";
        case SDL_SCANCODE_KP_BINARY:          return "PAD02";
        case SDL_SCANCODE_KP_OCTAL:           return "PAD08";
        case SDL_SCANCODE_KP_DECIMAL:         return "PAD10";
        case SDL_SCANCODE_KP_HEXADECIMAL:     return "PAD16";
        case SDL_SCANCODE_MODE:               return "MODE";
        case SDL_SCANCODE_AUDIONEXT:          return "NEXT";
        case SDL_SCANCODE_AUDIOPREV:          return "PREV";
        case SDL_SCANCODE_AUDIOSTOP:          return "STOP";
        case SDL_SCANCODE_AUDIOPLAY:          return "PLAY";
        case SDL_SCANCODE_AUDIOMUTE:          return "MUTE";
        case SDL_SCANCODE_MEDIASELECT:        return "MEDIA";
        case SDL_SCANCODE_WWW:                return "WWW";
        case SDL_SCANCODE_MAIL:               return "MAIL";
        case SDL_SCANCODE_CALCULATOR:         return "CALC";
        case SDL_SCANCODE_COMPUTER:           return "COMP";
        case SDL_SCANCODE_AC_SEARCH:          return "SRCH";
        case SDL_SCANCODE_AC_HOME:            return "HOME";
        case SDL_SCANCODE_AC_BACK:            return "BACK";
        case SDL_SCANCODE_AC_FORWARD:         return "FRWD";
        case SDL_SCANCODE_AC_STOP:            return "STOP";
        case SDL_SCANCODE_AC_REFRESH:         return "RFRSH";
        case SDL_SCANCODE_AC_BOOKMARKS:       return "BKMK";
        case SDL_SCANCODE_BRIGHTNESSDOWN:     return "BNDN";
        case SDL_SCANCODE_BRIGHTNESSUP:       return "BNUP";
        case SDL_SCANCODE_DISPLAYSWITCH:      return "DPSW";
        case SDL_SCANCODE_KBDILLUMTOGGLE:     return "KBDIT";
        case SDL_SCANCODE_KBDILLUMDOWN:       return "KBDID";
        case SDL_SCANCODE_KBDILLUMUP:         return "KBDIU";
        case SDL_SCANCODE_EJECT:              return "EJECT";
        case SDL_SCANCODE_SLEEP:              return "SLEEP";
        case SDL_SCANCODE_APP1:               return "APP1";
        case SDL_SCANCODE_APP2:               return "APP2";
        case SDL_SCANCODE_AUDIOREWIND:        return "RWND";
        case SDL_SCANCODE_AUDIOFASTFORWARD:   return "FF";
        case SDL_SCANCODE_UNKNOWN:            return "";
    }
    // clang-format on
    return "????";
}

static const char *S_Input_GetButtonName(SDL_GameControllerButton button)
{
    // clang-format off
    switch (m_ControllerType) {
        case SDL_CONTROLLER_TYPE_PS3:
        case SDL_CONTROLLER_TYPE_PS4:
        case SDL_CONTROLLER_TYPE_PS5:
            switch (button) {
                case SDL_CONTROLLER_BUTTON_INVALID:       return "";
                case SDL_CONTROLLER_BUTTON_A:             return "\206";
                case SDL_CONTROLLER_BUTTON_B:             return "\205";
                case SDL_CONTROLLER_BUTTON_X:             return "\207";
                case SDL_CONTROLLER_BUTTON_Y:             return "\204";
                case SDL_CONTROLLER_BUTTON_BACK:          return "CREATE";
                case SDL_CONTROLLER_BUTTON_GUIDE:         return "HOME"; /* Home button*/
                case SDL_CONTROLLER_BUTTON_START:         return "START";
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return "L3";
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return "R3";
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return "^";
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "_";
                case SDL_CONTROLLER_BUTTON_DPAD_UP:       return "\203";
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return "\202";
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return "\200";
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return "\201";
                case SDL_CONTROLLER_BUTTON_MISC1:         return "MIC"; /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button */
                case SDL_CONTROLLER_BUTTON_PADDLE1:       return "PADDLE 1"; /* Xbox Elite paddle P1 (upper left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE2:       return "PADDLE 2"; /* Xbox Elite paddle P3 (upper right, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE3:       return "PADDLE 3"; /* Xbox Elite paddle P2 (lower left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE4:       return "PADDLE 4"; /* Xbox Elite paddle P4 (lower right, facing the back) */
                case SDL_CONTROLLER_BUTTON_TOUCHPAD:      return "TOUCHPAD"; /* PS4/PS5 touchpad button */
                case SDL_CONTROLLER_BUTTON_MAX:           return "";
            }
            break;
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            switch (button) {
                case SDL_CONTROLLER_BUTTON_INVALID:       return "INVALID";
                case SDL_CONTROLLER_BUTTON_A:             return "B";
                case SDL_CONTROLLER_BUTTON_B:             return "A";
                case SDL_CONTROLLER_BUTTON_X:             return "Y";
                case SDL_CONTROLLER_BUTTON_Y:             return "X";
                case SDL_CONTROLLER_BUTTON_BACK:          return "BACK";
                case SDL_CONTROLLER_BUTTON_GUIDE:         return "HOME"; /* Home button*/
                case SDL_CONTROLLER_BUTTON_START:         return "START";
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return "L STICK";
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return "R STICK";
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return "L BUTTON";
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "R BUTTON";
                case SDL_CONTROLLER_BUTTON_DPAD_UP:       return "\203";
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return "\202";
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return "\200 ";
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return "\201 ";
                case SDL_CONTROLLER_BUTTON_MISC1:         return "CAPTURE"; /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button */
                case SDL_CONTROLLER_BUTTON_PADDLE1:       return "PADDLE 1"; /* Xbox Elite paddle P1 (upper left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE2:       return "PADDLE 2"; /* Xbox Elite paddle P3 (upper right, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE3:       return "PADDLE 3"; /* Xbox Elite paddle P2 (lower left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE4:       return "PADDLE 4"; /* Xbox Elite paddle P4 (lower right, facing the back) */
                case SDL_CONTROLLER_BUTTON_TOUCHPAD:      return "TOUCHPAD"; /* PS4/PS5 touchpad button */
                case SDL_CONTROLLER_BUTTON_MAX:           return "";
            }
            break;
        case SDL_CONTROLLER_TYPE_XBOX360:
        case SDL_CONTROLLER_TYPE_XBOXONE:
            switch (button) {
                case SDL_CONTROLLER_BUTTON_INVALID:       return "INVALID";
                case SDL_CONTROLLER_BUTTON_A:             return "A";
                case SDL_CONTROLLER_BUTTON_B:             return "B";
                case SDL_CONTROLLER_BUTTON_X:             return "X";
                case SDL_CONTROLLER_BUTTON_Y:             return "Y";
                case SDL_CONTROLLER_BUTTON_BACK:          return "BACK";
                case SDL_CONTROLLER_BUTTON_GUIDE:         return "XBOX"; /* Home button*/
                case SDL_CONTROLLER_BUTTON_START:         return "START";
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return "L STICK";
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return "R STICK";
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return "L BUMPER";
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "R BUMPER";
                case SDL_CONTROLLER_BUTTON_DPAD_UP:       return "\203";
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return "\202";
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return "\200 ";
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return "\201 ";
                case SDL_CONTROLLER_BUTTON_MISC1:         return "SHARE"; /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button */
                case SDL_CONTROLLER_BUTTON_PADDLE1:       return "PADDLE 1"; /* Xbox Elite paddle P1 (upper left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE2:       return "PADDLE 2"; /* Xbox Elite paddle P3 (upper right, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE3:       return "PADDLE 3"; /* Xbox Elite paddle P2 (lower left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE4:       return "PADDLE 4"; /* Xbox Elite paddle P4 (lower right, facing the back) */
                case SDL_CONTROLLER_BUTTON_TOUCHPAD:      return "TOUCHPAD"; /* PS4/PS5 touchpad button */
                case SDL_CONTROLLER_BUTTON_MAX:           return "";
            }
            break;
        default:
            switch (button) {
                case SDL_CONTROLLER_BUTTON_INVALID:       return "INVALID";
                case SDL_CONTROLLER_BUTTON_A:             return "A";
                case SDL_CONTROLLER_BUTTON_B:             return "B";
                case SDL_CONTROLLER_BUTTON_X:             return "X";
                case SDL_CONTROLLER_BUTTON_Y:             return "Y";
                case SDL_CONTROLLER_BUTTON_BACK:          return "BACK";
                case SDL_CONTROLLER_BUTTON_GUIDE:         return "HOME"; /* Home button*/
                case SDL_CONTROLLER_BUTTON_START:         return "START";
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return "L STICK";
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return "R STICK";
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return "L BUMPER";
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "R BUMPER";
                case SDL_CONTROLLER_BUTTON_DPAD_UP:       return "\203";
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return "\202";
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return "\200 ";
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return "\201 ";
                case SDL_CONTROLLER_BUTTON_MISC1:         return "SHARE"; /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button */
                case SDL_CONTROLLER_BUTTON_PADDLE1:       return "PADDLE 1"; /* Xbox Elite paddle P1 (upper left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE2:       return "PADDLE 2"; /* Xbox Elite paddle P3 (upper right, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE3:       return "PADDLE 3"; /* Xbox Elite paddle P2 (lower left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE4:       return "PADDLE 4"; /* Xbox Elite paddle P4 (lower right, facing the back) */
                case SDL_CONTROLLER_BUTTON_TOUCHPAD:      return "TOUCHPAD"; /* PS4/PS5 touchpad button */
                case SDL_CONTROLLER_BUTTON_MAX:           return "";
            }
            break;
    }
    // clang-format on
    return "????";
}

static const char *S_Input_GetAxisName(
    SDL_GameControllerAxis axis, int16_t axis_dir)
{
    // clang-format off
    switch (m_ControllerType) {
        case SDL_CONTROLLER_TYPE_PS3:
        case SDL_CONTROLLER_TYPE_PS4:
        case SDL_CONTROLLER_TYPE_PS5:
            switch (axis) {
                case SDL_CONTROLLER_AXIS_INVALID:         return "";
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? "L ANALOG LEFT" : "L ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? "L ANALOG UP" : "L ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? "R ANALOG LEFT" : "R ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? "R ANALOG UP" : "R ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return "\300";
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return "{";
                case SDL_CONTROLLER_AXIS_MAX:             return "";
            }
            break;
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            switch (axis) {
                case SDL_CONTROLLER_AXIS_INVALID:         return "";
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? "L ANALOG LEFT" : "L ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? "L ANALOG UP" : "L ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? "R ANALOG LEFT" : "R ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? "R ANALOG UP" : "R ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return "ZL";
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return "ZR";
                case SDL_CONTROLLER_AXIS_MAX:             return "";
            }
            break;
        case SDL_CONTROLLER_TYPE_XBOX360:
        case SDL_CONTROLLER_TYPE_XBOXONE:
        default:
            switch (axis) {
                case SDL_CONTROLLER_AXIS_INVALID:         return "";
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? "L ANALOG LEFT" : "L ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? "L ANALOG UP" : "L ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? "R ANALOG LEFT" : "R ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? "R ANALOG UP" : "R ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return "L TRIGGER";
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return "R TRIGGER";
                case SDL_CONTROLLER_AXIS_MAX:             return "";
            }
            break;
    }
    // clang-format on
    return "????";
}

static bool S_Input_KbdKey(INPUT_ROLE role, INPUT_LAYOUT layout_num)
{
    INPUT_SCANCODE scancode = m_Layout[layout_num][role];
    if (KEY_DOWN(scancode)) {
        return true;
    }
    if (scancode == SDL_SCANCODE_LCTRL) {
        return KEY_DOWN(SDL_SCANCODE_RCTRL);
    }
    if (scancode == SDL_SCANCODE_RCTRL) {
        return KEY_DOWN(SDL_SCANCODE_LCTRL);
    }
    if (scancode == SDL_SCANCODE_LSHIFT) {
        return KEY_DOWN(SDL_SCANCODE_RSHIFT);
    }
    if (scancode == SDL_SCANCODE_RSHIFT) {
        return KEY_DOWN(SDL_SCANCODE_LSHIFT);
    }
    if (scancode == SDL_SCANCODE_LALT) {
        return KEY_DOWN(SDL_SCANCODE_RALT);
    }
    if (scancode == SDL_SCANCODE_RALT) {
        return KEY_DOWN(SDL_SCANCODE_LALT);
    }
    return false;
}

static bool S_Input_Key(INPUT_ROLE role, INPUT_LAYOUT layout_num)
{
    return S_Input_KbdKey(role, layout_num);
}

static bool S_Input_JoyBtn(SDL_GameControllerButton button)
{
    return SDL_GameControllerGetButton(m_Controller, button);
}

static int16_t S_Input_JoyAxis(SDL_GameControllerAxis axis)
{
    Sint16 value = SDL_GameControllerGetAxis(m_Controller, axis);
    if (value < -SDL_JOYSTICK_AXIS_MAX / 2) {
        return -1;
    }
    if (value > SDL_JOYSTICK_AXIS_MAX / 2) {
        return 1;
    }
    return 0;
}

 static INPUT_STATE S_Input_GetControllerState(INPUT_STATE state, INPUT_LAYOUT cntlr_layout_num)
 {
    // clang-format off
    for (int role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        CONTROLLER_MAP assigned = m_ControllerLayout[cntlr_layout_num][role];
        int16_t btn_state = 0;

        if (assigned.type == BT_BUTTON) {
            btn_state = S_Input_JoyBtn(assigned.bind.button);
        } else {
            btn_state = S_Input_JoyAxis(assigned.bind.axis) == assigned.axis_dir;
        }

        switch(role) {
        case INPUT_ROLE_UP:
            state.forward                |= btn_state;
            break;
        case INPUT_ROLE_DOWN:
            state.back                   |= btn_state;
            break;
        case INPUT_ROLE_LEFT:
            state.left                   |= btn_state;
            break;
        case INPUT_ROLE_RIGHT:
            state.right                  |= btn_state;
            break;
        case INPUT_ROLE_STEP_L:
            state.step_left              |= btn_state;
            break;
        case INPUT_ROLE_STEP_R:
            state.step_right             |= btn_state;
            break;
        case INPUT_ROLE_SLOW:
            state.slow                   |= btn_state;
            break;
        case INPUT_ROLE_JUMP:
            state.jump                   |= btn_state;
            break;
        case INPUT_ROLE_ACTION:
            state.action                 |= btn_state;
            state.select                 |= btn_state;
            break;
        case INPUT_ROLE_DRAW:
            state.draw                   |= btn_state;
            break;
        case INPUT_ROLE_LOOK:
            state.look                   |= btn_state;
            break;
        case INPUT_ROLE_ROLL:
            state.roll                   |= btn_state;
            break;
        case INPUT_ROLE_OPTION:
            state.option                 |= btn_state;
            state.deselect               |= btn_state;
            break;
        case INPUT_ROLE_FLY_CHEAT:
            state.fly_cheat              |= btn_state;
            break;
        case INPUT_ROLE_ITEM_CHEAT:
            state.item_cheat             |= btn_state;
            break;
        case INPUT_ROLE_LEVEL_SKIP_CHEAT:
            state.level_skip_cheat       |= btn_state;
            break;
        case INPUT_ROLE_TURBO_CHEAT:
            state.turbo_cheat            |= btn_state;
            break;
        case INPUT_ROLE_PAUSE:
            state.pause                  |= btn_state;
            break;
        case INPUT_ROLE_CAMERA_UP:
            state.camera_up              |= btn_state;
            break;
        case INPUT_ROLE_CAMERA_DOWN:
            state.camera_down            |= btn_state;
            break;
        case INPUT_ROLE_CAMERA_LEFT:
            state.camera_left            |= btn_state;
            break;
        case INPUT_ROLE_CAMERA_RIGHT:
            state.camera_right           |= btn_state;
            break;
        case INPUT_ROLE_CAMERA_RESET:
            state.camera_reset           |= btn_state;
            break;
        case INPUT_ROLE_EQUIP_PISTOLS:
            state.equip_pistols          |= btn_state;
            break;
        case INPUT_ROLE_EQUIP_SHOTGUN:
            state.equip_shotgun          |= btn_state;
            break;
        case INPUT_ROLE_EQUIP_MAGNUMS:
            state.equip_magnums          |= btn_state;
            break;
        case INPUT_ROLE_EQUIP_UZIS:
            state.equip_uzis             |= btn_state;
            break;
        case INPUT_ROLE_USE_SMALL_MEDI:
            state.use_small_medi         |= btn_state;
            break;
        case INPUT_ROLE_USE_BIG_MEDI:
            state.use_big_medi           |= btn_state;
            break;
        case INPUT_ROLE_SAVE:
            state.save                   |= btn_state;
            break;
        case INPUT_ROLE_LOAD:
            state.load                   |= btn_state;
            break;
        case INPUT_ROLE_FPS:
            state.toggle_fps_counter     |= btn_state;
            break;
        case INPUT_ROLE_BILINEAR:
            state.toggle_bilinear_filter |= btn_state;
            break;
        default:
            break;
        }
    }
    // clang-format on
    return state;
 }

void S_Input_Init(void)
{
    m_KeyboardState = SDL_GetKeyboardState(NULL);

    int32_t result = SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR);
    if (result < 0) {
        LOG_ERROR("Error while calling SDL_Init: 0x%lx", result);
    } else {
        int controllers = SDL_NumJoysticks();
        LOG_INFO("%d controllers", controllers);
        for (int i = 0; i < controllers; i++) {
            m_ControllerName = SDL_GameControllerNameForIndex(i);
            m_ControllerType = SDL_GameControllerTypeForIndex(i);
            bool is_game_controller = SDL_IsGameController(i);
            LOG_DEBUG(
                "controller %d: %s %d (%d)", i, m_ControllerName,
                m_ControllerType, is_game_controller);
            if (!m_Controller && is_game_controller) {
                m_Controller = SDL_GameControllerOpen(i);
                if (!m_Controller) {
                    LOG_ERROR("Could not open controller: %s", SDL_GetError());
                }
            }
        }
    }
}

void S_Input_Shutdown(void)
{
    if (m_Controller) {
        SDL_GameControllerClose(m_Controller);
    }
}

INPUT_STATE S_Input_GetCurrentState(
    INPUT_LAYOUT layout_num, INPUT_LAYOUT cntlr_layout_num)
{
    INPUT_STATE linput = { 0 };

    // clang-format off
    linput.forward                   = S_Input_Key(INPUT_ROLE_UP, layout_num);
    linput.back                      = S_Input_Key(INPUT_ROLE_DOWN, layout_num);
    linput.left                      = S_Input_Key(INPUT_ROLE_LEFT, layout_num);
    linput.right                     = S_Input_Key(INPUT_ROLE_RIGHT, layout_num);
    linput.step_left                 = S_Input_Key(INPUT_ROLE_STEP_L, layout_num);
    linput.step_right                = S_Input_Key(INPUT_ROLE_STEP_R, layout_num);
    linput.slow                      = S_Input_Key(INPUT_ROLE_SLOW, layout_num);
    linput.jump                      = S_Input_Key(INPUT_ROLE_JUMP, layout_num);
    linput.action                    = S_Input_Key(INPUT_ROLE_ACTION, layout_num);
    linput.draw                      = S_Input_Key(INPUT_ROLE_DRAW, layout_num);
    linput.look                      = S_Input_Key(INPUT_ROLE_LOOK, layout_num);
    linput.roll                      = S_Input_Key(INPUT_ROLE_ROLL, layout_num);
    linput.option                    = S_Input_Key(INPUT_ROLE_OPTION, layout_num);
    linput.pause                     = S_Input_Key(INPUT_ROLE_PAUSE, layout_num);
    linput.camera_up                 = S_Input_Key(INPUT_ROLE_CAMERA_UP, layout_num);
    linput.camera_down               = S_Input_Key(INPUT_ROLE_CAMERA_DOWN, layout_num);
    linput.camera_left               = S_Input_Key(INPUT_ROLE_CAMERA_LEFT, layout_num);
    linput.camera_right              = S_Input_Key(INPUT_ROLE_CAMERA_RIGHT, layout_num);
    linput.camera_reset              = S_Input_Key(INPUT_ROLE_CAMERA_RESET, layout_num);

    linput.item_cheat                = S_Input_Key(INPUT_ROLE_ITEM_CHEAT, layout_num);
    linput.fly_cheat                 = S_Input_Key(INPUT_ROLE_FLY_CHEAT, layout_num);
    linput.level_skip_cheat          = S_Input_Key(INPUT_ROLE_LEVEL_SKIP_CHEAT, layout_num);
    linput.turbo_cheat               = S_Input_Key(INPUT_ROLE_TURBO_CHEAT, layout_num);
    linput.health_cheat              = KEY_DOWN(SDL_SCANCODE_F11);

    linput.equip_pistols             = S_Input_Key(INPUT_ROLE_EQUIP_PISTOLS, layout_num);
    linput.equip_shotgun             = S_Input_Key(INPUT_ROLE_EQUIP_SHOTGUN, layout_num);
    linput.equip_magnums             = S_Input_Key(INPUT_ROLE_EQUIP_MAGNUMS, layout_num);
    linput.equip_uzis                = S_Input_Key(INPUT_ROLE_EQUIP_UZIS, layout_num);
    linput.use_small_medi            = S_Input_Key(INPUT_ROLE_USE_SMALL_MEDI, layout_num);
    linput.use_big_medi              = S_Input_Key(INPUT_ROLE_USE_BIG_MEDI, layout_num);

    linput.select                    = S_Input_Key(INPUT_ROLE_ACTION, layout_num);
    linput.deselect                  = S_Input_Key(INPUT_ROLE_OPTION, layout_num);

    linput.save                      = S_Input_Key(INPUT_ROLE_SAVE, layout_num);
    linput.load                      = S_Input_Key(INPUT_ROLE_LOAD, layout_num);

    linput.toggle_fps_counter        = S_Input_Key(INPUT_ROLE_FPS, layout_num);
    linput.toggle_bilinear_filter    = S_Input_Key(INPUT_ROLE_BILINEAR, layout_num);
    linput.toggle_perspective_filter = KEY_DOWN(SDL_SCANCODE_F4);
    // clang-format on

    if (m_Controller) {
        linput = S_Input_GetControllerState(linput, cntlr_layout_num);
    }

    return linput;
}

INPUT_SCANCODE S_Input_GetAssignedScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return m_Layout[layout_num][role];
}

int16_t S_Input_GetUniqueBind(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    CONTROLLER_MAP assigned = m_ControllerLayout[layout_num][role];
    if (assigned.type == BT_AXIS) {
        // Add SDL_CONTROLLER_BUTTON_MAX as an axis offset because button and
        // axis enum values overlap. Also offset depending on axis direction.
        if (assigned.axis_dir == -1) {
            return m_ControllerLayout[layout_num][role].bind.axis
                + SDL_CONTROLLER_BUTTON_MAX;
        } else {
            return m_ControllerLayout[layout_num][role].bind.axis
                + SDL_CONTROLLER_BUTTON_MAX + 10;
        }
    }
    return m_ControllerLayout[layout_num][role].bind.button;
}

int16_t S_Input_GetAssignedButtonType(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return m_ControllerLayout[layout_num][role].type;
}

int16_t S_Input_GetAssignedBind(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    if (m_ControllerLayout[layout_num][role].type == BT_BUTTON) {
        return m_ControllerLayout[layout_num][role].bind.button;
    } else {
        return m_ControllerLayout[layout_num][role].bind.axis;
    }
}

int16_t S_Input_GetAssignedAxisDir(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return m_ControllerLayout[layout_num][role].axis_dir;
}

void S_Input_AssignScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, INPUT_SCANCODE scancode)
{
    m_Layout[layout_num][role] = scancode;
}

void S_Input_AssignButton(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, SDL_GameControllerButton button)
{
    m_ControllerLayout[layout_num][role].type = BT_BUTTON;
    m_ControllerLayout[layout_num][role].bind.button = button;
    m_ControllerLayout[layout_num][role].axis_dir = 0;
}

void S_Input_AssignAxis(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, SDL_GameControllerAxis axis,
    int16_t axis_dir)
{
    m_ControllerLayout[layout_num][role].type = BT_AXIS;
    m_ControllerLayout[layout_num][role].bind.axis = axis;
    m_ControllerLayout[layout_num][role].axis_dir = axis_dir;
}

void S_Input_ResetControllerToDefault(INPUT_LAYOUT layout_num)
{
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        CONTROLLER_MAP default_btn =
            m_ControllerLayout[INPUT_LAYOUT_DEFAULT][role];
        m_ControllerLayout[layout_num][role] = default_btn;
    }
}

bool S_Input_ReadAndAssignKey(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    if (mode == CM_KEYBOARD) {
        for (INPUT_SCANCODE scancode = 0; scancode < SDL_NUM_SCANCODES;
             scancode++) {
            if (KEY_DOWN(scancode)) {
                m_Layout[layout_num][role] = scancode;
                return true;
            }
        }
    } else {
        for (SDL_GameControllerButton button = 0; button < SDL_CONTROLLER_BUTTON_MAX;
             button++) {
            if (S_Input_JoyBtn(button)) {
                S_Input_AssignButton(layout_num, role, button);
                return true;
            }
        }
        for (SDL_GameControllerAxis axis = 0; axis < SDL_CONTROLLER_AXIS_MAX;
             axis++) {
            int16_t axis_dir = S_Input_JoyAxis(axis);
            if (axis_dir != 0) {
                S_Input_AssignAxis(layout_num, role, axis, axis_dir);
                return true;
            }
        }
    }
    return false;
}

const char *S_Input_GetKeyName(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    if (mode == CM_KEYBOARD) {
        return S_Input_GetScancodeName(m_Layout[layout_num][role]);
    } else {
        CONTROLLER_MAP check = m_ControllerLayout[layout_num][role];
        if (check.type == BT_BUTTON) {
            return S_Input_GetButtonName(check.bind.button);
        } else {
            return S_Input_GetAxisName(check.bind.axis, check.axis_dir);
        }
    }
}

const char *S_Input_GetButtonNameFromString(
    INPUT_LAYOUT layout_num, const char *btn_name)
{
    SDL_GameControllerButton button =
        SDL_GameControllerGetButtonFromString(btn_name);
    return S_Input_GetButtonName(button);
}

bool S_Input_CheckKeypress(const char *key_name)
{
    SDL_Scancode scancode = SDL_GetScancodeFromName(key_name);

    if (KEY_DOWN(scancode)) {
        return true;
    }
    return false;
}

bool S_Input_CheckButtonPress(const char *btn_name)
{
    SDL_GameControllerButton button =
        SDL_GameControllerGetButtonFromString(btn_name);

    return S_Input_JoyBtn(button);
}
