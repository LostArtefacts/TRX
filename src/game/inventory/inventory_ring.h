#pragma once

#include "global/types.h"

#include <stdint.h>

void Inv_Ring_IsOpen(RING_INFO *ring);
void Inv_Ring_IsNotOpen(RING_INFO *ring);
void Inv_Ring_Active(INVENTORY_ITEM *inv_item);
void RingNotActive(void);

void RemoveInventoryText(void);

void Inv_Ring_Init(
    RING_INFO *ring, int16_t type, INVENTORY_ITEM **list, int16_t qty,
    int16_t current, IMOTION_INFO *imo);
void Inv_Ring_GetView(RING_INFO *a1, PHD_3DPOS *viewer);
void Inv_Ring_Light(RING_INFO *ring);
void Inv_Ring_CalcAdders(RING_INFO *ring, int16_t rotation_duration);
void Inv_Ring_DoMotions(RING_INFO *ring);
void Inv_Ring_RotateLeft(RING_INFO *ring);
void Inv_Ring_RotateRight(RING_INFO *ring);
void Inv_Ring_MotionInit(
    RING_INFO *ring, int16_t frames, int16_t status, int16_t status_target);
void Inv_Ring_MotionSetup(
    RING_INFO *ring, int16_t status, int16_t status_target, int16_t frames);
void Inv_Ring_MotionRadius(RING_INFO *ring, int16_t target);
void Inv_Ring_MotionRotation(RING_INFO *ring, int16_t rotation, int16_t target);
void Inv_Ring_MotionCameraPos(RING_INFO *ring, int16_t target);
void Inv_Ring_MotionCameraPitch(RING_INFO *ring, int16_t target);
void Inv_Ring_MotionItemSelect(RING_INFO *ring, INVENTORY_ITEM *inv_item);
void Inv_Ring_MotionItemDeselect(RING_INFO *ring, INVENTORY_ITEM *inv_item);
