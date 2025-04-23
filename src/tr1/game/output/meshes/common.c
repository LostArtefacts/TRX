#include "game/output/meshes/common.h"

#include "game/output/meshes/dynamic.h"
#include "game/output/meshes/objects.h"
#include "game/output/meshes/rooms.h"

static OUTPUT_SHADER *m_Shader = nullptr;

void Output_Meshes_Init(void)
{
    m_Shader = Output_Shader_Create("shaders/meshes.glsl");
    Output_Meshes_InitDynamic();
    Output_Meshes_InitRooms();
    Output_Meshes_InitObjects();
}

void Output_Meshes_Shutdown(void)
{
    Output_Meshes_ShutdownObjects();
    Output_Meshes_ShutdownRooms();
    Output_Meshes_ShutdownDynamic();
    Output_Shader_Free(m_Shader);
    m_Shader = nullptr;
}

void Output_Meshes_ObserveLevelLoad(void)
{
    Output_Meshes_ObserveLevelLoadObjects();
    Output_Meshes_ObserveLevelLoadRooms();
}

void Output_Meshes_ObserveLevelUnload(void)
{
    Output_Meshes_ObserveLevelUnloadObjects();
}

void Output_Meshes_RenderBegin(void)
{
    Output_Shader_UploadCommonUniforms(m_Shader);
    Output_Shader_UploadProjectionMatrix(m_Shader);
}

OUTPUT_SHADER *Output_Meshes_GetShader(void)
{
    return m_Shader;
}

void Output_Meshes_UploadProjectionMatrix(void)
{
    Output_Shader_UploadProjectionMatrix(m_Shader);
}
