#ifndef TR1MAIN_STRUCT_H
#define TR1MAIN_STRUCT_H

#include <stdio.h>

typedef enum {
    ID_LARA = 0,
    ID_SMALL_MEDIPACK_ITEM = 93,
    ID_LARGE_MEDIPACK_ITEM = 94,
    ID_NUMBER_OBJECTS = 191,
} GAME_OBJECT_ID;

typedef enum {
    GBUF_RoomInfos = 11,
    GBUF_RoomMesh = 12,
    GBUF_RoomDoor = 13,
    GBUF_RoomFloor = 14,
    GBUF_RoomLights = 15,
    GBUF_RoomStaticMeshInfos = 16,
    GBUF_FloorData = 17,
} GAMEALLOC_BUFFER;

#pragma pack(push, 1)

typedef struct Pos2D_t {
    __int16 x;
    __int16 y;
} POS_2D;

typedef struct Pos3D_t {
    __int16 x;
    __int16 y;
    __int16 z;
} POS_3D;

typedef struct DoorInfo_t {
    __int16 room;
    __int16 x;
    __int16 y;
    __int16 z;
    POS_3D vertex[4];
} DOOR_INFO;

typedef struct DoorInfos_t {
    __int16 count;
    DOOR_INFO door[];
} DOOR_INFOS;

typedef struct FloorInfo_t {
    __int16 index;
    __int16 box;
    __int8 pitRoom;
    __int8 floor;
    __int8 skyRoom;
    __int8 ceiling;
} FLOOR_INFO;

typedef struct LightInfo_t {
    __int32 x;
    __int32 y;
    __int32 z;
    __int16 intensity;
    __int32 falloff;
} LIGHT_INFO;

typedef struct MeshInfo_t {
    __int32 x;
    __int32 y;
    __int32 z;
    __int16 yRot;
    __int16 shade;
    __int16 staticNumber;
} MESH_INFO;

typedef struct RoomInfo_t {
    __int16 *data;
    DOOR_INFOS *doors;
    FLOOR_INFO *floor;
    LIGHT_INFO *light;
    MESH_INFO *mesh;
    __int32 x;
    __int32 y;
    __int32 z;
    __int32 minFloor;
    __int32 maxCeiling;
    __int16 xSize;
    __int16 ySize;
    __int16 ambient;
    __int16 numLights;
    __int16 numMeshes;
    __int16 boundLeft;
    __int16 boundRight;
    __int16 boundTop;
    __int16 boundBottom;
    __int16 boundActive;
    __int16 itemNumber;
    __int16 fxNumber;
    __int16 flippedRoom;
    __int16 flags;
} ROOM_INFO;

typedef struct Phd3dPos_t {
    int x;
    int y;
    int z;
    __int16 rotX;
    __int16 rotY;
    __int16 rotZ;
} PHD_3DPOS;

typedef struct ItemInfo_t {
    int floor;
    __int32 touchBits;
    __int32 meshBits;
    __int16 objectID;
    __int16 currentAnimState;
    __int16 goalAnimState;
    __int16 requiredAnimState;
    __int16 animNumber;
    __int16 frameNumber;
    __int16 roomNumber;
    __int16 nextItem;
    __int16 nextActive;
    __int16 speed;
    __int16 fallSpeed;
    __int16 hitPoints;
    __int16 boxNumber;
    __int16 timer;
    unsigned __int16 flags; // see IFL_* defines
    __int16 shade1;
    void *data;
    PHD_3DPOS pos;
    unsigned __int16 moreFlags;
} ITEM_INFO;

typedef struct LaraInfo_t {
    __int16 itemNumber;
    __int16 gunStatus;
    __int16 field_4;
    __int16 field_6;
    __int16 field_8;
    __int16 waterStatus;
    __int16 field_C;
    __int16 field_E;
    __int16 field_10;
    __int16 air;
    __int16 field_14;
    __int16 field_16;
    __int16 field_18;
    __int16 field_1A;
    __int32 field_1C;
    __int32 field_20;
    __int8 field_24[60];
    __int32 field_60;
    __int8 field_64[4];
    __int32 field_68;
    __int32 field_6C;
    __int32 field_70;
    __int32 field_74;
    __int8 field_78[6];
    __int16 field_7E;
    __int8 field_80[6];
    __int16 field_86;
    __int8 field_88[6];
    __int16 field_8E;
    __int8 field_90[6];
    __int16 field_96;
    __int32 field_98;
    __int8 field_9C[8];
    __int32 field_A4;
    __int8 field_A8[8];
    __int32 field_B0;
    __int8 field_B4[8];
    __int32 field_BC;
    __int8 field_C0[8];
    __int8 field_C8[8];
    __int16 field_D0;
    __int16 field_D2;
    __int16 field_D4;
    __int16 field_D6;
    __int16 field_D8;
} LARA_INFO;

#pragma pop

#endif
