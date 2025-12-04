#version 330 core
layout(location=0) in vec3 inVertexPos;   // base quad (-0.5..0.5)
layout(location=1) in vec4 inOffset;      // x,y,z,size
layout(location=2) in vec4 inColor;       // normalized (ubyte -> float)

uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;

out vec2 vUV;
out vec4 vColor;

void main() {
    // build world-space position for this instance
    vec3 worldPos = inOffset.xyz
        + (uCameraRight * inVertexPos.x + uCameraUp * inVertexPos.y) * inOffset.w;

    gl_Position = uProj * uView * vec4(worldPos, 1.0);
    vUV = inVertexPos.xy + vec2(0.5); // map [-0.5,0.5] -> [0,1]
    vColor = inColor;
}
