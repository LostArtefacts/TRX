#include "common.glsl"

uniform vec4 uTint;

#ifdef VERTEX

uniform mat4 uMatModel;
uniform bool uWibbleEffect;

#include "billboard.glsl"
#include "lights.glsl"

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec3 inUVW;
layout(location = 3) in vec4 inTextureSize;
layout(location = 4) in vec2 inTrapezoidRatios;
layout(location = 5) in uint inFlags;
layout(location = 6) in vec4 inColor;
layout(location = 7) in float inShade;
layout(location = 8) in float inReflectivity;
layout(location = 9) in vec2 inUVScroll; // x = V per game frame, y = wrap

out vec4 gEyePos;
out vec3 gNormal;
flat out uint gFlags;
flat out int gTexLayer;
out vec2 gTexUV;
flat out vec4 gAtlasSize;
out vec2 gTrapezoidRatios;
out float gShade;
out vec4 gColor;
out vec3 gAdd;
flat out float gReflectivity;
// Interpolates the PS1 fog factor linearly in screen space, and with it a
// second copy of the texture coordinates for affine mapping.
noperspective out float gFogFactor;
noperspective out vec4 gAffineUV;

vec3 gammaCurve(vec3 rgb, float gamma_exp)
{
    return pow(clamp(rgb, 0.0, 1.0), vec3(gamma_exp));
}

vec3 waterWibble(vec4 worldPosition, vec4 screenPosition)
{
    vec3 ndc = screenPosition.xyz / screenPosition.w;
    vec2 pixelPos = (ndc.xy * 0.5 + 0.5) * uViewportSize;
#if TR_VERSION == 3
    float phases = (uTimeInGame * 0.5 + length(worldPosition.xyz)) * (2.0 * PI / WIBBLE_SIZE);
    float scale = length(uViewportSize) / length(vec2(640.0, 480.0));
    float adjustedWibble = scale;
    pixelPos.y += sin(phases) * adjustedWibble;
#else
    float phases = (uTimeInGame + length(worldPosition.xyz)) * (2.0 * PI / WIBBLE_SIZE);
    pixelPos.x += sin(phases) * MAX_WIBBLE;
    pixelPos.y += cos(phases) * MAX_WIBBLE;
#endif
    // reverse transform
    ndc.xy = (pixelPos / uViewportSize - 0.5) * 2.0;
    return ndc * screenPosition.w;
}

// The PS1 GTE computed screen coordinates as integers, so vertices could only
// land on whole output pixels. Quantizing the clip position to the selected
// simulated pixel grid reproduces the wobble that geometry shows when the
// camera moves.
vec4 vertexSnap(vec4 clipPos)
{
    vec3 ndc = clipPos.xyz / clipPos.w;
    ndc.xy = floor(ndc.xy * uVertexSnapResolution * 0.5)
        / (uVertexSnapResolution * 0.5);
    return vec4(ndc * clipPos.w, clipPos.w);
}

// Returns the PlayStation depth-cue factor from eye distance. The GTE evaluates
// IR0 = DQB + DQA * (h / sz) per vertex, so the blend is linear in 1/z.
// The configured near and far distances solve DQA and DQB.
float ps1FogFactor(float dist)
{
    float near_dist = max(uFogDistance.x, 1.0);
    float far_dist = max(uFogDistance.y, near_dist + 1.0);
    float dqa = 1.0 / far_dist - 1.0 / near_dist;
    return clamp((1.0 / max(dist, 1.0) - 1.0 / near_dist) / dqa, 0.0, 1.0);
}

