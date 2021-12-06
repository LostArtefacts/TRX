#version 330 core
#extension GL_EXT_gpu_shader4: enable

in vec4 vertColor;
in vec3 vertTexCoords;

layout(location = 0) out vec4 fragColor;

uniform sampler2D tex0;
uniform vec3 chromaKey;
uniform bool tmapEn;

void main(void) {
    fragColor = vertColor;

    // texturing
    if (tmapEn) {
        // chroma keying
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

        // texture mapping
        vec4 texColor = texture(tex0, vertTexCoords.xy / vertTexCoords.z);
        if (texColor.a == 0.0) {
            discard;
        }

        // texture lighting
        fragColor = vec4(fragColor.rgb * texColor.rgb, texColor.a);
    }
}
