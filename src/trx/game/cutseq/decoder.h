// Decoder for the delta-compressed rotation/position tracks stored in the
// TR4 cutseq.pak file. Each track is a bit-packed RLE stream of signed
// per-frame deltas; a node bundles three tracks (one per axis).
#pragma once

#include <trx/game/types.h>

#include <stdint.h>

// The largest number of meshes a cutscene actor can animate. Actor node
// counts come from the pak file; Lara uses 15, other actors use fewer.
#define CUTSEQ_MAX_MESHES 32

typedef struct {
    uint32_t length;
    uint32_t off;
    uint16_t counter;
    int16_t data;
    uint8_t decode_type;
    uint8_t pack_method;
} CUTSEQ_TRACK_STATE;

typedef struct {
    int16_t x_run, y_run, z_run;
    int16_t x_key, y_key, z_key;
    CUTSEQ_TRACK_STATE decode_x, decode_y, decode_z;
    uint32_t x_length, y_length, z_length;
    const uint8_t *x_packed, *y_packed, *z_packed;
} CUTSEQ_PACK_NODE;

// A fully decoded actor pose for one frame: the root offset (relative to the
// cutscene origin) plus one rotation per mesh (node 0 is the root, node N+1
// carries the rotation of mesh N).
typedef struct {
    XYZ_32 offset;
    XYZ_16 rots[CUTSEQ_MAX_MESHES];
} CUTSEQ_POSE;

// Parses num_nodes node headers at the start of packed and points each node
// at its three track streams. Returns the total byte size of the node block,
// or -1 when it does not fit within data_size.
int32_t CutSeq_Decoder_InitNodes(
    const uint8_t *packed, uint32_t data_size, CUTSEQ_PACK_NODE *nodes,
    int32_t num_nodes);

// Rewinds all tracks to frame 0 (runs = keys).
void CutSeq_Decoder_Reset(CUTSEQ_PACK_NODE *nodes, int32_t num_nodes);

// Advances all tracks by one frame. The root node accumulates unmasked
// (positions); child nodes are masked (1023 for 10-bit actor angles,
// 0xFFFF for camera nodes).
void CutSeq_Decoder_Advance(
    CUTSEQ_PACK_NODE *nodes, int32_t num_nodes, uint16_t mask);

// Converts the current runs of an actor's nodes into a drawable pose.
void CutSeq_Decoder_BuildPose(
    const CUTSEQ_PACK_NODE *nodes, int32_t num_nodes, CUTSEQ_POSE *pose);
