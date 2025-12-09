#version 330 core

// Create a UV coordinate in variable
in vec2 v_uv;

uniform sampler2D tOrig; // original scene texture
uniform sampler2D tLUT; // LUT texture

uniform float u_GradeStrength;
uniform bool u_EnableColorGrading;

out vec4 fragColor;

uniform bool useColorGrade;
//dof vars
uniform bool dof;
uniform float uNear;
uniform float uFar;
uniform float uFocus;
uniform sampler2D sceneDepthTex; //stores depth
uniform vec2 uInvScreenSize;
uniform bool watercolor;
uniform bool pixelated;
uniform float uPixelSize;
uniform bool isNight;

float linearizeDepth(float depth) {
    float z_ndc = depth * 2.0 - 1.0;
    return (2.0 * uNear * uFar) /
           (uFar + uNear - z_ndc * (uFar - uNear));
}


// Helper method to apply 4x4x4 LUT stored as 16x4 texture
vec3 applyLUT(vec3 color) {
    const float slices     = 4.0;
    const float xOffset    = 1.0 / slices;
    const float maxSlice   = slices - 1.0;
    const float npWidth    = 1.0 / (slices * slices);
    const float npHeight   = 1.0 / slices;
    const float npHalfW    = npWidth * 0.5;
    const float npHalfH    = npHeight * 0.5;

    float x = (color.r * (xOffset - npWidth)) + npHalfW;
    float y = (color.g * (1.0   - npHeight)) + npHalfH;

    float slice = color.b * maxSlice;
    float SB = floor(slice);
    float ST = ceil(slice);
    float SM = fract(slice);

    vec3 colB = texture(tLUT, vec2(xOffset * SB + x, y)).rgb;
    vec3 colT = texture(tLUT, vec2(xOffset * ST + x, y)).rgb;
    vec3 lutColor = mix(colB, colT, SM);

    return lutColor;
}

vec3 gradeColor(vec3 c) {
    vec3 lutColor = applyLUT(c);
    return mix(c, lutColor, u_GradeStrength);
}

// NEW: Sample with optional color grading applied
vec3 sampleWithGrading(vec2 uv) {
    vec3 c = texture(tOrig, uv).rgb;
    if (useColorGrade) {
        c = gradeColor(c);
    }
    return c;
}

vec3 samplePixelated(vec2 uv) {
    float pixelSize = uPixelSize;

    vec2 screenSize = 1.0 / uInvScreenSize;
    vec2 pixelGrid  = screenSize / pixelSize;

    vec2 quantizedUV = floor(uv * pixelGrid) / pixelGrid;
    vec2 pixelCenter = quantizedUV + vec2(0.5) / pixelGrid;
    pixelCenter = clamp(pixelCenter, vec2(0.0), vec2(1.0));

    // FIX: Use sampleWithGrading instead of texture(tOrig)
    vec3 c = sampleWithGrading(pixelCenter);

    // optional palette quantization
    float levels = 8.0;
    c = floor(c * levels) / levels;

    if (isNight) {
        // Apply a cool blue tint instead of brightening everything
        // This creates a moonlight effect rather than a swampy look
        vec3 nightTint = vec3(0.6, 0.7, 1.0); // Cool blue tint
        c *= nightTint;

        // Optional: Slightly boost very dark areas to maintain visibility
        float lum = dot(c, vec3(0.299, 0.587, 0.114));
        if (lum < 0.1) {
            c = mix(c, vec3(0.05, 0.06, 0.08), 0.3); // Very subtle dark blue lift
        }
    }

    return c;
}

