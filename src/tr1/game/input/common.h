#pragma once

#include <libtrx/json.h>

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    INPUT_BACKEND_KEYBOARD,
    INPUT_BACKEND_CONTROLLER,
} INPUT_BACKEND;

typedef enum {
    INPUT_LAYOUT_DEFAULT,
    INPUT_LAYOUT_CUSTOM_1,
    INPUT_LAYOUT_CUSTOM_2,
    INPUT_LAYOUT_CUSTOM_3,
    INPUT_LAYOUT_NUMBER_OF,
} INPUT_LAYOUT;

typedef enum {
    // clang-format off
    INPUT_ROLE_UP                = 0,
    INPUT_ROLE_DOWN              = 1,
    INPUT_ROLE_LEFT              = 2,
    INPUT_ROLE_RIGHT             = 3,
    INPUT_ROLE_STEP_L            = 4,
    INPUT_ROLE_STEP_R            = 5,
    INPUT_ROLE_SLOW              = 6,
    INPUT_ROLE_JUMP              = 7,
    INPUT_ROLE_ACTION            = 8,
    INPUT_ROLE_DRAW              = 9,
    INPUT_ROLE_LOOK              = 10,
    INPUT_ROLE_ROLL              = 11,
    INPUT_ROLE_OPTION            = 12,
    INPUT_ROLE_FLY_CHEAT         = 13,
    INPUT_ROLE_ITEM_CHEAT        = 14,
    INPUT_ROLE_LEVEL_SKIP_CHEAT  = 15,
    INPUT_ROLE_TURBO_CHEAT       = 16,
    INPUT_ROLE_PAUSE             = 17,
    INPUT_ROLE_CAMERA_FORWARD    = 18,
    INPUT_ROLE_CAMERA_BACK       = 19,
    INPUT_ROLE_CAMERA_LEFT       = 20,
    INPUT_ROLE_CAMERA_RIGHT      = 21,
    INPUT_ROLE_CAMERA_RESET      = 22,
    INPUT_ROLE_EQUIP_PISTOLS     = 23,
    INPUT_ROLE_EQUIP_SHOTGUN     = 24,
    INPUT_ROLE_EQUIP_MAGNUMS     = 25,
    INPUT_ROLE_EQUIP_UZIS        = 26,
    INPUT_ROLE_USE_SMALL_MEDI    = 27,
    INPUT_ROLE_USE_BIG_MEDI      = 28,
    INPUT_ROLE_SAVE              = 29,
    INPUT_ROLE_LOAD              = 30,
    INPUT_ROLE_FPS               = 31,
    INPUT_ROLE_BILINEAR          = 32,
    INPUT_ROLE_ENTER_CONSOLE     = 33,
    INPUT_ROLE_CHANGE_TARGET     = 34,
    INPUT_ROLE_TOGGLE_UI         = 35,
    INPUT_ROLE_CAMERA_UP         = 36,
    INPUT_ROLE_CAMERA_DOWN       = 37,
    INPUT_ROLE_TOGGLE_PHOTO_MODE = 38,
    INPUT_ROLE_UNBIND_KEY        = 39,
    INPUT_ROLE_RESET_BINDINGS    = 40,
    INPUT_ROLE_NUMBER_OF,
    // clang-format on
} INPUT_ROLE;

