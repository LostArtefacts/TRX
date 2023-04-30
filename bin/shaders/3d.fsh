#version 120
#extension GL_EXT_gpu_shader4: enable

varying vec4 vertColor;
varying vec3 vertTexCoords;

uniform sampler2D tex0;
uniform bool texturingEnabled;
uniform bool smoothingEnabled;

void main(void) {
    gl_FragColor = vertColor;

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

        gl_FragColor = vec4(gl_FragColor.rgb * texColor.rgb, texColor.a);
    }
}
