// TR3 lighting family: ambient + up to 3 directional lights (sun, brightest
// bulb, brightest dynamic), resolved per item on the CPU.

layout(std140) uniform LightSource {
    vec4 uTR3Ambient;
    vec4 uTR3LightDirView[3];
    vec4 uTR3LightColor[3];
};

vec3 lightObjectsTR3(vec3 rawNormal)
{
    vec3 N = safeNormalize(mat3(uMatView * uMatModel) * (rawNormal.xyz / float(1 << 14)));

    vec3 L0 = uTR3LightDirView[0].xyz;
    vec3 L1 = uTR3LightDirView[1].xyz;
    vec3 L2 = uTR3LightDirView[2].xyz;

    float d0 = max(dot(N, L0), 0.0);
    float d1 = max(dot(N, L1), 0.0);
    float d2 = max(dot(N, L2), 0.0);

    vec3 rgb = uTR3Ambient.rgb
        + uTR3LightColor[0].rgb * d0
        + uTR3LightColor[1].rgb * d1
        + uTR3LightColor[2].rgb * d2;
    return clamp(rgb, 0.0, 1.0);
}

vec3 lightOwnTR3(float shade)
{
    float shade8 = floor((SHADE_MAX - shade) / 32.0); // (0x1FFF - shade) >> 5
    shade8 = (shade8 <= 0.0) ? 255.0 : shade8;
    return clamp(uTR3Ambient.rgb * (shade8 / 255.0), 0.0, 1.0);
}

vec3 lightDynamicTR3(vec4 vertexPos)
{
    vec3 add = vec3(0.0);
    for (int i = 0; i < uNumLights; i++) {
        float radius = uLights[i].falloff * 0.5; // falloff_raw >> 1
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

    if ((flags & VERT_USE_DYNAMIC_LIGHT) != 0u) {
        result.add += lightDynamicTR3(pos);
    }

    if ((flags & VERT_USE_OBJECT_LIGHT) != 0u) {
        result.mul *= lightObjectsTR3(normal);
    } else if ((flags & VERT_USE_OWN_LIGHT) != 0u) {
        result.mul *= lightOwnTR3(shade);
    }

    float add = 0.0;
    if ((flags & VERT_MOVE) != 0u) {
        add += effectChoppy(pos.xyz) / 256.0;
    }
    if ((flags & VERT_GLOW) != 0u) {
        add += effectShimmer(pos.xyz) / 256.0;
        add += effectAbs() / 256.0;
    }
    result.add += vec3(add);

    return result;
}