void main(void) {
    vec4 worldPos = uMatModel * vec4(inPosition.xyz, 1.0);
    vec4 lightWorldPos = worldPos;

    if ((inFlags & VERT_MOVE) != 0u) {
        float waterMul = (uWaterEffect != 0) ? 1.0 : 0.0;
        worldPos.y += effectChoppy(worldPos.xyz) * waterMul;
    }

    if ((inFlags & (VERT_ABS_SPRITE | VERT_BILLBOARD)) != 0u) {
        int lockMode = (inFlags & VERT_ABS_SPRITE) != 0u ? BILLBOARD_LOCK_NONE : uBillboardLockMode;
        gEyePos = offsetBillboard(inPosition.xyz, inNormal.xy, uMatView, uMatModel, uMatProj, lockMode);
    } else {
        gEyePos = uMatView * worldPos;
    }

    // Reflections are sphere-mapped, so the normal has to be in view space for
    // the reflection to track the camera - an object space normal locks it to
    // the object instead. This is what the OG does for TR4, transforming the
    // mesh normals by the view matrix. Normalize because ours arrive as raw
    // int16s rather than the unit vectors the mapping expects.
    gNormal = normalize(mat3(uMatView * uMatModel) * inNormal.xyz);
    gl_Position = uMatProj * gEyePos;
    gl_Position.z += inPosition.w;

    // Apply water wibble effect only to non-sprite vertices
    if (uWibbleEffect && (inFlags & (VERT_NO_WIBBLE | VERT_BILLBOARD)) == 0u) {
        gl_Position.xyz = waterWibble(worldPos, gl_Position);
    }

    // Sprites and billboards keep sub-pixel positioning, so that UI elements
    // and effects don't crawl.
    if (uVertexSnapEnabled != 0
        && (inFlags & (VERT_ABS_SPRITE | VERT_BILLBOARD)) == 0u) {
        gl_Position = vertexSnap(gl_Position);
    }

    gFlags = inFlags;
    gAtlasSize = inTextureSize;
    gTexLayer = (uTexturesEnabled != 0) && (gFlags & VERT_FLAT_SHADED) == 0u ? int(inUVW.z) : -1;
    gTrapezoidRatios = inTrapezoidRatios;
    gTexUV = inUVW.xy;
    if (inUVScroll.y > 0.0) {
        gTexUV.y += mod(uUVScrollTick * inUVScroll.x, inUVScroll.y);
    }
    gReflectivity = inReflectivity;
    gFogFactor = ps1FogFactor(length(gEyePos.xyz));
    if (uTrapezoidFilterEnabled != 0) {
        gTexUV *= inTrapezoidRatios;
    }
    gAffineUV = vec4(gTexUV, gTrapezoidRatios);

    // The vertex diffuse is lit first and then modulated by the texture (or by
    // the flat polygon's palette color). Keep the lighting component separate
    // from the base color so gamma is applied in the right place.
    LightingResult lr =
        light(inShade, gFlags, inNormal.xyz, lightWorldPos, inNormal.w);
    gShade = lr.shade;

    float gamma_exp = 1.0 / ((uGamma / 10.0) * 4.0);

    // Applies PlayStation water tint before the gamma curve and texture
    // modulation, where the GTE combines it with the light register.
    vec3 tintReg =
        uLightingCurve == LIGHTING_CURVE_SATURATE ? uTint.rgb : vec3(1.0);

#if TR_VERSION >= 4
    // The OG engine lights everything in the "128 = neutral" scale: the lit value is
    // doubled and the excess above 1.0 becomes an additive overbright term
    // applied after texturing (CalcColorSplit).
    vec3 lightIn;
    vec3 modulate;
    if ((gFlags & VERT_FLAT_SHADED) != 0u) {
        lightIn = vec3(128.0 / 255.0); // neutral: no lighting, no overbright
        modulate = inColor.rgb;
    } else if (uLightingEnabled == 0) {
        lightIn = vec3(128.0 / 255.0);
        modulate = vec3(1);
    } else {
        if ((gFlags & VERT_USE_OBJECT_LIGHT) != 0u) {
            lightIn = lightObjectsTR4(inNormal.xyz);
            // White for regular meshes; mesh policies may recolor vertices
            // (e.g. the skybox fog gradient faces are painted black).
            modulate = inColor.rgb;
        } else if ((gFlags & VERT_USE_OWN_LIGHT) != 0u) {
            lightIn = lightOwnTR4(inShade);
            modulate = inColor.rgb;
        } else {
            lightIn = inColor.rgb;
            modulate = vec3(1);
        }
    }

    bool overbright = uLightingCurve == LIGHTING_CURVE_OVERBRIGHT;
    bool saturate = uLightingCurve == LIGHTING_CURVE_SATURATE;
    vec3 L = lightIn * (overbright || saturate ? 255.0 / 128.0 : 1.0) + lr.add;
    gAdd = overbright ? max(L - vec3(1.0), vec3(0.0)) * (64.0 / 255.0)
                      : vec3(0.0);
    if ((gFlags & VERT_ADDITIVE) != 0u) {
        // The OG draws additive polys with specular disabled
        // (HWR_DrawSortList drawtype 2), so no overbright excess.
        gAdd = vec3(0.0);
    }
    // Keeps the saturating excess past the gamma curve so texture modulation
    // clips each color channel separately, as the PlayStation does.
    L = min(L * tintReg, vec3(255.0 / 128.0));
    vec3 over = saturate ? max(L - vec3(1.0), vec3(0.0)) : vec3(0.0);
    vec3 lit = gammaCurve(L, gamma_exp) + over;
    gColor = vec4(lit * modulate, inColor.a);
#elif TR_VERSION >= 3
    vec3 lightIn;
    vec3 modulate;
    if ((gFlags & VERT_FLAT_SHADED) == 0u) {
        if (uLightingEnabled == 0) {
            lightIn = vec3(1);
        } else {
            lightIn = inColor.rgb;
        }
        modulate = vec3(1);
    } else {
        lightIn = vec3(1);
        modulate = inColor.rgb;
    }

    // Combine lighting in linear-ish space first: (base + add) * mul
    bool saturate = uLightingCurve == LIGHTING_CURVE_SATURATE;
    vec3 lit;
    gAdd = vec3(0.0);
    if ((gFlags & VERT_OVERBRIGHT) != 0u && uLightingEnabled != 0) {
        // OG "128 = neutral" lighting: inColor is a raw prelit color where
        // 128/255 means full brightness. Doubling it saturates the diffuse
        // part; the excess becomes an additive term applied after texturing
        // (tomb4's CalcColorSplit: diffuse = min(2c, 255), add = (c-128)/2).
        vec3 L = lightIn * (255.0 / 128.0) + lr.add;
        if (saturate) {
            lit = L;
        } else {
            gAdd = max(L - vec3(1.0), vec3(0.0)) * (64.0 / 255.0);
            lit = clamp(L, 0.0, 1.0);
        }
    } else {
        vec3 L = lightIn + lr.add;
        lit = saturate ? L : clamp(L, 0.0, 1.0);
    }
    lit *= lr.mul;

    // Doubles non-overbright TR3 light only when the selected curve uses the
    // OG hardware renderer's 128-neutral scale.
    if (uLightingEnabled != 0 && (gFlags & VERT_OVERBRIGHT) == 0u) {
        if (saturate) {
            lit *= 255.0 / 128.0;
        } else if (uLightingCurve == LIGHTING_CURVE_OVERBRIGHT) {
            vec3 L = lit * (255.0 / 128.0);
            gAdd += max(L - vec3(1.0), vec3(0.0)) * (64.0 / 255.0);
            lit = clamp(L, 0.0, 1.0);
        }
    }

    // Keeps the saturating excess past the gamma curve.
    lit = min(lit * tintReg, vec3(255.0 / 128.0));
    vec3 over = max(lit - vec3(1.0), vec3(0.0));
    lit = gammaCurve(lit, gamma_exp) + over;

    // Apply flat shading AFTER modulation
    gColor = vec4(lit * modulate, inColor.a);
#else
    gAdd = vec3(0.0);
    float shade_mul = 1.0;
    if ((gFlags & VERT_NO_LIGHTING) == 0u) {
        shade_mul = (2.0 - (max(gShade, uMinShade) / SHADE_NEUTRAL));
    }

    // `shade_mul` is roughly in [0..2]. Remap to [0..1], apply the gamma
    // curve, and restore the range. Use sqrt() to limit the effect scope,
    // since we're applying it to the shade (TR1-2) rather than RGB (TR3).
    vec3 mul = gammaCurve(vec3(shade_mul * 0.5), sqrt(gamma_exp)) * 2.0;

    gColor = inColor;
    if ((gFlags & VERT_FLAT_SHADED) == 0u) {
        gColor.rgb = gammaCurve(gColor.rgb, gamma_exp);
    }
    gColor.rgb *= mul;
    // Preserve the >1.0 lighting range until after texturing so TR1/TR2
    // high contrast can still brighten textured geometry.
    gColor.rgb += lr.add;
#endif

    // Applies PlayStation depth cue before texture modulation, as tomb3's
    // transform.cpp does, so fog keeps the surface texture.
    if (uPS1FogEnabled != 0 && (gFlags & VERT_NO_LIGHTING) == 0u
        && uLightingEnabled != 0) {
        gColor.rgb = mix(gColor.rgb, uFogColor.rgb, gFogFactor);
        gAdd *= 1.0 - gFogFactor;
    }
}