vec3 dofBlur(vec2 uv, float zView) {
    const float DEAD_ZONE_M   = 0.4;
    const float MAX_BLUR_PX   = 14.0;

    float depthRange = max(uFar - uNear, 1e-3);

    float defocusN = abs(zView - uFocus) / depthRange;
    float deadN    = DEAD_ZONE_M / depthRange;

    float defocus = max(defocusN - deadN, 0.0);

    const float FOCUS_RANGE_N = 0.12;
    float coc = smoothstep(0.0, FOCUS_RANGE_N, defocus);

    float radiusPx = MAX_BLUR_PX * coc;

    if (radiusPx < 0.5) {
        vec3 c = pixelated ? samplePixelated(uv)
                           : sampleWithGrading(uv);
        return c;
    }

    const int TAP_COUNT = 13;
    vec2 taps[TAP_COUNT] = vec2[](
        vec2( 0.0,  0.0),
        vec2( 0.4,  0.1), vec2(-0.4, -0.1),
        vec2( 0.1,  0.4), vec2(-0.1, -0.4),
        vec2( 0.7,  0.3), vec2(-0.7, -0.3),
        vec2( 0.3,  0.7), vec2(-0.3, -0.7),
        vec2( 0.9,  0.0), vec2(-0.9,  0.0),
        vec2( 0.0,  0.9), vec2( 0.0, -0.9)
    );

    vec3 accum = vec3(0.0);
    float wsum = 0.0;

    for (int i = 0; i < TAP_COUNT; i++) {
        vec2 offsetUV = taps[i] * radiusPx * uInvScreenSize;
        vec2 sampleUV = clamp(uv + offsetUV, vec2(0.0), vec2(1.0));

        vec3 raw = pixelated ? samplePixelated(sampleUV)
                             : sampleWithGrading(sampleUV);

        float w = (i == 0) ? 3.0 : 1.0;
        accum += raw * w;
        wsum  += w;
    }

    return accum / wsum;
}

