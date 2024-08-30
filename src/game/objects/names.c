#include "game/objects/names.h"

#include "game/inventory.h"
#include "game/inventory/inventory_vars.h"
#include "game/objects/common.h"

#include <libtrx/memory.h>
#include <libtrx/strings.h>
#include <libtrx/vector.h>

#include <string.h>

typedef struct {
    const GAME_OBJECT_ID obj_id;
    const char *regex;
} ITEM_NAME;

typedef struct {
    int32_t match_length;
    GAME_OBJECT_ID obj_id;
} MATCH;

static const INVENTORY_ITEM *const m_InvItems[] = {
    &g_InvItemMedi,
    &g_InvItemBigMedi,

    &g_InvItemPistols,
    &g_InvItemShotgun,
    &g_InvItemMagnum,
    &g_InvItemUzi,
    &g_InvItemGrenade,

    &g_InvItemPistolAmmo,
    &g_InvItemShotgunAmmo,
    &g_InvItemMagnumAmmo,
    &g_InvItemUziAmmo,

    &g_InvItemPuzzle1,
    &g_InvItemPuzzle2,
    &g_InvItemPuzzle3,
    &g_InvItemPuzzle4,

    &g_InvItemKey1,
    &g_InvItemKey2,
    &g_InvItemKey3,
    &g_InvItemKey4,

    &g_InvItemPickup1,
    &g_InvItemPickup2,
    &g_InvItemLeadBar,
    &g_InvItemScion,

    &g_InvItemCompass,
    &g_InvItemGame,
    &g_InvItemDetails,
    &g_InvItemSound,
    &g_InvItemControls,
    &g_InvItemLarasHome,

    NULL,
};