#elif defined(FRAGMENT)

uniform sampler2DArray uTexAtlas;
uniform sampler2D uTexEnvMap;
uniform bool uDiscardAlpha;

#if TR_VERSION >= 4
// TR4 reflections sample the env map out of the atlas, from the sprite the OG
// uses for it. uEnvMapLayer is < 0 when the level has no such sprite.
uniform vec2 uEnvMapUV0;
uniform vec2 uEnvMapUV1;
uniform int uEnvMapLayer;
#endif

in vec4 gEyePos;
in vec3 gNormal;
flat in uint gFlags;
flat in int gTexLayer;
in vec2 gTexUV;
flat in vec4 gAtlasSize;
in float gShade;
in vec4 gColor;
in vec3 gAdd;
in vec2 gTrapezoidRatios;
flat in float gReflectivity;
noperspective in float gFogFactor;
noperspective in vec4 gAffineUV;
out vec4 outColor;

vec4 applyFog(vec4 color, float dist)
{
    // Leaves PlayStation depth cue on the vertex color.
    if (uPS1FogEnabled != 0) {
        return color;
    }
    float fogFactor = clamp(
        (dist - uFogDistance.x) / (uFogDistance.y - uFogDistance.x), 0.0, 1.0);
    return mix(color, uFogColor, fogFactor);
}

