#pragma once

#include <trx/core/math.h>
#include <trx/game/items/enum.h>
#include <trx/game/objects/ids.h>
#include <trx/game/objects/property.h>
#include <trx/game/output/types.h>

typedef struct CARRIED_ITEM {
    OBJECT_ID object_id;
    int16_t spawn_num;
    XYZ_32 pos;
    XYZ_16 rot;
    int16_t room_num;
    int16_t fall_speed;
    DROP_STATUS status;
    struct CARRIED_ITEM *next_item;
} CARRIED_ITEM;

typedef struct TRAP_DATA TRAP_DATA;
typedef struct CREATURE CREATURE;

// The lean description an item trigger acts on, all an item needs from a
// floordata trigger or a script. `mask` is the 0..31 editor mask (a heavy
// switch has its heavy mask already folded in). `timer` is in seconds;
// Item_Trigger converts it to frames.
typedef struct ITEM_TRIGGER {
    ITEM_TRIGGER_KIND kind;
    int16_t mask;
    float timer;
    bool one_shot;
} ITEM_TRIGGER;

// An item's live trigger state. Held as fields at runtime; the level-load word
// and the savegame word decode into it (see Item_Initialise, M_PackItemFlags).
typedef struct ITEM_TRIGGER_STATE {
    uint8_t mask; // 0..31, the five floordata code bits
    bool reversed;
    bool spent; // general one-shot latch: this trigger target is spent
    bool switch_spent; // TR3 latch: a spent one-shot switch
    bool anti_spent; // TR3 latch: a spent one-shot antitrigger
} ITEM_TRIGGER_STATE;

typedef struct ITEM {
    int32_t floor;
    uint32_t touch_bits;
    uint32_t mesh_bits;
    int16_t after_death;
    // Coverage a fading body has left, out of 255, or 0 when it is not
    // fading. Kept apart from after_death, which TR3 counts its blood bath by.
    int16_t fade;
    OBJECT_ID object_id;
    int16_t current_anim_state;
    int16_t goal_anim_state;
    int16_t required_anim_state;
    int16_t anim_num;
    int16_t frame_num;
    int16_t prev_frame_num;
    int16_t room_num;
    int16_t next_item;
    int16_t next_simulated;
    int16_t speed;
    int16_t fall_speed;
    int16_t hit_points;
    int16_t max_hit_points;
    int16_t box_num;
    int16_t timer;
    // The level-format flags word for this slot, consumed once by
    // Item_Initialise. Not runtime state and not saved; the live trigger state
    // it decodes into lives in `trigger`.
    uint16_t init_flags;
    uint8_t ai_bits;
    int16_t ai_tag;
    // The number carried by the AI marker the creature was placed on, which
    // is how TR4 tells one marker of a kind from another. Level data rather
    // than runtime state, so it is not saved.
    int16_t ai_ocb;
    ITEM_PROPERTY_SET properties;
    ITEM_TRIGGER_STATE trigger;

    SHADE shade;
    union {
        CREATURE *creature_data;
        TRAP_DATA *trap_data;
    };
    int16_t *extra_rotations;
    void *priv;
    CARRIED_ITEM *carried_item;
    char *name;

    XYZ_32 pos;
    XYZ_16 rot;

    bool enable_interpolation;
    bool enable_shadow;

    // Lifecycle axes. These are independent by design: the states the old
    // mutually-exclusive status enum could not name are exactly the legal
    // coexistences here, and the type no longer forbids them, so the rules
    // it used to enforce for free are stated here instead.
    //
    //   is_destroyed is terminal and dominant. Item_Destroy sets it after
    //   taking the slot off the simulation list and out of the room chain,
    //   so is_destroyed implies !is_simulated and !is_present, and the handle
    //   is stale. Readers gate on it first; no other axis is meaningful once
    //   it is set.
    //
    //   is_simulated, is_visible, is_finished and is_collidable are otherwise
    //   free to combine. The combinations that carried the old model's lies:
    //
    //     simulated  && !visible    - ambush enemy woken, not yet in play
    //     simulated  && is_finished - trap playing out its finish
    //     !simulated && visible     - enemy or trap at rest, re-triggerable
    //     collidable && !visible    - a hidden ambush enemy still blocks
    //     !visible   && is_finished - a collected pickup, its trigger spent
    //
    //   That last pair is the one the saved status enum cannot hold, so
    //   is_finished travels in a field of its own (see M_PackItemStatus).
    //
    //   is_present is positional bookkeeping (the room item chain), orthogonal
    //   to the rest and cleared only by Item_DetachFromRoom. An item can be
    //   present but not simulated (at rest), or detached but not destroyed
    //   (a pickup in a carrier's hand).
    //
    // Control routine runs each frame; the simulation-list membership bit.
    // Distinct from the saved status value it helps reconstruct (see
    // M_PackItemStatus). Replaces item->active.
    bool is_simulated;
    // In the world: linked in the room item chain that collision and
    // Item_FindTypeInRoom walk. Read-only, managed by that chain alone
    // (Item_Initialise adds, Item_DetachFromRoom removes). Orthogonal to
    // is_simulated (a sleeping enemy is present, not simulated) and to
    // is_visible (an ambush enemy is present, not visible).
    bool is_present;
    // Drawn on screen. Collision and targeting read it too, the coupling the
    // status enum gave them; world membership is is_present.
    bool is_visible;
    // Object-local "my run is over" phase marker.
    bool is_finished;
    // Runtime form of the saved IF_DESTROYED bit: the slot is dead and removed.
    bool is_destroyed;

    bool gravity;
    bool hit_status;
    bool is_collidable;

    bool looked_at;
    bool dynamic_light;
    bool clear_body;
    bool include_in_kill_stats;

    struct {
        struct {
            int32_t floor;
            XYZ_32 pos;
            XYZ_16 rot;
        } result, prev;
    } interp;
} ITEM;