static const ITEM_NAME m_ItemNames[] = {
    { O_LARA, "^lara$" },
    { O_LARA_EXTRA, "^lara$" },
    { O_BACON_LARA, "^bacon[- ]?lara$" },
    { O_WOLF, "^wolf$" },
    { O_BEAR, "^bear$" },
    { O_BAT, "^bat$" },
    { O_CROCODILE, "^crocodile^" },
    { O_ALLIGATOR, "^alligator$" },
    { O_LION, "^lion$" },
    { O_LIONESS, "^lion(ess)?$" },
    { O_PUMA, "^puma$" },
    { O_APE, "^ape$" },
    { O_RAT, "^rat$" },
    { O_VOLE, "^(rat|vole)$" },
    { O_TREX, "^(t-?rex|barney)$" },
    { O_RAPTOR, "^raptor$" },
    { O_WARRIOR1, "^(mutant|flyer|(flying|winged)[- ]?mutant)$" },
    { O_WARRIOR2, "^mutant$" },
    { O_WARRIOR3, "^mutant$" },
    { O_CENTAUR, "^(centaur|mutant)$" },
    { O_MUMMY, "^mummy$" },
    { O_DINO_WARRIOR, "^dino[- ]?warrior$" },
    { O_FISH, "^fish$" },
    { O_LARSON, "^larson$" },
    { O_PIERRE, "^pierre$" },
    { O_SKATEBOARD, "^skateboard$" },
    { O_SKATEKID, "^skate(board)?[- ]?kid$" },
    { O_COWBOY, "^cowboy$" },
    { O_BALDY, "^baldy$" },
    { O_NATLA, "^natla$" },
    { O_TORSO, "^torso$" },
    { O_TORSO, "^adam$" },
    { O_TORSO, "^abortion$" },
    { O_FALLING_BLOCK, "^falling[- ]?block$" },
    { O_PENDULUM, "^(pendulum|blade)$" },
    { O_SPIKES, "^spikes$" },
    { O_ROLLING_BALL, "^(rolling[- ]?ball|boulder)$" },
    { O_DARTS, "^darts$" },
    { O_DART_EMITTER, "^(darts?|darts?[- ]?emitter)$" },
    { O_DRAW_BRIDGE, "^draw[- ]?bridge$" },
    { O_TEETH_TRAP, "^(clang[- ]?clang([- ]?door)?|teeth( trap)?)$" },
    { O_DAMOCLES_SWORD, "^(damocles[- ]?sword|swords?)$" },
    { O_THORS_HANDLE, "^thors[- ]?hammer$" },
    { O_THORS_HEAD, "^thors[- ]?hammer$" },
    { O_LIGHTNING_EMITTER, "^lightning[- ]?emitter$" },
    { O_MOVING_BAR, "^moving[- ]?bar$" },
    { O_MOVABLE_BLOCK, "^(movable|push(able)?)[- ]?block$" },
    { O_MOVABLE_BLOCK2, "^(movable|push(able)?)[- ]?block$" },
    { O_MOVABLE_BLOCK3, "^(movable|push(able)?)[- ]?block$" },
    { O_MOVABLE_BLOCK4, "^(movable|push(able)?)[- ]?block$" },
    { O_SLIDING_PILLAR, "^sliding[- ]?pillar$" },
    { O_FALLING_CEILING1, "^falling[- ]?ceiling( 1)?$" },
    { O_FALLING_CEILING2, "^falling[- ]?ceiling( 2)?$" },
    { O_SWITCH_TYPE1, "^(switch|lever)([- ]?1)?$" },
    { O_SWITCH_TYPE2, "^(switch|lever)([- ]?2)?$" },
    { O_DOOR_TYPE1, "^door([- ]?1)?$" },
    { O_DOOR_TYPE2, "^door([- ]?2)?$" },
    { O_DOOR_TYPE3, "^door([- ]?3)?$" },
    { O_DOOR_TYPE4, "^door([- ]?4)?$" },
    { O_DOOR_TYPE5, "^door([- ]?5)?$" },
    { O_DOOR_TYPE6, "^door([- ]?6)?$" },
    { O_DOOR_TYPE7, "^door([- ]?7)?$" },
    { O_DOOR_TYPE8, "^door([- ]?8)?$" },
    { O_TRAPDOOR, "^trap[- ]?door([- ]?1)?$" },
    { O_TRAPDOOR2, "^trap[- ]?door([- ]?2)?$" },
    { O_BIGTRAPDOOR, "^(trap[- ]?door([- ]?3)?|big[- ]?trap[- ]?door)$" },
    { O_BRIDGE_FLAT, "^bridge([- ]?flat)?$" },
    { O_BRIDGE_TILT1, "^bridge([- ]?tilt)?([- ]?1)?$" },
    { O_BRIDGE_TILT2, "^bridge([- ]?tilt)?([- ]?2)?$" },
    { O_COG_1, "^cog([- ]?1)?$" },
    { O_COG_2, "^cog([- ]?2)?$" },
    { O_COG_3, "^cog([- ]?3)?$" },
    { O_PLAYER_1, "^player([- ]?1)?$" },
    { O_PLAYER_2, "^player([- ]?2)?$" },
    { O_PLAYER_3, "^player([- ]?3)?$" },
    { O_PLAYER_4, "^player([- ]?4)?$" },
    { O_SAVEGAME_ITEM, "^(save(game)?[- ]?)?crystal$" },
    { O_PISTOL_ITEM, "^pistols$" },
    { O_SHOTGUN_ITEM, "^(sg|shotgun)$" },
    { O_MAGNUM_ITEM, "^magnums?$" },
    { O_UZI_ITEM, "^uzis?$" },
    { O_PISTOL_AMMO_ITEM, "^pistol[- ]?(ammo|clips?)$" },
    { O_SG_AMMO_ITEM, "^(sg|shotgun)[- ]?(ammo|shells?)$" },
    { O_MAG_AMMO_ITEM, "^magnums?[- ]?(ammo|clips?)$" },
    { O_UZI_AMMO_ITEM, "^uzis?[- ]?(ammo|clips?)$" },
    { O_EXPLOSIVE_ITEM, "^grenade$" },
    { O_MEDI_ITEM, "^(small[- ]?)?medi?([- ]?pack)?$" },
    { O_BIGMEDI_ITEM, "^((big|large)[- ]?)?medi?([- ]?pack)?$" },
    { O_PUZZLE_ITEM1, "^puzzle([- ]?item)?([- ]?1)?$" },
    { O_PUZZLE_ITEM2, "^puzzle([- ]?item)?([- ]?2)?$" },
    { O_PUZZLE_ITEM3, "^puzzle([- ]?item)?([- ]?3)?$" },
    { O_PUZZLE_ITEM4, "^puzzle([- ]?item)?([- ]?4)?$" },
    { O_PUZZLE_HOLE1, "^puzzle[- ]?hole([- ]?1)?$" },
    { O_PUZZLE_HOLE2, "^puzzle[- ]?hole([- ]?2)?$" },
    { O_PUZZLE_HOLE3, "^puzzle[- ]?hole([- ]?3)?$" },
    { O_PUZZLE_HOLE4, "^puzzle[- ]?hole([- ]?4)?$" },
    { O_PUZZLE_DONE1, "^puzzle[- ]?hole([- ]?1)?$" },
    { O_PUZZLE_DONE2, "^puzzle[- ]?hole([- ]?2)?$" },
    { O_PUZZLE_DONE3, "^puzzle[- ]?hole([- ]?3)?$" },
    { O_PUZZLE_DONE4, "^puzzle[- ]?hole([- ]?4)?$" },
    { O_LEADBAR_ITEM, "^lead[- ]?bar$" },
    { O_MIDAS_TOUCH, "^midas[- ]?hand$" },
    { O_KEY_ITEM1, "^key([- ]?1)?$" },
    { O_KEY_ITEM2, "^key([- ]?2)?$" },
    { O_KEY_ITEM3, "^key([- ]?3)?$" },
    { O_KEY_ITEM4, "^key([- ]?4)?$" },
    { O_KEY_HOLE1, "^key[- ]?hole([- ]?1)?$" },
    { O_KEY_HOLE2, "^key[- ]?hole([- ]?2)?$" },
    { O_KEY_HOLE3, "^key[- ]?hole([- ]?3)?$" },
    { O_KEY_HOLE4, "^key[- ]?hole([- ]?4)?$" },
    { O_PICKUP_ITEM1, "^pickup([- ]?1)?$" },
    { O_PICKUP_ITEM2, "^pickup([- ]?2)?$" },
    { O_SCION_ITEM, "^scion([- ]?1)?$" },
    { O_SCION_ITEM2, "^scion([- ]?2)?$" },
    { O_SCION_ITEM3, "^scion([- ]?3)?$" },
    { O_SCION_ITEM4, "^scion([- ]?4)?$" },
    { O_SCION_HOLDER, "^scion[- ]?holder$" },
    { O_EXPLOSION1, "^explosion([- ]?1)?$" },
    { O_EXPLOSION2, "^explosion([- ]?2)?$" },
    { O_SPLASH1, "^splash([- ]?1)?$" },
    { O_SPLASH2, "^splash([- ]?2)?$" },
    { O_BUBBLES1, "^bubble([- ]?1)?$" },
    { O_BUBBLES2, "^bubble([- ]?2)?$" },
    { O_BUBBLE_EMITTER, "^bubble[- ]?emitter$" },
    { O_BLOOD1, "^blood([- ]?1)?$" },
    { O_BLOOD2, "^blood([- ]?2)?$" },
    { O_DART_EFFECT, "^dart[- ]?effect$" },
    { O_STATUE, "^statue$" },
    { O_PORTACABIN, "^cabin$" },
    { O_PORTACABIN, "^porta[- ]?cabin$" },
    { O_PODS, "^pod$" },
    { O_RICOCHET1, "^ricochet$" },
    { O_TWINKLE, "^twinkle$" },
    { O_GUN_FLASH, "^gun[- ]?flash$" },
    { O_DUST, "^dust$" },
    { O_BODY_PART, "^body[- ]?part$" },
    { O_CAMERA_TARGET, "^camera[- ]?target$" },
    { O_WATERFALL, "^waterfall$" },
    { O_MISSILE1, "^missile([- ]?1)?$" },
    { O_MISSILE2, "^missile([- ]?2)?$" },
    { O_MISSILE3, "^missile([- ]?3)?$" },
    { O_MISSILE4, "^missile([- ]?4)?$" },
    { O_MISSILE5, "^missile([- ]?5)?$" },
    { O_LAVA, "^lava$" },
    { O_LAVA_EMITTER, "^lava[- ]?emitter$" },
    { O_FLAME, "^flame$" },
    { O_FLAME_EMITTER, "^flame[- ]?emitter$" },
    { O_LAVA_WEDGE, "^lava[- ]?wedge$" },
    { O_BIG_POD, "^big[- ]?pod$" },
    { O_BOAT, "^boat$" },
    { O_EARTHQUAKE, "^earthquake$" },
    { O_HAIR, "^braid$" },
    { NO_OBJECT, "" },
};

