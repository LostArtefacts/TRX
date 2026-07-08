// TR1/TR2 lighting family: a single scalar shade derived from the brightest
// room light, plus additive dynamic lights.

layout(std140) uniform LightSource {
    float uLightAdder;
    float uLightDivider;
    vec4 uLightVectorSource;
};

float lightObjects(vec3 rawNormal, vec4 vertexPos)
{
    float lightAdder = uLightAdder;
    if (uLightDivider != 0) {
        vec3 L = mat3(transpose(uMatView * uMatModel)) * uLightVectorSource.xyz / uLightDivider;
        lightAdder += dot(L, rawNormal.xyz / (1 << 14)) / 4;
        lightAdder = clamp(lightAdder, 0, SHADE_MAX);
    }
    return lightAdder;
}

float lightDynamicTR12Lum(float baseLight, vec4 vertexPos)
{
    float lightAdder = baseLight;
    for (int i = 0; i < uNumLights; i++) {
        if (uLights[i].kind != 0.0) {
            continue;
        }
        vec3 dist = uLights[i].pos.xyz - vertexPos.xyz;
        float radius = exp2(uLights[i].falloff);
        float distSq = dot(dist, dist);
        if (distSq > radius * radius) {
            continue;
        }

        float maxShade = exp2(uLights[i].shade);
        float distTerm = distSq / exp2(2 * uLights[i].falloff - uLights[i].shade);
        float shade = maxShade - distTerm;
        lightAdder -= shade;
    }
    return max(lightAdder, 0);
}

vec3 lightDynamicTR12RGB(vec4 vertexPos)
{
    vec3 add = vec3(0.0);
    for (int i = 0; i < uNumLights; i++) {
        if (uLights[i].kind == 0.0) {
            continue;
        }

        float radius = uLights[i].falloff * 0.5;
        vec3 dist = uLights[i].pos.xyz - vertexPos.xyz;
        float distSq = dot(dist, dist);
        float radiusSq = radius * radius;
        if (distSq > radiusSq) {
            continue;
        }

        float d = sqrt(distSq);
        float factor = (radius - d) / max(radius, 1.0);
        add += factor * uLights[i].color.rgb;
    }
    return add;
}

float lightLumTR12(float shade, uint flags, vec3 normal, vec4 pos, float phase)
{
    if ((flags & VERT_USE_OWN_LIGHT) != 0u) {
        shade = uLightAdder + shade;
    } else if ((flags & VERT_USE_OBJECT_LIGHT) != 0u) {
        shade = lightObjects(normal, pos);
    } else {
        if ((flags & VERT_USE_DYNAMIC_LIGHT) != 0u) {
            shade = lightDynamicTR12Lum(shade, pos);
            shade += lightRoom(uRoomLightMode, uTimeInGame, phase);
        }
        shade = clamp(shade, 0, SHADE_MAX);
    }

    if (uWaterEffect == 1) {
        shade = lightWaterCaustics(shade, pos.xyz);
    }

    return shade;
}

LightingResult light(
    float shade, uint flags, vec3 normal, vec4 pos,
    float vertexPhase)
{
    LightingResult result;
    result.shade = SHADE_NEUTRAL;
    result.add = vec3(0.0);
    result.mul = vec3(1.0);

    if (uLightingEnabled == 0) {
        return result;
    }
    if ((flags & VERT_NO_LIGHTING) != 0u) {
        return result;
    }

    result.shade = lightLumTR12(shade, flags, normal, pos, vertexPhase);
    if ((flags & VERT_USE_DYNAMIC_LIGHT) != 0u) {
        result.add += lightDynamicTR12RGB(pos) * getDynamicLightContrastMul();
    }

    return result;
}
