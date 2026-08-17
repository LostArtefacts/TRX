#include <trx/game/output/shaders/ui.h>

#include <trx/gl/utils.h>

RESULT Output_UIShader_Create(OUTPUT_UI_SHADER **const out_shader)
{
    *out_shader = nullptr;
    OUTPUT_SHADER *shader = nullptr;
    MUST(Output_Shader_Create("ui.glsl", &shader));
    TRX_GL_TRACK_UNIFORM(
        glUniform1i, Output_Shader_LookupUniform(shader, "uTexAtlas"), 0);
    *out_shader = shader;
    return OK;
}

void Output_UIShader_Bind(const OUTPUT_UI_SHADER *const shader)
{
    Output_Shader_Bind(shader);
}

void Output_UIShader_Free(OUTPUT_UI_SHADER *const shader)
{
    Output_Shader_Free(shader);
}