typedef union INPUT_STATE {
    uint64_t any;
    struct {
        uint64_t forward : 1;
        uint64_t back : 1;
        uint64_t left : 1;
        uint64_t right : 1;
        uint64_t jump : 1;
        uint64_t draw : 1;
        uint64_t action : 1;
        uint64_t slow : 1;
        uint64_t option : 1;
        uint64_t look : 1;
        uint64_t step_left : 1;
        uint64_t step_right : 1;
        uint64_t roll : 1;
        uint64_t pause : 1;
        uint64_t save : 1;
        uint64_t load : 1;
        uint64_t fly_cheat : 1;
        uint64_t item_cheat : 1;
        uint64_t level_skip_cheat : 1;
        uint64_t turbo_cheat : 1;
        uint64_t health_cheat : 1;
        uint64_t camera_up : 1;
        uint64_t camera_down : 1;
        uint64_t camera_forward : 1;
        uint64_t camera_back : 1;
        uint64_t camera_left : 1;
        uint64_t camera_right : 1;
        uint64_t camera_reset : 1;
        uint64_t equip_pistols : 1;
        uint64_t equip_shotgun : 1;
        uint64_t equip_magnums : 1;
        uint64_t equip_uzis : 1;
        uint64_t use_small_medi : 1;
        uint64_t use_big_medi : 1;
        uint64_t toggle_bilinear_filter : 1;
        uint64_t toggle_perspective_filter : 1;
        uint64_t toggle_fps_counter : 1;
        uint64_t menu_up : 1;
        uint64_t menu_down : 1;
        uint64_t menu_left : 1;
        uint64_t menu_right : 1;
        uint64_t menu_confirm : 1;
        uint64_t menu_back : 1;
        uint64_t enter_console : 1;
        uint64_t change_target : 1;
        uint64_t toggle_ui : 1;
        uint64_t toggle_photo_mode : 1;
    };
} INPUT_STATE;

typedef struct {
    void (*init)(void);
    void (*shutdown)(void);
    bool (*update)(INPUT_STATE *result, INPUT_LAYOUT layout);
    bool (*is_pressed)(INPUT_LAYOUT layout, INPUT_ROLE role);
    bool (*is_role_conflicted)(INPUT_LAYOUT layout, INPUT_ROLE role);
    const char *(*get_name)(INPUT_LAYOUT layout, INPUT_ROLE role);
    void (*unassign_role)(INPUT_LAYOUT layout, INPUT_ROLE role);
    bool (*assign_from_json_object)(INPUT_LAYOUT layout, JSON_OBJECT *bind_obj);
    bool (*assign_to_json_object)(
        INPUT_LAYOUT layout, JSON_OBJECT *bind_obj, INPUT_ROLE role);
    void (*reset_layout)(INPUT_LAYOUT layout);
    bool (*read_and_assign)(INPUT_LAYOUT layout, INPUT_ROLE role);
} INPUT_BACKEND_IMPL;

extern INPUT_STATE g_Input;
extern INPUT_STATE g_InputDB;
extern INPUT_STATE g_OldInputDB;

void Input_Init(void);
void Input_Shutdown(void);
void Input_InitController(void);
void Input_ShutdownController(void);
void Input_Update(void);

// Checks whether the given role can be assigned to by the player.
// Hard-coded roles are exempt from conflict checks (eg will never flash in the
// controls dialog).
bool Input_IsRoleRebindable(INPUT_ROLE role);

// Returns whether the key assigned to the given role is also used elsewhere
// within the custom layout.
bool Input_IsKeyConflicted(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// Checks if the given key is being pressed. Works regardless of Input_Update.
bool Input_CheckKeypress(INPUT_LAYOUT layout, INPUT_ROLE role);

// Given the input layout and input key role, check if the assorted key is
// pressed, bypassing Input_Update.
bool Input_IsPressed(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// If there is anything pressed, assigns the pressed key to the given key role
// and returns true. If nothing is pressed, immediately returns false.
bool Input_ReadAndAssignRole(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// Remove assigned key from a given key role.
void Input_UnassignRole(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// Given the input layout and input key role, get the assigned key name.
const char *Input_GetKeyName(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// Reset a given layout to the default.
void Input_ResetLayout(INPUT_BACKEND backend, INPUT_LAYOUT layout);

// Disables updating g_Input.
void Input_EnterListenMode(void);

// Enables updating g_Input.
void Input_ExitListenMode(void);

// Restores the user configuration by converting the JSON object back into the
// original input layout.
bool Input_AssignFromJSONObject(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, JSON_OBJECT *bind_obj);

// Converts the original input layout into a JSON object for storing the user
// configuration.
bool Input_AssignToJSONObject(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, JSON_OBJECT *bind_obj,
    INPUT_ROLE role);
