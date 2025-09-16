#define EFFECT_NONE 0
#define EFFECT_VIGNETTE 1
#define EFFECT_WAVE 2

#define WAVE_SPEED_SHORT -3.92
#define WAVE_SPEED_LONG -2.81
#define WAVE_TILE_PHASE_SHORT vec2(67.5, 73.0)
#define WAVE_TILE_PHASE_LONG vec2(33.94, 28.31)
#define WAVE_LIGHT_DELTA 0.125
#define WAVE_Y_TILES 6
#define WAVE_ORBIT_RADIUS 0.2
#define WAVE_FPS_DRIFT 25 / 30

#ifdef VERTEX

uniform int uEffect;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoords;

out vec2 vertTexCoords;
out vec2 vertCoords;
out float vertLight;

void main() {
    if ((uEffect & EFFECT_WAVE) != 0) {
        float edgeOffset = (1.0 / WAVE_Y_TILES) * 2.0;
        vec2 baseNDC = ((inPosition.xy * (2.0 + 2.0 * edgeOffset)) - (1.0 + edgeOffset)) * vec2(1.0, -1.0);

        vec2 aspectCorrection = vec2(uViewportSize.y / uViewportSize.x, 1);
        vec2 repeat = float(WAVE_Y_TILES) / aspectCorrection;
        float shortPhase = dot(inPosition, repeat * WAVE_TILE_PHASE_SHORT);
        float longPhase = dot(inPosition, repeat * WAVE_TILE_PHASE_LONG);
        float shortAng = radians((uTime * WAVE_FPS_DRIFT)  * WAVE_SPEED_SHORT + shortPhase);
        float longAng = radians((uTime * WAVE_FPS_DRIFT) * WAVE_SPEED_LONG + longPhase);

        float viewportSizeNDC = (1 + edgeOffset * 2);
        vec2 tileSize = viewportSizeNDC / repeat;
        vec2 vertexOffset = vec2(cos(shortAng), sin(shortAng)) * tileSize * WAVE_ORBIT_RADIUS;
        vertLight = 0.5 + (sin(shortAng) + sin(longAng)) * WAVE_LIGHT_DELTA;

        gl_Position  = vec4(baseNDC + vertexOffset, 0.0, 1.0);
    } else {
        vec2 baseNDC = inPosition * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
        gl_Position = vec4(baseNDC, 0.0, 1.0);
    }

    vertCoords    = inPosition;
    vertTexCoords = inTexCoords;
}

#elif defined(FRAGMENT)

uniform sampler2D texMain;
uniform vec4 uTexSize;
uniform int uEffect;

in vec2 vertTexCoords;
in vec2 vertCoords;
in float vertLight;
out vec4 outColor;

void main(void) {
    vec2 uv = vertTexCoords;
    uv = clampTexAtlas(uv, uTexSize);

    outColor = texture(texMain, uv);

    if ((uEffect & EFFECT_WAVE) != 0) {
        outColor.rgb *= vertLight;
    } else if ((uEffect & EFFECT_VIGNETTE) != 0) {
        float x_dist = vertCoords.x - 0.5;
        float y_dist = vertCoords.y - 0.5;
        float lightV = 256.0 - sqrt(x_dist * x_dist + y_dist * y_dist) * 300.0;
        lightV = clamp(lightV, 0.0, 255.0) / 255.0;
        outColor *= vec4(lightV, lightV, lightV, 1.0);
    }

    outColor.rgb *= uBrightnessMultiplier;
}
#endif