// Simple noise function
float noise(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Enhanced edge detection using Sobel operator (multiple scales)
float detectEdge(sampler2D tex, vec2 uv) {
    vec2 offset = uInvScreenSize;

    float tl = dot(texture(tex, uv + vec2(-offset.x, -offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    float tm = dot(texture(tex, uv + vec2(0.0, -offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    float tr = dot(texture(tex, uv + vec2(offset.x, -offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    float ml = dot(texture(tex, uv + vec2(-offset.x, 0.0)).rgb, vec3(0.299, 0.587, 0.114));
    float mm = dot(texture(tex, uv).rgb, vec3(0.299, 0.587, 0.114));
    float mr = dot(texture(tex, uv + vec2(offset.x, 0.0)).rgb, vec3(0.299, 0.587, 0.114));
    float bl = dot(texture(tex, uv + vec2(-offset.x, offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    float bm = dot(texture(tex, uv + vec2(0.0, offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    float br = dot(texture(tex, uv + vec2(offset.x, offset.y)).rgb, vec3(0.299, 0.587, 0.114));

    float sobelX = -tl + tr - 2.0 * ml + 2.0 * mr - bl + br;
    float sobelY = -tl - 2.0 * tm - tr + bl + 2.0 * bm + br;
    float fineEdge = sqrt(sobelX * sobelX + sobelY * sobelY);

    vec2 coarseOffset = offset * 2.0;
    float coarseTl = dot(texture(tex, clamp(uv + vec2(-coarseOffset.x, -coarseOffset.y), vec2(0.0), vec2(1.0))).rgb, vec3(0.299, 0.587, 0.114));
    float coarseTr = dot(texture(tex, clamp(uv + vec2(coarseOffset.x, -coarseOffset.y), vec2(0.0), vec2(1.0))).rgb, vec3(0.299, 0.587, 0.114));
    float coarseBl = dot(texture(tex, clamp(uv + vec2(-coarseOffset.x, coarseOffset.y), vec2(0.0), vec2(1.0))).rgb, vec3(0.299, 0.587, 0.114));
    float coarseBr = dot(texture(tex, clamp(uv + vec2(coarseOffset.x, coarseOffset.y), vec2(0.0), vec2(1.0))).rgb, vec3(0.299, 0.587, 0.114));
    float coarseEdge = abs(coarseTl - coarseBr) + abs(coarseTr - coarseBl);

    float laplacian = abs(4.0 * mm - tl - tr - bl - br);

    return max(fineEdge, max(coarseEdge * 0.5, laplacian * 0.3));
}

float detectDepthEdge(sampler2D depthTex, vec2 uv) {
    vec2 offset = uInvScreenSize;

    float center = texture(depthTex, uv).r;
    float left = texture(depthTex, clamp(uv + vec2(-offset.x, 0.0), vec2(0.0), vec2(1.0))).r;
    float right = texture(depthTex, clamp(uv + vec2(offset.x, 0.0), vec2(0.0), vec2(1.0))).r;
    float top = texture(depthTex, clamp(uv + vec2(0.0, -offset.y), vec2(0.0), vec2(1.0))).r;
    float bottom = texture(depthTex, clamp(uv + vec2(0.0, offset.y), vec2(0.0), vec2(1.0))).r;

    float tl = texture(depthTex, clamp(uv + vec2(-offset.x, -offset.y), vec2(0.0), vec2(1.0))).r;
    float tr = texture(depthTex, clamp(uv + vec2(offset.x, -offset.y), vec2(0.0), vec2(1.0))).r;
    float bl = texture(depthTex, clamp(uv + vec2(-offset.x, offset.y), vec2(0.0), vec2(1.0))).r;
    float br = texture(depthTex, clamp(uv + vec2(offset.x, offset.y), vec2(0.0), vec2(1.0))).r;

    float sobelX = -tl + tr - 2.0 * left + 2.0 * right - bl + br;
    float sobelY = -tl - 2.0 * top - tr + bl + 2.0 * bottom + br;

    float maxDiff = max(max(abs(left - right), abs(top - bottom)),
                       max(abs(tl - br), abs(tr - bl)));

    return max(sqrt(sobelX * sobelX + sobelY * sobelY) * 15.0, maxDiff * 20.0);
}

float crosshatch(vec2 uv) {
    float angle1 = 45.0 * 3.14159 / 180.0;
    float angle2 = -45.0 * 3.14159 / 180.0;

    float scale = 200.0;
    float line1 = abs(sin(dot(uv, vec2(cos(angle1), sin(angle1)) * scale)));
    float line2 = abs(sin(dot(uv, vec2(cos(angle2), sin(angle2)) * scale)));

    return min(line1, line2);
}

vec3 applyLineArt(vec3 baseColor, vec2 uv) {
    float colorEdge = detectEdge(tOrig, uv);
    float depthEdge = detectDepthEdge(sceneDepthTex, uv);

    vec3 centerColor = texture(tOrig, uv).rgb;
    vec3 leftColor = texture(tOrig, clamp(uv + vec2(-uInvScreenSize.x, 0.0), vec2(0.0), vec2(1.0))).rgb;
    vec3 rightColor = texture(tOrig, clamp(uv + vec2(uInvScreenSize.x, 0.0), vec2(0.0), vec2(1.0))).rgb;
    vec3 topColor = texture(tOrig, clamp(uv + vec2(0.0, -uInvScreenSize.y), vec2(0.0), vec2(1.0))).rgb;
    vec3 bottomColor = texture(tOrig, clamp(uv + vec2(0.0, uInvScreenSize.y), vec2(0.0), vec2(1.0))).rgb;

    float colorDiff = max(max(length(centerColor - leftColor), length(centerColor - rightColor)),
                        max(length(centerColor - topColor), length(centerColor - bottomColor)));

    float combinedEdge = max(max(colorEdge, depthEdge * 0.7), colorDiff * 2.0);
    float edgeStrength = smoothstep(0.1, 0.4, combinedEdge);

    float gray = dot(baseColor, vec3(0.299, 0.587, 0.114));

    float hatch = crosshatch(uv);
    float hatchMask = 1.0 - smoothstep(0.3, 0.7, gray);
    float hatched = mix(1.0, hatch, hatchMask * 0.3);

    vec3 lineArt = vec3(1.0);
    lineArt = mix(lineArt, vec3(0.0), edgeStrength);
    lineArt = mix(lineArt, vec3(0.7) * hatched, (1.0 - edgeStrength) * (1.0 - gray) * 0.4);

    float paper = noise(uv * 300.0) * 0.05;
    lineArt += paper;

    return clamp(lineArt, 0.0, 1.0);
}

// REMOVED: applyPixelated function - no longer needed as standalone

void main() {
    float depth = texture(sceneDepthTex, v_uv).r;
    float zView = linearizeDepth(depth);

    vec3 finalColor;

    // Apply DOF first (which handles both pixelation and color grading internally)
    if (dof) {
        finalColor = dofBlur(v_uv, zView);
    }
    // If no DOF, handle pixelation + grading together
    else if (pixelated) {
        finalColor = samplePixelated(v_uv);
    }
    // Otherwise just sample with grading
    else {
        finalColor = sampleWithGrading(v_uv);
    }

    // Apply watercolor/line art effect last
    if (watercolor) {
        finalColor = applyLineArt(finalColor, v_uv);
    }

    fragColor = vec4(finalColor, 1.0);
}
