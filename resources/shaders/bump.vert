#version 330 core

layout(location=0) in vec3 objPosition;
layout(location=1) in vec3 objNormal;
layout(location=2) in vec2 objUV;
layout(location=3) in vec3 objTangent;
layout(location=4) in vec3 objBitangent;

out vec3 worldPosition;
out vec3 worldNormal;
out vec2 fragUV;

// TBN matrix for tangent-space calculations
out mat3 TBN;

// Pass tangent-space vectors for bump mapping
out vec3 tangent_cameraspace;
out vec3 bitangent_cameraspace;
out vec3 normal_cameraspace;

// Shadow mapping
out vec4 lightSpacePos[8];

uniform mat4 m_model;
uniform mat4 view;
uniform mat4 proj;
uniform mat3 MV3x3;

// UV shift for bump mapping
uniform vec2 uvShift;

// Shadow mapping light view-projection matrices
uniform mat4 lightVP[8];
uniform int numLights;

void main() {
    // Compute world-space position and normal
    vec4 worldPos4 = m_model * vec4(objPosition, 1.0);
    worldPosition = worldPos4.xyz;
    worldNormal = normalize(mat3(inverse(transpose(m_model))) * normalize(objNormal));
    
    // Apply UV shift for bump mapping passes
    fragUV = objUV + uvShift;

    // Transform TBN vectors from model space to camera space
    normal_cameraspace = MV3x3 * normalize(objNormal);
    tangent_cameraspace = MV3x3 * normalize(objTangent);
    bitangent_cameraspace = MV3x3 * normalize(objBitangent);
    
    // Construct TBN matrix: transforms from camera space to tangent space
    TBN = transpose(mat3(
        tangent_cameraspace,
        bitangent_cameraspace,
        normal_cameraspace
    ));

    // Shadow mapping: compute light space positions
    vec4 posWorld = m_model * vec4(objPosition, 1.0);
    for (int i = 0; i < numLights; i++) {
        lightSpacePos[i] = lightVP[i] * posWorld;
    }

    // Transform to clip space
    gl_Position = proj * view * m_model * vec4(objPosition, 1.0);
}

