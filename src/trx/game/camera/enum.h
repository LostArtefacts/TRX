#pragma once

typedef enum {
    CAM_CHASE = 0,
    CAM_FIXED = 1,
    CAM_LOOK = 2,
    CAM_COMBAT = 3,
    CAM_CINEMATIC = 4,
    CAM_HEAVY = 5,
    CAM_PHOTO_MODE = 6,
    CAM_FLYBY_MODE = 7,
    CAM_BINOCULARS = 8,
} CAMERA_TYPE;

typedef enum {
    CF_NORMAL = 0,
    CF_FOLLOW_CENTRE = 1,
    CF_NO_CHUNKY = 2,
    CF_CHASE_OBJECT = 3,
    CF_BLOCK_UPDATE = 4,
} CAMERA_FLAGS;

// Flags of a fixed camera's OBJECT_VECTOR word. The value is fixed by the
// floordata trigger encoding and round-trips through level and save files.
typedef enum {
    FCF_ONE_SHOT = 0x0100,
} FIXED_CAMERA_FLAG;
