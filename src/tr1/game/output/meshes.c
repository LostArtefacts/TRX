#include "game/output/meshes.h"

static OUTPUT_SHADER *m_Shader = nullptr;

void Output_Meshes_Init(void)
{
    m_Shader = Output_Shader_Create("shaders/meshes.glsl");
}

void Output_Meshes_Shutdown(void)
{
    Output_Shader_Free(m_Shader);
    m_Shader = nullptr;
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