// Volumetric fog bulbs (port of the OG OmniEffect/OmniFog,
// polyinsert.cpp:530-688), evaluated per fragment in view space. Level
// bulbs push the fragment toward the fog color; FX bulbs (e.g. underwater
// flares) add colored light. TR4 levels carry bulbs of their own; the other
// games see the ones a script asks for.
#define MAX_FOG_BULBS 10

struct FogBulb {
    vec4 pos;    // view space center; w = distance to camera
    vec4 edge;   // view space sphere edge point toward camera; w = sqrad
    vec4 color;  // rgb 0..1; w = density (0..255)
    vec4 params; // x = 1 / sqrad, y = 1 for FX bulbs
};

layout(std140) uniform FogBulbs {
    int uNumFogBulbs;
    FogBulb uFogBulbs[MAX_FOG_BULBS];
};

vec4 applyFogBulbs(vec4 color)
{
    vec3 fxAdd = vec3(0.0);
    float fogAmount = 0.0;
    vec3 fogColor = vec3(0.0);
    for (int i = 0; i < uNumFogBulbs; i++) {
        vec3 p = gEyePos.xyz;
        // Fragments beyond the bulb are pulled back onto the eye-sphere at
        // the bulb's distance, so the fog reads as a volume. The OG projects
        // onto the plane z == dist instead, which is discontinuous across
        // z == pos.z for off-center bulbs - harmless when interpolated from
        // vertices, but a hard seam across nearby geometry per-fragment.
        float fragDist = length(p);
        if (fragDist > uFogBulbs[i].pos.w) {
            p *= uFogBulbs[i].pos.w / fragDist;
        }
        vec3 dP = p - uFogBulbs[i].pos.xyz;
        vec3 dV = uFogBulbs[i].edge.xyz - uFogBulbs[i].pos.xyz;
        float dv2 = dot(dV, dV);
        if (dv2 <= 0.0) {
            continue;
        }
        float t = dot(dP, dV) / dv2;
        if (t < -1.0) {
            continue;
        }
        if (t > 0.0) {
            dP -= t * dV;
        }
        float d2 = dot(dP, dP);
        if (d2 <= 0.0 || d2 >= uFogBulbs[i].edge.w) {
            continue;
        }
        float density = uFogBulbs[i].color.w;
        float val = d2 * uFogBulbs[i].params.x * density;
        if (uFogBulbs[i].params.y != 0.0) {
            fxAdd += (density - val) * uFogBulbs[i].color.rgb * (1.0 / 256.0);
        } else {
            fogAmount += (density - val) * (1.0 / 255.0);
            // Level bulbs blend toward the OG hardware fog color.
            fogColor = uFogBulbs[i].color.rgb;
        }
    }
    // The framebuffer blend is premultiplied alpha, so scale the bulb
    // contributions by the coverage. Translucent fragments - e.g. bilinear
    // filtered chroma-key edges on the horizon mesh - would otherwise
    // receive full-strength fog and ring against the bulb-less sky layers
    // behind them.
    color.rgb = clamp(color.rgb + fxAdd * color.a, 0.0, 1.0);
    color.rgb = mix(color.rgb, fogColor * color.a, clamp(fogAmount, 0.0, 1.0));
    return color;
}