GAME_OBJECT_ID *Object_IdsFromName(const char *name, int32_t *out_match_count)
{
    // first, calculate the number of matches to allocate
    VECTOR *matches = Vector_Create(sizeof(MATCH));

    // Store matches from hardcoded strings
    for (const ITEM_NAME *desc = m_ItemNames; desc->obj_id != NO_OBJECT;
         desc++) {
        const int32_t match_length = String_Match(name, desc->regex);
        if (match_length > 0) {
            MATCH match = {
                .match_length = match_length,
                .obj_id = desc->obj_id,
            };
            Vector_Add(matches, &match);
        }
    }

    // Store matches from customizable inventory strings
    for (const INVENTORY_ITEM *const *item_ptr = m_InvItems; *item_ptr != NULL;
         item_ptr++) {
        const INVENTORY_ITEM *item = *item_ptr;
        if (String_CaseSubstring(item->string, name)) {
            MATCH match = {
                .match_length = strlen(name),
                .obj_id = Object_GetCognateInverse(
                    item->object_number, g_ItemToInvObjectMap),
            };
            Vector_Add(matches, &match);
        }
    }

    // sort by match length so that best-matching results appear first
    for (int i = 0; i < matches->count; i++) {
        for (int j = i + 1; j < matches->count; j++) {
            if (((MATCH *)Vector_Get(matches, i))->match_length
                < ((MATCH *)Vector_Get(matches, j))->match_length) {
                Vector_Swap(matches, i, j);
            }
        }
    }

    // Make sure the returned matching object ids are unique
    GAME_OBJECT_ID *unique_ids =
        Memory_Alloc(sizeof(GAME_OBJECT_ID) * (matches->count + 1));

    int32_t unique_count = 0;
    for (int32_t i = 0; i < matches->count; i++) {
        const GAME_OBJECT_ID obj_id = ((MATCH *)Vector_Get(matches, i))->obj_id;
        bool is_unique = true;
        for (int32_t j = 0; j < unique_count; j++) {
            if (obj_id == unique_ids[j]) {
                is_unique = false;
                break;
            }
        }
        if (is_unique) {
            unique_ids[unique_count++] = obj_id;
        }
    }

    Vector_Free(matches);
    matches = NULL;

    // Finalize results
    unique_ids[unique_count] = NO_OBJECT;
    if (out_match_count != NULL) {
        *out_match_count = unique_count;
    }

    Memory_FreePointer(&matches);
    return unique_ids;
}

const char *Object_GetCanonicalName(
    const GAME_OBJECT_ID obj_id, const char *user_input)
{
    for (const INVENTORY_ITEM *const *item_ptr = m_InvItems; *item_ptr != NULL;
         item_ptr++) {
        const INVENTORY_ITEM *item = *item_ptr;
        if (item->string != NULL
            && Inv_GetItemOption(item->object_number)
                == Inv_GetItemOption(obj_id)) {
            return item->string;
        }
    }

    return user_input;
}
