#version 330 core

// Create a UV coordinate in variable
in vec2 v_uv;

uniform sampler2D tOrig; // original scene texture
uniform sampler2D tLUT; // LUT texture

uniform float u_GradeStrength;
uniform bool u_EnableColorGrading;

out vec4 fragColor;

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

void main() {
    // The pixel color you previously rendered to FBO
    vec4 base = texture(tOrig, v_uv);

    // Color from LUT
    if (length(base.rgb) < 0.001 || !u_EnableColorGrading) {
        fragColor = base;
        return;
    }

    vec3 lutColor = applyLUT(base.rgb);
    vec3 finalColor = mix(base.rgb, lutColor, u_GradeStrength);
    fragColor = vec4(finalColor, base.a);
}
