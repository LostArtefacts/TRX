#version 120
#extension GL_ARB_explicit_attrib_location: enable

varying vec2 vertTexCoords;

layout(location = 0) out vec4 fragColor;

uniform sampler2D tex0;

void main(void) {
    fragColor = texture2D(tex0, vertTexCoords);
}
