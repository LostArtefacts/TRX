#version 120
#extension GL_ARB_explicit_attrib_location: enable

layout(location = 0) in vec2 inPosition;

out vec2 vertTexCoords;

void main(void) {
    vertTexCoords = inPosition;
    gl_Position = vec4(vertTexCoords * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);
}
