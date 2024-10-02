#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char *content;
    size_t size;
    char *cur_ptr;
} VFILE;

VFILE *VFile_CreateFromPath(const char *path);
VFILE *VFile_CreateFromBuffer(const char *data, size_t size);
void VFile_Close(VFILE *file);

size_t VFile_GetPos(const VFILE *file);
void VFile_SetPos(VFILE *file, size_t pos);
void VFile_Skip(VFILE *file, int32_t offset);

void VFile_Read(VFILE *file, void *target, size_t size);
int8_t VFile_ReadS8(VFILE *file);
int16_t VFile_ReadS16(VFILE *file);
int32_t VFile_ReadS32(VFILE *file);
uint8_t VFile_ReadU8(VFILE *file);
uint16_t VFile_ReadU16(VFILE *file);
uint32_t VFile_ReadU32(VFILE *file);
