#version 330 core
#extension GL_EXT_gpu_shader4: enable

// ATI3DCIF enums

// C3D_ESHADE
#define C3D_ESH_NONE            0   // shading mode is undefined
#define C3D_ESH_SOLID           1   // shade using the clrSolid from the RC
#define C3D_ESH_FLAT            2   // shade using the last vertex to flat shade
#define C3D_ESH_SMOOTH          3   // shade using linearly interpolating vert clr

// C3D_ETEXOP
#define C3D_ETEXOP_NONE         0    // 
#define C3D_ETEXOP_CHROMAKEY    1    // select texels not equal to the chroma key
#define C3D_ETEXOP_ALPHA        2    // pass texel alpha to the alpha blender
#define C3D_ETEXOP_ALPHA_MASK   3    // lw bit 0: tex not drawn otw: alpha int

// C3D_ETLIGHT
#define C3D_ETL_NONE            0    //  TEXout = Tclr
#define C3D_ETL_MODULATE        1    //  TEXout = Tclr*CInt
#define C3D_ETL_ALPHA_DECAL     2    //  TEXout = (Tclr*Talp)+(CInt*(1-Talp))
#define C3D_ETL_NUM             3    //  invalid enumeration

in vec4 vertColor;
flat in vec4 vertColorFlat;
in vec3 vertTexCoords;

layout(location = 0) out vec4 fragColor;

uniform sampler2D tex0;
uniform vec4 solidColor;
uniform vec3 chromaKey;
uniform int shadeMode;
uniform bool tmapEn;
uniform int tmapLight;
uniform int texOp;

void main(void) {
    // discard fragment if there's no shading mode and no texture
    if (shadeMode == C3D_ESH_NONE && !tmapEn) {
        discard;
    }

    // shading
    switch (shadeMode) {
        case C3D_ESH_SOLID:
            fragColor = solidColor;
            break;

        case C3D_ESH_FLAT:
            fragColor = vertColorFlat;
            break;

        case C3D_ESH_SMOOTH:
            fragColor = vertColor;
            break;
    }

    // texturing
    if (tmapEn) {
        // chroma keying
        if (texOp == C3D_ETEXOP_CHROMAKEY) {
            // fetch raw texel for fragment
            ivec2 size = textureSize2D(tex0, 0);
            int tx = int((vertTexCoords.x / vertTexCoords.z) * size.x) % size.x;
            int ty = int((vertTexCoords.y / vertTexCoords.z) * size.y) % size.y;
            vec4 texel = texelFetch(tex0, ivec2(tx, ty), 0);

            // discard fragment if texel matches chroma key
            float diff = abs(distance(texel.rgb, chromaKey));
            if (diff == 0) {
                discard;
            }
        }

        // texture mapping
        vec4 texColor = texture(tex0, vertTexCoords.xy / vertTexCoords.z);
        if (texColor.a == 0.0) {
            discard;
        }

        // texture lighting
        switch (tmapLight) {
            case C3D_ETL_NONE:
                fragColor = texColor;
                break;

            case C3D_ETL_MODULATE:
                fragColor = vec4(fragColor.rgb * texColor.rgb, texColor.a);
                break;

            case C3D_ETL_ALPHA_DECAL:
                fragColor = vec4((texColor.rgb * texColor.a) + (fragColor.rgb * (1.0 - texColor.a)), 1.0);
                break;
        }
    }
}
