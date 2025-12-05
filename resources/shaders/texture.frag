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
    if (length(c) < 0.001 || !u_EnableColorGrading) return c;
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
        return gradeColor(texture(tOrig, uv).rgb);
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
        vec3 graded = gradeColor(raw);   // DOF second fix

        float w = (i == 0) ? 3.0 : 1.0;
        accum += graded * w;
        wsum  += w;
    }

    return accum / wsum;
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

    fragColor = vec4(finalColor, 1.0);
}
