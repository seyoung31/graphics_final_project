#version 330 core
in vec2 vUV;
in vec4 vColor;
out vec4 fragColor;

uniform sampler2D uParticleTex;
uniform float uAlphaCutoff;
uniform vec4 uTint;

void main() {
    vec4 tex = texture(uParticleTex, vUV);
    float alpha = tex.a * vColor.a * uTint.a;
    if (alpha < uAlphaCutoff) discard;
    vec3 rgb = tex.rgb * vColor.rgb * uTint.rgb;
    fragColor = vec4(rgb, alpha);
}
