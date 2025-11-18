#define MAX_LIGHTS 32

#define RLM_NORMAL  0
#define RLM_FLICKER 1
#define RLM_GLOW    2
#define RLM_SUNSET  3

uniform bool uWaterEffect;

struct Light {
    vec4 pos;
    int shade;
    int falloff;
};

layout(std140) uniform Lights {
    int uNumLights;
    int uRoomLightMode;
    Light uLights[MAX_LIGHTS];
};

int lightFlicker(float t) {
    float h = fract(sin(t * 593.123) * 43758.5453);
    return int(h * 32.0);
}

int lightGlow(float time) {
    float phase = mod(time, 32.0) / 32.0;
    float s = sin(phase * 2 * PI);
    float normalized = (s + 1.0) * 0.5;
    return int(normalized * 31.0);
}

int lightSunset(float time) {
    float sunsetProgress = clamp(time / uSunsetDuration, 0.0, 1.0);
    return int(sunsetProgress * 31.0);
}

int calcRoomShadeIndex(int mode, float time)
{
    if (mode == RLM_FLICKER) {
        return lightFlicker(time);
    }
    if (mode == RLM_GLOW) {
        return lightGlow(time);
    }
    if (mode == RLM_SUNSET) {
        return lightSunset(time);
    }
    return 0;
}

float lightRoom(
    int lightMode, float time, float vertexPhase)
{
    int i = calcRoomShadeIndex(lightMode, time);
    float j = float(int(vertexPhase) & 31);

    const float MAX_UNIT = 512.0;
    return (j - 16.0) * float(i) * MAX_UNIT / 31.0;
}

float lightWaterCaustics(float shade, vec3 vtxPos)
{
    float time = mod(float(uTimeInGame), float(WIBBLE_SIZE));
    // just a random offset based on the source vertex
    float caustic = fract(sin(dot(vtxPos.xyz, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
    caustic = (caustic * 1023.0) - 511.0;
    float angle = radians(360.0 * mod((time + caustic) / float(WIBBLE_SIZE), 1.0));
    return clamp(shade + sin(angle) * float(SHADE_CAUSTICS), 0.0, float(SHADE_MAX));
}

float lightDynamic(float baseLight, vec4 vertexPos)
{
    float lightAdder = baseLight;
    for (int i = 0; i < uNumLights; i++) {
        Light light = uLights[i];
        vec3 dist = light.pos.xyz - vertexPos.xyz;
        float radius = float(1 << light.falloff);
        if (any(greaterThan(abs(dist), vec3(radius)))) {
            continue;
        }

        float distSq = dot(dist, dist);
        if (distSq > radius * radius) {
            continue;
        }

        float maxShade = float(1 << light.shade);
        float distTerm = distSq / float(1 << (2 * light.falloff - light.shade));
        float shade = maxShade - distTerm;
        lightAdder -= shade;
    }
    return max(lightAdder, 0);
}

float light(float shade, uint flags, vec3 normal, vec4 pos, float phase)
{
    if (uWaterEffect) {
        shade = lightWaterCaustics(shade, pos.xyz);
    }

    if ((flags & VERT_USE_DYNAMIC_LIGHT) != 0u) {
        shade = lightDynamic(shade, pos);
        shade += lightRoom(uRoomLightMode, uTimeInGame, phase);
    }

    return shade;
}
