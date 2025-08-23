#pragma once

#include <stdint.h>

typedef struct {
    int32_t buffer_transfer_count;
    int32_t buffer_total_bytes;
    int32_t uniform_changes;
    int32_t opaque_vert_count;
    int32_t trans_vert_count;
} GFX_METRICS;

extern GFX_METRICS g_GFX_Metrics;

#define GFX_TRACK_UNIFORM(fn, ...)                                             \
    do {                                                                       \
        g_GFX_Metrics.uniform_changes++;                                       \
        fn(__VA_ARGS__);                                                       \
    } while (0);

#define GFX_TRACK_DATA(fn, a, b, c, d)                                         \
    do {                                                                       \
        g_GFX_Metrics.buffer_total_bytes += b;                                 \
        g_GFX_Metrics.buffer_transfer_count++;                                 \
        fn(a, b, c, d);                                                        \
    } while (0);

#define GFX_TRACK_SUBDATA(fn, a, b, c, d)                                      \
    do {                                                                       \
        g_GFX_Metrics.buffer_total_bytes += c;                                 \
        g_GFX_Metrics.buffer_transfer_count++;                                 \
        fn(a, b, c, d);                                                        \
    } while (0);

void GFX_Track_Reset(void);
GFX_METRICS GFX_Track_GetMetrics(void);
