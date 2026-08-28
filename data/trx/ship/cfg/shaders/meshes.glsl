#include "common.glsl"

uniform vec4 uTint;

// One definition of what the mesh stages hand each other. The geometry stage
// is optional, so the vertex stage names its block after whichever stage reads
// it: a block is matched across stages by its name, and a stage cannot declare
// an input block and an output block under one name.
#define MESH_VARYINGS             \
    vec4 eyePos;                  \
    vec3 normal;                  \
    flat uint flags;              \
    flat int texLayer;            \
    vec2 texUV;                   \
    flat vec4 atlasSize;          \
    vec2 trapezoidRatios;         \
    vec4 color;                   \
    vec3 add;                     \
    float tintWeight;             \
    flat float reflectivity;      \
    noperspective vec4 affineUV;

#ifdef SUBDIVIDE
    #define MESH_VS_BLOCK VertexData
#else
    #define MESH_VS_BLOCK GeomData
#endif

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

out MESH_VS_BLOCK {
    MESH_VARYINGS
} gOut;

vec3 gammaCurve(vec3 rgb, float gamma_exp)
{
    return pow(clamp(rgb, 0.0, 1.0), vec3(gamma_exp));
}

// Grows the distortion with the display, so that it stays visible where the
// original left it at the two pixels a 640x480 screen was given. A sharp
// screen shows the same share of the picture as a stronger distortion, so the
// growth tapers rather than tracking the height. Supersampling raises the
// render target alone and must not change what reaches the screen.
float wibbleScale(void)
{
    float ss = float(uSupersamplingFactor);
    return sqrt((uViewportSize.y / ss) / 480.0) * ss;
}

float wibbleTable(float phase, float pos, float scale)
{
    float idx =
        mod(floor(phase / 8.0 + pos / (8.0 * scale)), float(WIBBLE_SIZE));
    return float(MAX_WIBBLE) * scale * sin(idx * (2.0 * PI / float(WIBBLE_SIZE)));
}

vec3 waterWibble(vec4 worldPosition, vec4 screenPosition)
{
    vec3 ndc = screenPosition.xyz / screenPosition.w;
    vec2 pixelPos = (ndc.xy * 0.5 + 0.5) * uViewportSize;
#if TR_VERSION == 3
    float phases = (uTimeInGame * 0.5 + length(worldPosition.xyz)) * (2.0 * PI / WIBBLE_SIZE);
    pixelPos.y += sin(phases) * wibbleScale();
#elif TR_VERSION == 4
    float phase = uTimeInGame * 4.0;
    float scale = wibbleScale();
    vec2 srcPos = pixelPos;
    pixelPos.x += wibbleTable(phase, srcPos.y, scale);
    pixelPos.y += wibbleTable(phase, srcPos.x, scale);
#else
    float phases = (uTimeInGame + length(worldPosition.xyz)) * (2.0 * PI / WIBBLE_SIZE);
    float amplitude = MAX_WIBBLE * float(uSupersamplingFactor);
    pixelPos.x += sin(phases) * amplitude;
    pixelPos.y += cos(phases) * amplitude;
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
        gOut.eyePos = offsetBillboard(inPosition.xyz, inNormal.xy, uMatView, uMatModel, uMatProj, lockMode);
    } else {
        gOut.eyePos = uMatView * worldPos;
    }

    // Reflections are sphere-mapped, so the normal has to be in view space for
    // the reflection to track the camera - an object space normal locks it to
    // the object instead. This is what the OG does for TR4, transforming the
    // mesh normals by the view matrix. Normalize because ours arrive as raw
    // int16s rather than the unit vectors the mapping expects.
    gOut.normal = normalize(mat3(uMatView * uMatModel) * inNormal.xyz);
    gl_Position = uMatProj * gOut.eyePos;
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

    gOut.flags = inFlags;
    gOut.atlasSize = inTextureSize;
    gOut.texLayer = (uTexturesEnabled != 0) && (gOut.flags & VERT_FLAT_SHADED) == 0u ? int(inUVW.z) : -1;
    gOut.trapezoidRatios = inTrapezoidRatios;
    gOut.texUV = inUVW.xy;
    if (inUVScroll.y > 0.0) {
        gOut.texUV.y += mod(uUVScrollTick * inUVScroll.x, inUVScroll.y);
    }
    gOut.reflectivity = inReflectivity;
    float fogFactor = ps1FogFactor(length(gOut.eyePos.xyz));
    if (uTrapezoidFilterEnabled != 0) {
        gOut.texUV *= inTrapezoidRatios;
    }
    gOut.affineUV = vec4(gOut.texUV, gOut.trapezoidRatios);

    // The vertex diffuse is lit first and then modulated by the texture (or by
    // the flat polygon's palette color). Keep the lighting component separate
    // from the base color so gamma is applied in the right place.
    LightingResult lr =
        light(inShade, gOut.flags, inNormal.xyz, lightWorldPos, inNormal.w);
    float shade = lr.shade;

    float gamma_exp = 1.0 / ((uGamma / 10.0) * 4.0);

    float tintWeight = 1.0;
