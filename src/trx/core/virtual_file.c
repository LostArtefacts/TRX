#include <trx/core/virtual_file.h>

#include <trx/core/filesystem.h>
#include <trx/core/memory.h>

#define M_DEFINE_READ(name_, inner_, type_)                                    \
    type_ name_(VFILE *const file)                                             \
    {                                                                          \
        return inner_(file);                                                   \
    }

#define M_DEFINE_TRY_READ(name_, inner_, type_)                                \
    bool name_(VFILE *const file, type_ *const dst)                            \
    {                                                                          \
        return inner_(file, dst);                                              \
    }

VFILE *VFile_CreateFromPath(const char *const path)
{
    char *data = nullptr;
    size_t size = 0;
    if (!File_Load(path, &data, &size)) {
        return nullptr;
    }
    VFILE *const file = File_OpenBuffer(data, size);
    Memory_FreePointer(&data);
    return file;
}

VFILE *VFile_CreateFromBuffer(const char *const data, const size_t size)
{
    return File_OpenBuffer(data, size);
}

void VFile_Close(VFILE *const file)
{
    File_Close(file);
}

size_t VFile_GetPos(const VFILE *const file)
{
    return File_Pos(file);
}

void VFile_SetPos(VFILE *const file, const size_t pos)
{
    File_Seek(file, pos, FILE_SEEK_SET);
}

void VFile_Skip(VFILE *const file, const int32_t offset)
{
    File_Skip(file, offset);
}

bool VFile_TrySkip(VFILE *const file, const int32_t offset)
{
    return File_TrySkip(file, offset);
}

void VFile_Read(VFILE *const file, void *const target, const size_t size)
{
    File_ReadData(file, target, size);
}

bool VFile_TryRead(VFILE *const file, void *const target, const size_t size)
{
    return File_TryReadData(file, target, size);
}

M_DEFINE_READ(VFile_ReadS8, File_ReadS8, int8_t)
M_DEFINE_READ(VFile_ReadS16, File_ReadS16, int16_t)
M_DEFINE_READ(VFile_ReadS32, File_ReadS32, int32_t)
M_DEFINE_READ(VFile_ReadU8, File_ReadU8, uint8_t)
M_DEFINE_READ(VFile_ReadU16, File_ReadU16, uint16_t)
M_DEFINE_READ(VFile_ReadU32, File_ReadU32, uint32_t)
M_DEFINE_READ(VFile_ReadFloat, File_ReadFloat, float)
M_DEFINE_READ(VFile_ReadDouble, File_ReadDouble, double)

M_DEFINE_TRY_READ(VFile_TryReadS8, File_TryReadS8, int8_t)
M_DEFINE_TRY_READ(VFile_TryReadS16, File_TryReadS16, int16_t)
M_DEFINE_TRY_READ(VFile_TryReadS32, File_TryReadS32, int32_t)
M_DEFINE_TRY_READ(VFile_TryReadU8, File_TryReadU8, uint8_t)
M_DEFINE_TRY_READ(VFile_TryReadU16, File_TryReadU16, uint16_t)
M_DEFINE_TRY_READ(VFile_TryReadU32, File_TryReadU32, uint32_t)
