// TR4 lighting family: colored per-item light lists resolved on the CPU,
// evaluated per vertex the way the retail game had Direct3D light them.
// Values are in the OG "128 = neutral" scale; meshes.glsl doubles the result
// and splits the excess into an additive overbright term (the OG
// diffuse/specular split).

#define MAX_TR4_LIGHTS 48

#define TR4_LIGHT_POINT 0
#define TR4_LIGHT_SPOT 1
#define TR4_LIGHT_SUN 2

struct TR4Light {
    vec4 pos;   // object space; w = type
    vec4 color; // rgb 0..1; w = range
    vec4 dir;   // object space, where a spot points or a sun shines; w = att1
    vec4 cone;  // x = cosine of a spot's cone edge
};

layout(std140) uniform LightSource {
    vec4 uTR4Ambient; // 0..1 where 128/255 is the OG neutral
    int uTR4NumLights;
    TR4Light uTR4Lights[MAX_TR4_LIGHTS];
};

// Lights a vertex as the retail game's Direct3D lights did: in object space,
// from the vertex itself, with the legacy attenuation att1 * (1 - d / range).
// Direct3D wrote the lit color in 8 bits, so the sum stops at 255.
vec3 lightObjectsTR4(vec3 rawNormal, vec3 pos, vec3 ambient)
{
    vec3 N = safeNormalize(rawNormal);

    vec3 acc = ambient;
    for (int i = 0; i < uTR4NumLights; i++) {
        TR4Light light = uTR4Lights[i];
        int type = int(light.pos.w);
        vec3 L;
        float atten = 1.0;
        if (type == TR4_LIGHT_SUN) {
            L = -safeNormalize(light.dir.xyz);
        } else {
            vec3 d = light.pos.xyz - pos;
            float dist = length(d);
            if (dist <= 0.0 || dist > light.color.w) {
                continue;
            }
            L = d / dist;
            atten = light.dir.w * (1.0 - dist / light.color.w);
            if (type == TR4_LIGHT_SPOT
                && dot(-L, safeNormalize(light.dir.xyz)) <= light.cone.x) {
                continue;
            }
        }
        float val = dot(N, L);
        if (val > 0.0) {
            acc += (val * atten) * light.color.rgb;
        }
    }
    return min(acc, vec3(1.0));
}

// Port of the OG ProcessStaticMeshVertices' prelight modulation
// (output.cpp:305): vertex prelight (13-bit shade) times the per-instance
// shade multiplier carried in uTR4Ambient.
vec3 lightOwnTR4(float shade)
{
    float shade8 = clamp(floor((SHADE_MAX - shade) / 32.0), 0.0, 255.0);
    return uTR4Ambient.rgb * (shade8 / 255.0) * (255.0 / 256.0);
}

// Port of the OG room dynamic lighting (drawroom.cpp:185-210). Returns the
// contribution in the "128 = neutral" L scale. Note the OG quirk: the
// vertex normal is rotated into view space but dotted with the room-space
// light direction, and the (1 - dot) term brightens faces pointing away
// from the light. Kept faithfully.
vec3 lightDynamicRoomTR4(vec3 rawNormal, vec4 vertexPos)
{
    vec3 add = vec3(0.0);
    vec3 nView = mat3(uMatView) * safeNormalize(rawNormal / float(1 << 14));
    for (int i = 0; i < uNumLights; i++) {
        float falloff = uLights[i].falloff * 0.625; // (f >> 1) + (f >> 3)
        if (falloff <= 0.0) {
            continue;
        }
        vec3 d = vertexPos.xyz - uLights[i].pos.xyz;
        float distSq = dot(d, d);
        if (distSq >= falloff * falloff) {
            continue;
        }
        float dist = sqrt(distSq);
        float val2 = (falloff - dist) / falloff;
        float val = val2 * (1.0 - dot(nView, d / dist));
        add += uLights[i].color.rgb * val;
    }
    return add;
}

// Port of the tomb4 static mesh dynamic lighting (output.cpp:314-338):
// cheaper falloff (1.7x distance), no Lambert term. This is a tomb4
// addition gated behind its static_lighting option; the retail game lit
// statics by prelight only.
vec3 lightDynamicStaticTR4(vec4 vertexPos)
{
    vec3 add = vec3(0.0);
    for (int i = 0; i < uNumLights; i++) {
        float falloff = uLights[i].falloff;
        if (falloff <= 0.0) {
            continue;
        }
        float dist = length(uLights[i].pos.xyz - vertexPos.xyz) * 1.7;
        if (dist > falloff) {
            continue;
        }
        add += ((falloff - dist) / falloff) * uLights[i].color.rgb;
    }
    // the OG adds val2 * color(0..255) onto the 128-neutral channels.
    return add * (255.0 / 128.0);
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
        result.add += lightDynamicRoomTR4(normal, pos);
    } else if (
        (flags & VERT_USE_OWN_LIGHT) != 0u && uStaticLightingEnabled != 0) {
        result.add += lightDynamicStaticTR4(pos);
    }

    // Water shimmer, on the 0 to 255 scale of the room color. A water vertex
    // (0x2000) darkens by the height the choppy table lifted it, and a shore
    // vertex (0x4000) takes the shimmer and the absolute term together. The
    // two classes never share a term.
    float add = 0.0;
    if ((flags & VERT_MOVE) != 0u) {
        add += effectChoppy(pos.xyz) / 256.0;
    }
    if ((flags & VERT_GLOW) != 0u) {
        add += effectShimmer(pos.xyz) / 256.0 + effectAbs() / 256.0;
    }
    result.add += vec3(add);

    // The added light stands on its own and never cuts into the room color.
    // The OG sums the dynamic light and the shimmer, clamps that sum at zero,
    // and only then adds the room color on top, so a shimmer that swings
    // negative cancels the lights and stops there.
    result.add = max(result.add, vec3(0.0));

    return result;
}
