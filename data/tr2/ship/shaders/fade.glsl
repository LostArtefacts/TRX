#ifdef VERTEX
// Vertex shader

layout(location = 0) in vec2 inPosition;

void main(void) {
    gl_Position = vec4(inPosition * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);
}

#else
// Fragment shader

uniform float opacity;

void main(void) {
    OUTCOLOR = vec4(0, 0, 0, opacity);
}
#endif // VERTEX
