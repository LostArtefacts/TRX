#ifdef VERTEX
// Vertex shader

layout(location = 0) in vec2 inPosition;

#ifdef OGL33C
    out vec2 vertTexCoords;
#else
    varying vec2 vertTexCoords;
#endif

void main(void) {
    vertTexCoords = inPosition;
    gl_Position = vec4(vertTexCoords * vec2(2.0, 2.0) + vec2(-1.0, -1.0), 0.0, 1.0);
}

#else
// Fragment shader

uniform sampler2D tex0;

#ifdef OGL33C
    #define OUTCOLOR outColor
    #define TEXTURE texture

    in vec2 vertTexCoords;
    out vec4 OUTCOLOR;
#else
    #define OUTCOLOR gl_FragColor
    #define TEXTURE texture2D

    varying vec2 vertTexCoords;
#endif

void main(void) {
    OUTCOLOR = TEXTURE(tex0, vertTexCoords);
}
#endif // VERTEX
