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

vec3 dofBlur(vec2 uv, float zView) {
    const float FOCUS_RANGE = 3.5;
    const float DEAD_ZONE   = 0.4;
    const float MAX_BLUR_PX = 14.0;

    float defocus = max(abs(zView - uFocus) - DEAD_ZONE, 0.0);
    float coc = pow(smoothstep(0.0, FOCUS_RANGE, defocus), 1.6);
    float radiusPx = coc * MAX_BLUR_PX;

    if (radiusPx < 0.5) {
        // DOF second => return graded center pixel
        vec3 c = texture(tOrig, uv).rgb;
        return useColorGrade ? gradeColor(c) : c;
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

        // EXACT match: clamp like your original
        vec2 sampleUV = clamp(uv + offsetUV, vec2(0.0), vec2(1.0));

        vec3 raw = texture(tOrig, sampleUV).rgb;
        if (useColorGrade){
            vec3 processed = gradeColor(raw);   // DOF second fix
            float w = (i == 0) ? 3.0 : 1.0;
            accum += processed * w;
            wsum  += w;

        }else{
            float w = (i == 0) ? 3.0 : 1.0;
            accum += raw * w;
            wsum  += w;
        }
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
    
    // Fine-scale edge detection
    float tl = dot(texture(tex, uv + vec2(-offset.x, -offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    float tm = dot(texture(tex, uv + vec2(0.0, -offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    float tr = dot(texture(tex, uv + vec2(offset.x, -offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    float ml = dot(texture(tex, uv + vec2(-offset.x, 0.0)).rgb, vec3(0.299, 0.587, 0.114));
    float mm = dot(texture(tex, uv).rgb, vec3(0.299, 0.587, 0.114));
    float mr = dot(texture(tex, uv + vec2(offset.x, 0.0)).rgb, vec3(0.299, 0.587, 0.114));
    float bl = dot(texture(tex, uv + vec2(-offset.x, offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    float bm = dot(texture(tex, uv + vec2(0.0, offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    float br = dot(texture(tex, uv + vec2(offset.x, offset.y)).rgb, vec3(0.299, 0.587, 0.114));
    
    // Sobel X kernel
    float sobelX = -tl + tr - 2.0 * ml + 2.0 * mr - bl + br;
    // Sobel Y kernel
    float sobelY = -tl - 2.0 * tm - tr + bl + 2.0 * bm + br;
    float fineEdge = sqrt(sobelX * sobelX + sobelY * sobelY);
    
    // Coarse-scale edge detection (catches larger features)
    vec2 coarseOffset = offset * 2.0;
    float coarseTl = dot(texture(tex, clamp(uv + vec2(-coarseOffset.x, -coarseOffset.y), vec2(0.0), vec2(1.0))).rgb, vec3(0.299, 0.587, 0.114));
    float coarseTr = dot(texture(tex, clamp(uv + vec2(coarseOffset.x, -coarseOffset.y), vec2(0.0), vec2(1.0))).rgb, vec3(0.299, 0.587, 0.114));
    float coarseBl = dot(texture(tex, clamp(uv + vec2(-coarseOffset.x, coarseOffset.y), vec2(0.0), vec2(1.0))).rgb, vec3(0.299, 0.587, 0.114));
    float coarseBr = dot(texture(tex, clamp(uv + vec2(coarseOffset.x, coarseOffset.y), vec2(0.0), vec2(1.0))).rgb, vec3(0.299, 0.587, 0.114));
    float coarseEdge = abs(coarseTl - coarseBr) + abs(coarseTr - coarseBl);
    
    // Laplacian edge detection (catches subtle edges)
    float laplacian = abs(4.0 * mm - tl - tr - bl - br);
    
    // Combine all edge detection methods
    return max(fineEdge, max(coarseEdge * 0.5, laplacian * 0.3));
}

// Depth-based edge detection for additional line definition
float detectDepthEdge(sampler2D depthTex, vec2 uv) {
    vec2 offset = uInvScreenSize;
    
    float center = texture(depthTex, uv).r;
    float left = texture(depthTex, clamp(uv + vec2(-offset.x, 0.0), vec2(0.0), vec2(1.0))).r;
    float right = texture(depthTex, clamp(uv + vec2(offset.x, 0.0), vec2(0.0), vec2(1.0))).r;
    float top = texture(depthTex, clamp(uv + vec2(0.0, -offset.y), vec2(0.0), vec2(1.0))).r;
    float bottom = texture(depthTex, clamp(uv + vec2(0.0, offset.y), vec2(0.0), vec2(1.0))).r;
    
    // Diagonal samples for better edge detection
    float tl = texture(depthTex, clamp(uv + vec2(-offset.x, -offset.y), vec2(0.0), vec2(1.0))).r;
    float tr = texture(depthTex, clamp(uv + vec2(offset.x, -offset.y), vec2(0.0), vec2(1.0))).r;
    float bl = texture(depthTex, clamp(uv + vec2(-offset.x, offset.y), vec2(0.0), vec2(1.0))).r;
    float br = texture(depthTex, clamp(uv + vec2(offset.x, offset.y), vec2(0.0), vec2(1.0))).r;
    
    // Sobel on depth
    float sobelX = -tl + tr - 2.0 * left + 2.0 * right - bl + br;
    float sobelY = -tl - 2.0 * top - tr + bl + 2.0 * bottom + br;
    
    // Also check for large depth differences
    float maxDiff = max(max(abs(left - right), abs(top - bottom)), 
                       max(abs(tl - br), abs(tr - bl)));
    
    return max(sqrt(sobelX * sobelX + sobelY * sobelY) * 15.0, maxDiff * 20.0);
}

// Crosshatching pattern for shading
float crosshatch(vec2 uv) {
    float angle1 = 45.0 * 3.14159 / 180.0;
    float angle2 = -45.0 * 3.14159 / 180.0;
    
    float scale = 200.0;
    float line1 = abs(sin(dot(uv, vec2(cos(angle1), sin(angle1)) * scale)));
    float line2 = abs(sin(dot(uv, vec2(cos(angle2), sin(angle2)) * scale)));
    
    return min(line1, line2);
}

// Line art effect
vec3 applyLineArt(vec3 baseColor, vec2 uv) {
    // Step 1: Detect edges from color (multi-scale Sobel + Laplacian)
    float colorEdge = detectEdge(tOrig, uv);
    
    // Step 2: Detect edges from depth for additional definition
    float depthEdge = detectDepthEdge(sceneDepthTex, uv);
    
    // Step 3: Also detect edges from color differences (catches texture edges)
    vec3 centerColor = texture(tOrig, uv).rgb;
    vec3 leftColor = texture(tOrig, clamp(uv + vec2(-uInvScreenSize.x, 0.0), vec2(0.0), vec2(1.0))).rgb;
    vec3 rightColor = texture(tOrig, clamp(uv + vec2(uInvScreenSize.x, 0.0), vec2(0.0), vec2(1.0))).rgb;
    vec3 topColor = texture(tOrig, clamp(uv + vec2(0.0, -uInvScreenSize.y), vec2(0.0), vec2(1.0))).rgb;
    vec3 bottomColor = texture(tOrig, clamp(uv + vec2(0.0, uInvScreenSize.y), vec2(0.0), vec2(1.0))).rgb;
    
    float colorDiff = max(max(length(centerColor - leftColor), length(centerColor - rightColor)),
                        max(length(centerColor - topColor), length(centerColor - bottomColor)));
    
    // Step 4: Combine all edge detections (more sensitive)
    float combinedEdge = max(max(colorEdge, depthEdge * 0.7), colorDiff * 2.0);
    
    // Step 5: Lower threshold to catch more edges, with sharper transition
    float edgeStrength = smoothstep(0.1, 0.4, combinedEdge);
    
    // Step 6: Convert to grayscale for line art look
    float gray = dot(baseColor, vec3(0.299, 0.587, 0.114));
    
    // Step 7: Add crosshatching for shading areas (lighter areas)
    float hatch = crosshatch(uv);
    float hatchMask = 1.0 - smoothstep(0.3, 0.7, gray); // Only in lighter areas
    float hatched = mix(1.0, hatch, hatchMask * 0.3);
    
    // Step 8: Combine: white background, black lines, gray crosshatching
    vec3 lineArt = vec3(1.0);
    
    // Draw black lines where edges are strong
    lineArt = mix(lineArt, vec3(0.0), edgeStrength);
    
    // Add crosshatching for mid-tones
    lineArt = mix(lineArt, vec3(0.7) * hatched, (1.0 - edgeStrength) * (1.0 - gray) * 0.4);
    
    // Step 9: Add subtle paper texture
    float paper = noise(uv * 300.0) * 0.05;
    lineArt += paper;
    
    return clamp(lineArt, 0.0, 1.0);
}

// Pixelated effect
vec3 applyPixelated(vec3 baseColor, vec2 uv) {
    // Pixelation size (adjustable - smaller = more pixelated)
    float pixelSize = 12.0; // Number of pixels to combine
    
    // Calculate the pixel grid using inverse screen size
    vec2 screenSize = 1.0 / uInvScreenSize;
    vec2 pixelGrid = screenSize / pixelSize;
    
    // Quantize UV coordinates to create pixel blocks
    vec2 quantizedUV = floor(uv * pixelGrid) / pixelGrid;
    
    // Sample the color at the quantized UV (nearest neighbor)
    // Add half pixel offset to sample from center of pixel block
    vec2 pixelCenter = quantizedUV + vec2(0.5) / pixelGrid;
    pixelCenter = clamp(pixelCenter, vec2(0.0), vec2(1.0));
    
    vec3 pixelatedColor = texture(tOrig, pixelCenter).rgb;
    
    // Optional: Color quantization for retro look (reduce color palette)
    float colorLevels = 8.0; // Number of color levels per channel
    pixelatedColor = floor(pixelatedColor * colorLevels) / colorLevels;
    
    return pixelatedColor;
}

void main() {
    vec3 baseColor = texture(tOrig, v_uv).rgb;

    float depth = texture(sceneDepthTex, v_uv).r;
    float zView = linearizeDepth(depth);

    // LUT first
    if (useColorGrade){
        vec3 lutColor = applyLUT(baseColor);
        baseColor = mix(baseColor, lutColor, u_GradeStrength);
    }

    vec3 finalColor = baseColor;

    // DOF second (now blurs graded samples)
    if (dof) {
        finalColor = dofBlur(v_uv, zView);
    }

    // Line art effect (applied after DOF)
    if (watercolor) {
        finalColor = applyLineArt(finalColor, v_uv);
    }

    // Pixelated effect (applied after line art)
    if (pixelated) {
        finalColor = applyPixelated(finalColor, v_uv);
    }

    fragColor = vec4(finalColor, 1.0);
}
