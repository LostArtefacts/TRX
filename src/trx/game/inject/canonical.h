#pragma once

#include <trx/core/vector.h>
#include <trx/game/inject/types.h>

// Readers for TRXI's canonical records. The transcode functions parse the
// game-independent layout and rebuild it in the given game's native byte
// layout in memory - they touch no engine state, so tests can hold the
// conversion logic on its own. The append functions wrap them for the
// loaded level, feeding the ordinary section readers.

// Returns the native buffer through out_data (caller frees) and its size.
int32_t InjectCanonical_TranscodeObjectTextures(
    TRX_FILE *src, int32_t data_count, int32_t game_version, char **out_data);

// Parses canonical frames, packs them in the game's native form, and
// reports each frame's byte offset in the native blob and its rotation
// count through the two vectors (int32_t entries, one per frame ordinal).
int32_t InjectCanonical_TranscodeAnimFrames(
    TRX_FILE *src, int32_t data_count, int32_t game_version, char **out_data,
    VECTOR *out_offsets, VECTOR *out_rot_counts);

// Rewrites each animation's frame ordinal into a native byte offset and its
// frame size to match the native packing.
int32_t InjectCanonical_TranscodeAnims(
    TRX_FILE *src, int32_t data_count, int32_t game_version,
    const VECTOR *frame_offsets, const VECTOR *frame_rot_counts,
    char **out_data);

// The native byte offset for a frame ordinal of the most recently
// transcoded frames block, for records outside the animation chunk.
int32_t InjectCanonical_FrameOffset(int32_t ordinal);

// Returns the number of native 16-bit words appended.
int32_t InjectCanonical_AppendAnimFrames(
    const INJECTION *injection, int32_t base_idx, int32_t data_count,
    int32_t data_size);

// Rewrites offsets in place from canonical to native blob positions.
int32_t InjectCanonical_TranscodeObjectMeshes(
    TRX_FILE *src, int32_t data_size, int32_t game_version, int32_t *offsets,
    int32_t num_offsets, char **out_data);

void InjectCanonical_AppendObjectTextures(
    const INJECTION *injection, int32_t base_idx, int16_t base_page_idx,
    int32_t data_count);

void InjectCanonical_AppendObjectMeshes(
    const INJECTION *injection, int32_t num_offsets, int32_t *offsets,
    int32_t data_size);

void InjectCanonical_AppendAnims(
    const INJECTION *injection, int32_t base_idx, int32_t data_count);
