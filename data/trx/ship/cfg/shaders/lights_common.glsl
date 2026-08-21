#define MAX_LIGHTS 32

#define RLM_NORMAL  0
#define RLM_FLICKER 1
#define RLM_GLOW    2
#define RLM_SUNSET  3

uniform int uWaterEffect;
uniform vec3 uWaterEffectParams; // x=choppy amp, y=shimmer amp, z=abs intensity

struct Light {
    vec4 pos;
    vec4 color;
    float shade;
    float falloff;
    float kind;
    float _pad0;
};

layout(std140) uniform Lights {
    int uNumLights;
    int uRoomLightMode;
    Light uLights[MAX_LIGHTS];
};

struct LightingResult {
    float shade; // used only for TR1-2
    vec3 add; // TR3+: additive light (dynamic + post effects)
    vec3 mul; // TR3+: multiplicative light (object/own)
};

float ogPhaseTurns(vec3 worldPos, int scheme)
{
    // The lane is a sum of the position, not a hash of the three axes: it
    // steps once every four units of that sum, so vertices near each other
    // share a lane and the light runs in broad bands. Hashing each vertex on
    // its own leaves neighbours unrelated, which reads as speckle.
    float sum = floor(worldPos.x / 64.0) + floor(worldPos.y / 64.0)
        + floor(worldPos.z / 128.0);
    float lane = mod(floor(sum / 4.0), 16.0);

    // stands in for the random byte the OG table holds per lane
    float n =
        fract(sin(lane * 12.9898 + float(scheme) * 19.19) * 43758.5453);
    float offTurns = floor(n * 16.0) / 16.0;

    // time base is uTimeInGame with period 64
    float tTurns = fract(uTimeInGame / 64.0);
    return tTurns + offTurns;
}

float effectChoppy(vec3 worldPos)
{
    int scheme = clamp(uWaterEffect - 2, 0, 21);
    float angle = fract(ogPhaseTurns(worldPos, scheme)) * 2 * PI;
    return -sin(angle) * uWaterEffectParams.x / 2.0;
}

float effectShimmer(vec3 worldPos)
{
    int scheme = clamp(uWaterEffect - 2, 0, 21);
    float angle = fract(ogPhaseTurns(worldPos, scheme)) * 2 * PI;
    return sin(angle) * uWaterEffectParams.y * 8.0;
}

float effectAbs()
{
    return uWaterEffectParams.z * 8.0;
}

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
    float sunsetProgress = clamp(time / max(1, uSunsetDuration), 0.0, 1.0);
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

vec3 safeNormalize(vec3 v)
{
    float len2 = dot(v, v);
    if (len2 <= 0.0) {
        return vec3(0.0);
    }
    return v * inversesqrt(len2);
}

float getDynamicLightContrastMul()
{
    // Returns the dynamic-light contrast multiplier from the minimum shade.
    // LOW: uMinShade = SHADE_NEUTRAL -> 1.0
    // MED: uMinShade = SHADE_HIGH    -> 1.5
    // HIGH:uMinShade = 0             -> 2.0
    return clamp(2.0 - (uMinShade / float(SHADE_NEUTRAL)), 1.0, 2.0);
}
