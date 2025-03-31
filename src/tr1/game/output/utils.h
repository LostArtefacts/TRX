#define OUTPUT_QUAD_VERTICES 6
#define OUTPUT_TRI_VERTICES 3

#define OUTPUT_QUAD_TO_FAN(i) ((int32_t[]) { 0, 2, 1, 0, 3, 2 }[i])
#define OUTPUT_TRI_TO_FAN(i) ((int32_t[]) { 0, 2, 1 }[i])
