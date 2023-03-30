#version 120
#extension GL_ARB_explicit_attrib_location: enable
#extension GL_EXT_gpu_shader4: enable

in vec4 vertColor;
in vec3 vertTexCoords;

layout(location = 0) out vec4 fragColor;

uniform sampler2D tex0;
uniform bool texturingEnabled;
uniform bool smoothingEnabled;

void main(void) {
    fragColor = vertColor;

    if (texturingEnabled) {
#ifdef GL_EXT_gpu_shader4
        if (smoothingEnabled) {
            // do not use smoothing for chroma key
            ivec2 size = textureSize2D(tex0, 0);
            int tx = int((vertTexCoords.x / vertTexCoords.z) * size.x) % size.x;
            int ty = int((vertTexCoords.y / vertTexCoords.z) * size.y) % size.y;
            vec4 texel = texelFetch2D(tex0, ivec2(tx, ty), 0);
            if (texel.a == 0.0) {
                discard;
            }
        }
#endif

        vec4 texColor = texture2D(tex0, vertTexCoords.xy / vertTexCoords.z);
        if (texColor.a == 0.0) {
            discard;
        }

        fragColor = vec4(fragColor.rgb * texColor.rgb, texColor.a);
    }
}