void main(void) {
    vec4 texColor = gColor;

    // Texturing and base color
    if (gTexLayer >= 0) {
        // The PlayStation had no perspective correction: it interpolated the
        // texture coordinates flat across the face, which is what warps its
        // textures as the camera turns.
        bool affine = uAffineMappingEnabled != 0;
        vec2 uv = affine ? gAffineUV.xy : gTexUV;
        vec3 texCoords = vec3(uv.x, uv.y, gTexLayer);
        if (uTrapezoidFilterEnabled != 0) {
            texCoords.xy /= affine ? gAffineUV.zw : gTrapezoidRatios;
        }
        if ((gFlags & VERT_TEX_WRAP) == 0u) {
            texCoords.xy = clampTexAtlas(texCoords.xy, gAtlasSize);
        }
        texColor *= texture(uTexAtlas, texCoords);
    } else {
        texColor.rgb *= texColor.a;
    }

    // Overbright lighting excess, added after texturing (OG specular).
    texColor.rgb += gAdd;

    // Clips saturating light before fog and bulb blending.
    if (uLightingCurve == LIGHTING_CURVE_SATURATE) {
        texColor.rgb = min(texColor.rgb, vec3(1.0));
    }

    // Alpha discard - chroma keying || transparent pixels in the opaque pass
    if (texColor.a <= 0.0
        || (uDiscardAlpha && texColor.a < 0.99
            && (gFlags & VERT_NO_ALPHA_DISCARD) == 0u)) {
        discard;
    }

    // Reflections
    if ((gFlags & VERT_REFLECTIVE) != 0u && uReflectionsEnabled != 0) {
#if TR_VERSION >= 4
        // The normal maps across the env map window. No y-flip here: view
        // space is Y-down (the GL/D3D flip lives in the projection) and the
        // atlas is stored top row first, so this already matches the OG.
        //
        // The OG draws the reflection as a second, purely additive pass over
        // the face (drawtype 2 = ONE/ONE), textured with the env map and
        // modulated by the lit vertex color scaled by the face's reflectivity.
        if (uEnvMapLayer >= 0) {
            vec2 env_uv = mix(uEnvMapUV0, uEnvMapUV1, gNormal.xy * 0.5 + 0.5);
            vec3 env_color = texture(uTexAtlas, vec3(env_uv, uEnvMapLayer)).rgb;
            texColor.rgb += env_color * gColor.rgb * gReflectivity * texColor.a;
        }
#else
        // The env map is a capture of the framebuffer, whose origin is at the
        // bottom left, hence the flip.
        vec2 env_uv = normalize(gNormal).xy * 0.5 + 0.5;
        env_uv.y = 1.0 - env_uv.y;
        texColor.rgb *= texture(uTexEnvMap, env_uv).rgb * 2.0;
#endif
    }

    // Fog
    if ((gFlags & VERT_NO_LIGHTING) == 0u && uLightingEnabled != 0) {
        texColor = applyFog(texColor, length(gEyePos.xyz));
        // The OG skips fog bulbs on additive polys (AddTriClippedSorted
        // excludes drawtypes 2 and 5 from OmniFog).
        if ((gFlags & VERT_ADDITIVE) == 0u) {
            texColor = applyFogBulbs(texColor);
        }
    }

    texColor.rgb *= uBrightnessMultiplier;
    // The framebuffer blend is premultiplied alpha, so the color carries the
    // coverage: fading a fragment out scales both, and the color a second time
    // by the alpha it is premultiplied against.
    if (uLightingCurve == LIGHTING_CURVE_SATURATE) {
        texColor.a *= uTint.a;
    } else {
        texColor *= uTint;
    }
    texColor.rgb *= uTint.a;

    outColor = texColor;
}

#endif