#if TR_VERSION == 4
    if ((inFlags & (VERT_MOVE | VERT_GLOW)) != 0u) {
        tintWeight = 0.0;
    }
#endif
    gOut.tintWeight = tintWeight;

    // Applies PlayStation water tint before the gamma curve and texture
    // modulation, where the GTE combines it with the light register.
    vec3 tintReg = uLightingCurve == LIGHTING_CURVE_SATURATE
        ? mix(vec3(1.0), uTint.rgb, tintWeight)
        : vec3(1.0);

#if TR_VERSION >= 4
    // The OG engine lights everything in the "128 = neutral" scale: the lit value is
    // doubled and the excess above 1.0 becomes an additive overbright term
    // applied after texturing (CalcColorSplit).
    vec3 lightIn;
    vec3 modulate;
    if ((gOut.flags & VERT_FLAT_SHADED) != 0u) {
        lightIn = vec3(128.0 / 255.0); // neutral: no lighting, no overbright
        modulate = inColor.rgb;
    } else if (uLightingEnabled == 0) {
        lightIn = vec3(128.0 / 255.0);
        modulate = vec3(1);
    } else {
        if ((gOut.flags & VERT_USE_OBJECT_LIGHT) != 0u) {
            lightIn = lightObjectsTR4(inNormal.xyz);
            // White for regular meshes; mesh policies may recolor vertices
            // (e.g. the skybox fog gradient faces are painted black).
            modulate = inColor.rgb;
        } else if ((gOut.flags & VERT_USE_OWN_LIGHT) != 0u) {
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
    gOut.add = overbright ? max(L - vec3(1.0), vec3(0.0)) * (64.0 / 255.0)
                      : vec3(0.0);
    // The OG draws additive polys with specular disabled (HWR_DrawSortList
    // drawtype 2). An unlit poly carries a color rather than a light value,
    // so it has no excess to split off either.
    if ((gOut.flags & (VERT_ADDITIVE | VERT_NO_LIGHTING)) != 0u) {
        gOut.add = vec3(0.0);
    }
    // Keeps the saturating excess past the gamma curve so texture modulation
    // clips each color channel separately, as the PlayStation does.
    L = min(L * tintReg, vec3(255.0 / 128.0));
    vec3 over = saturate ? max(L - vec3(1.0), vec3(0.0)) : vec3(0.0);
    vec3 lit = gammaCurve(L, gamma_exp) + over;
    gOut.color = vec4(lit * modulate, inColor.a);
#elif TR_VERSION >= 3
    vec3 lightIn;
    vec3 modulate;
    if ((gOut.flags & VERT_FLAT_SHADED) == 0u) {
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
    gOut.add = vec3(0.0);
    if ((gOut.flags & VERT_OVERBRIGHT) != 0u && uLightingEnabled != 0) {
        // OG "128 = neutral" lighting: inColor is a raw prelit color where
        // 128/255 means full brightness. Doubling it saturates the diffuse
        // part; the excess becomes an additive term applied after texturing
        // (tomb4's CalcColorSplit: diffuse = min(2c, 255), add = (c-128)/2).
        vec3 L = lightIn * (255.0 / 128.0) + lr.add;
        if (saturate) {
            lit = L;
        } else {
            gOut.add = max(L - vec3(1.0), vec3(0.0)) * (64.0 / 255.0);
            lit = clamp(L, 0.0, 1.0);
        }
    } else {
        vec3 L = lightIn + lr.add;
        lit = saturate ? L : clamp(L, 0.0, 1.0);
    }
    lit *= lr.mul;

    // Doubles non-overbright TR3 light only when the selected curve uses the
    // OG hardware renderer's 128-neutral scale.
    if (uLightingEnabled != 0 && (gOut.flags & VERT_OVERBRIGHT) == 0u) {
        if (saturate) {
            lit *= 255.0 / 128.0;
        } else if (uLightingCurve == LIGHTING_CURVE_OVERBRIGHT) {
            vec3 L = lit * (255.0 / 128.0);
            if ((gOut.flags & VERT_NO_LIGHTING) == 0u) {
                gOut.add += max(L - vec3(1.0), vec3(0.0)) * (64.0 / 255.0);
            }
            lit = clamp(L, 0.0, 1.0);
        }
    }

    // Keeps the saturating excess past the gamma curve.
    lit = min(lit * tintReg, vec3(255.0 / 128.0));
    vec3 over = max(lit - vec3(1.0), vec3(0.0));
    lit = gammaCurve(lit, gamma_exp) + over;

    // Apply flat shading AFTER modulation
    gOut.color = vec4(lit * modulate, inColor.a);
#else
    gOut.add = vec3(0.0);
    float shade_mul = 1.0;
    if ((gOut.flags & VERT_NO_LIGHTING) == 0u) {
        shade_mul = (2.0 - (max(shade, uMinShade) / SHADE_NEUTRAL));
    }

    // `shade_mul` is roughly in [0..2]. Remap to [0..1], apply the gamma
    // curve, and restore the range. Use sqrt() to limit the effect scope,
    // since we're applying it to the shade (TR1-2) rather than RGB (TR3).
    vec3 mul = gammaCurve(vec3(shade_mul * 0.5), sqrt(gamma_exp)) * 2.0;

    gOut.color = inColor;
    if ((gOut.flags & VERT_FLAT_SHADED) == 0u) {
        gOut.color.rgb = gammaCurve(gOut.color.rgb, gamma_exp);
    }
    gOut.color.rgb *= mul;
    // Preserve the >1.0 lighting range until after texturing so TR1/TR2
    // high contrast can still brighten textured geometry.
    gOut.color.rgb += lr.add;
#endif

    // Applies PlayStation depth cue before texture modulation, as tomb3's
    // transform.cpp does, so fog keeps the surface texture.
    if (uPS1FogEnabled != 0 && (gOut.flags & VERT_NO_LIGHTING) == 0u
        && uLightingEnabled != 0) {
        gOut.color.rgb = mix(gOut.color.rgb, uFogColor.rgb, fogFactor);
        gOut.add *= 1.0 - fogFactor;
    }
}

#elif defined(GEOMETRY)

layout(triangles) in;
layout(triangle_strip, max_vertices = 24) out;

in VertexData {
    MESH_VARYINGS
} gIn[];

out GeomData {
    MESH_VARYINGS
} gOut;

// How far, in pixels, the flat mapping puts a texel from where perspective
// would put it. Across one edge the flat mapping reaches the halfway point at
// the middle of the edge on screen, while perspective reaches it at
// w0 / (w0 + w1), and the gap between the two carries that much of the edge's
// screen length.
//
// The OG picked the split from view distance instead, at 2000 and 3500 world
// units (HWI_InsertGT4_Sorted), which it could do because it drew at one
// resolution. Those distances leave an error of about a pixel at 320x240 and
// grow with the render size, so the same numbers pop visibly once the render
// is larger. Measuring the error keeps the split where the OG put it at the
// OG's resolution, and moves it outward as the render grows.
float affineErrorPixels(void)
{
    float w0 = gl_in[0].gl_Position.w;
    float w1 = gl_in[1].gl_Position.w;
    float w2 = gl_in[2].gl_Position.w;
    float wMin = min(min(w0, w1), w2);
    float wMax = max(max(w0, w1), w2);

    // A face that reaches the eye plane has no finite size on screen. It is as
    // close as a face can be, so it takes the finest split.
    if (wMin < 1.0) {
        return 1.0e9;
    }

    vec2 p0 = gl_in[0].gl_Position.xy / w0;
    vec2 p1 = gl_in[1].gl_Position.xy / w1;
    vec2 p2 = gl_in[2].gl_Position.xy / w2;
    vec2 span = (max(max(p0, p1), p2) - min(min(p0, p1), p2)) * uViewportSize
        * 0.5;
    float extent = max(span.x, span.y);
    return extent * (wMax - wMin) / (2.0 * (wMax + wMin));
}

// Splitting a face in two halves the screen span and the depth range at once,
// so it quarters the error: a level of N leaves an Nth squared of it. The
// levels below are the ones that bring the error under a pixel.
int subdivideLevel(void)
{
    float error = affineErrorPixels();
    if (error > 4.0) {
        return 4;
    }
    if (error > 1.0) {
        return 2;
    }
    return 1;
}

// Clip space is a linear map of view space, so weighting the clip positions
// puts the new vertex where the OG's own SubdivideEdge puts it, and keeps the
// depth offset, the wibble, and the vertex snap the vertex stage applied.
void emitAt(vec3 w)
{
    gl_Position = gl_in[0].gl_Position * w.x + gl_in[1].gl_Position * w.y
        + gl_in[2].gl_Position * w.z;
    gOut.eyePos =
        gIn[0].eyePos * w.x + gIn[1].eyePos * w.y + gIn[2].eyePos * w.z;
    gOut.normal =
        gIn[0].normal * w.x + gIn[1].normal * w.y + gIn[2].normal * w.z;
    gOut.texUV =
        gIn[0].texUV * w.x + gIn[1].texUV * w.y + gIn[2].texUV * w.z;
    gOut.trapezoidRatios = gIn[0].trapezoidRatios * w.x
        + gIn[1].trapezoidRatios * w.y + gIn[2].trapezoidRatios * w.z;
    gOut.color = gIn[0].color * w.x + gIn[1].color * w.y + gIn[2].color * w.z;
    gOut.add = gIn[0].add * w.x + gIn[1].add * w.y + gIn[2].add * w.z;
    gOut.tintWeight = gIn[0].tintWeight * w.x + gIn[1].tintWeight * w.y
        + gIn[2].tintWeight * w.z;
    gOut.affineUV =
        gIn[0].affineUV * w.x + gIn[1].affineUV * w.y + gIn[2].affineUV * w.z;

    // The flat members hold one value for the whole face.
    gOut.flags = gIn[0].flags;
    gOut.texLayer = gIn[0].texLayer;
    gOut.atlasSize = gIn[0].atlasSize;
    gOut.reflectivity = gIn[0].reflectivity;
    EmitVertex();
}

// Row i of the grid holds i + 1 points, from the first corner down to the
// edge that joins the other two.
vec3 gridWeights(int i, int j, float inv)
{
    return vec3(
        1.0 - float(i) * inv, float(i - j) * inv, float(j) * inv);
}

void main(void)
{
    int level = subdivideLevel();
    float inv = 1.0 / float(level);
    for (int i = 0; i < level; i++) {
        for (int j = 0; j <= i; j++) {
            emitAt(gridWeights(i, j, inv));
            emitAt(gridWeights(i + 1, j, inv));
        }
        emitAt(gridWeights(i + 1, i + 1, inv));
        EndPrimitive();
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

in GeomData {
    MESH_VARYINGS
} gIn;
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
        vec3 p = gIn.eyePos.xyz;
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
    vec4 texColor = gIn.color;

    // Texturing and base color
    if (gIn.texLayer >= 0) {
        // The PlayStation had no perspective correction: it interpolated the
        // texture coordinates flat across the face, which is what warps its
        // textures as the camera turns.
        bool affine = uAffineMappingEnabled != 0;
        vec2 uv = affine ? gIn.affineUV.xy : gIn.texUV;
        vec3 texCoords = vec3(uv.x, uv.y, gIn.texLayer);
        if (uTrapezoidFilterEnabled != 0) {
            texCoords.xy /= affine ? gIn.affineUV.zw : gIn.trapezoidRatios;
        }
        if ((gIn.flags & VERT_TEX_WRAP) == 0u) {
            texCoords.xy = clampTexAtlas(texCoords.xy, gIn.atlasSize);
        }
        texColor *= texture(uTexAtlas, texCoords);
    } else {
        texColor.rgb *= texColor.a;
    }

    // Overbright lighting excess, added after texturing (OG specular).
    texColor.rgb += gIn.add;

    // Clips saturating light before fog and bulb blending.
    if (uLightingCurve == LIGHTING_CURVE_SATURATE) {
        texColor.rgb = min(texColor.rgb, vec3(1.0));
    }

    // Alpha discard - chroma keying || transparent pixels in the opaque pass
    if (texColor.a <= 0.0
        || (uDiscardAlpha && texColor.a < 0.99
            && (gIn.flags & VERT_NO_ALPHA_DISCARD) == 0u)) {
        discard;
    }

    // Reflections
    if ((gIn.flags & VERT_REFLECTIVE) != 0u && uReflectionsEnabled != 0) {
#if TR_VERSION >= 4
        // The normal maps across the env map window. No y-flip here: view
        // space is Y-down (the GL/D3D flip lives in the projection) and the
        // atlas is stored top row first, so this already matches the OG.
        //
        // The OG draws the reflection as a second, purely additive pass over
        // the face (drawtype 2 = ONE/ONE), textured with the env map and
        // modulated by the lit vertex color scaled by the face's reflectivity.
        if (uEnvMapLayer >= 0) {
            vec2 env_uv = mix(uEnvMapUV0, uEnvMapUV1, gIn.normal.xy * 0.5 + 0.5);
            vec3 env_color = texture(uTexAtlas, vec3(env_uv, uEnvMapLayer)).rgb;
            texColor.rgb += env_color * gIn.color.rgb * gIn.reflectivity * texColor.a;
        }
#else
        // The env map is a capture of the framebuffer, whose origin is at the
        // bottom left, hence the flip.
        vec2 env_uv = normalize(gIn.normal).xy * 0.5 + 0.5;
        env_uv.y = 1.0 - env_uv.y;
        texColor.rgb *= texture(uTexEnvMap, env_uv).rgb * 2.0;
#endif
    }

    // Fog
    if ((gIn.flags & VERT_NO_LIGHTING) == 0u && uLightingEnabled != 0) {
        texColor = applyFog(texColor, length(gIn.eyePos.xyz));
        // The OG skips fog bulbs on additive polys (AddTriClippedSorted
        // excludes drawtypes 2 and 5 from OmniFog).
        if ((gIn.flags & VERT_ADDITIVE) == 0u) {
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
        texColor.rgb *= mix(vec3(1.0), uTint.rgb, gIn.tintWeight);
        texColor.a *= uTint.a;
    }
    texColor.rgb *= uTint.a;

    outColor = texColor;
}

#endif
