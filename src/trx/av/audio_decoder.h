#pragma once

#include <stddef.h>
#include <stdint.h>

// Turns a compressed audio file into the mixer's working format. The caller
// keeps the memory a decoder was created over alive for as long as it lives.
typedef struct AUDIO_DECODER AUDIO_DECODER;

AUDIO_DECODER *AudioDecoder_CreateFromPath(const char *path, int32_t channels);
AUDIO_DECODER *AudioDecoder_CreateFromMemory(
    const uint8_t *data, size_t size, int32_t channels);
void AudioDecoder_Free(AUDIO_DECODER **decoder);

// How long the source is, in seconds, or a negative value where the container
// does not say.
double AudioDecoder_GetDuration(const AUDIO_DECODER *decoder);

// Where in the source the last decoded frame came from, in seconds.
double AudioDecoder_GetTimestamp(const AUDIO_DECODER *decoder);

bool AudioDecoder_Seek(AUDIO_DECODER *decoder, double timestamp);
bool AudioDecoder_Rewind(AUDIO_DECODER *decoder, double start_at);

// Decodes the next packet and points `out` at the samples it produced. Returns
// how many samples per channel those are - zero where the packet carried none
// - or a negative value once the source has run out. The samples stay valid
// until the next call.
int32_t AudioDecoder_Read(AUDIO_DECODER *decoder, const float **out);
