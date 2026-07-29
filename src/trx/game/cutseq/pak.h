// Loader for the TR4 cutseq.pak file: a zlib-compressed blob holding the
// in-game cutscene descriptors and their delta-compressed animation tracks.
#pragma once

#include <trx/game/objects/ids.h>
#include <trx/game/types.h>

#include <stdint.h>

#define CUTSEQ_MAX_ACTORS 10

typedef struct {
    uint32_t data_offset;
    OBJECT_ID obj_id;
    int32_t game_obj_slot;
    int32_t node_count;
} CUTSEQ_ACTOR_INFO;

typedef struct {
    int32_t num_actors;
    int32_t num_frames;
    XYZ_32 origin;
    int32_t audio_track;
    uint32_t camera_offset;
    CUTSEQ_ACTOR_INFO actors[CUTSEQ_MAX_ACTORS];

    // The raw cutscene payload; all offsets above are relative to it.
    const uint8_t *data;
    uint32_t data_size;
} CUTSEQ_INFO;

bool CutSeq_Pak_Load(void);
void CutSeq_Pak_Unload(void);
bool CutSeq_Pak_IsLoaded(void);

// One past the highest addressable cutscene number.
int32_t CutSeq_Pak_GetCutsceneCount(void);

// Parses the descriptor of the given cutscene. Returns false when the
// number is out of range or the data is malformed.
bool CutSeq_Pak_GetCutscene(int32_t num, CUTSEQ_INFO *info);
