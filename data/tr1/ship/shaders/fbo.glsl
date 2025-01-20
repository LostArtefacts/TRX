#ifdef VERTEX
// Vertex shader

layout(location = 0) in vec2 inPosition;

OUT vec2 vertTexCoords;

void main(void) {
    vertTexCoords = inPosition;
    gl_Position = vec4(vertTexCoords * vec2(2.0, 2.0) + vec2(-1.0, -1.0), 0.0, 1.0);
}

#else
// Fragment shader

uniform sampler2D tex0;

IN vec2 vertTexCoords;

void main(void) {
    OUTCOLOR = TEXTURE2D(tex0, vertTexCoords);
}
#endif // VERTEX
