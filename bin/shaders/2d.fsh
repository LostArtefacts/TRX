#version 330 core

in vec2 vertTexCoords;

layout(location = 0) out vec4 fragColor;

uniform sampler2D tex0;

void main(void) {
    fragColor = texture(tex0, vertTexCoords);
}
