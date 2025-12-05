#version 330 core

// Object-space attributes from VBO
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;
layout(location = 5) in mat4 instanceModel;

// Declare 'out' variable for the world-space position and normal
// to be passed to the fragment shader
out vec3 w_pos;
out vec3 w_normal;
out vec2 fragUV;

// TBN matrix for tangent-space normal mapping
out mat3 TBN;

// Declare uniform mat4's for the view and projection matrix
uniform mat4 view;
uniform mat4 proj;

//for shadow mapping!
out vec4 lightSpacePos[8]; //light pos in space
uniform int numLights; //num lights we have
uniform mat4 lightVP[8]; //light VP matrix

void main() {

    mat4 model = instanceModel;

    // World-space position
    vec4 posWorld = model * vec4(position, 1.0);
    w_pos = posWorld.xyz;

    // World-space normal
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    w_normal = normalize(normalMatrix * normal);

    // Pass UV coordinates
    fragUV = uv;

    // Transform TBN vectors from model space to camera space
    mat4 modelViewMatrix = view * model;
    mat3 MV3x3 = mat3(modelViewMatrix);
    vec3 vertexNormal_cameraspace = MV3x3 * normalize(normal);
    vec3 vertexTangent_cameraspace = MV3x3 * normalize(tangent);
    vec3 vertexBitangent_cameraspace = MV3x3 * normalize(bitangent);
    
    // Construct TBN matrix: Camera space to tangent space
    TBN = transpose(mat3(
        vertexTangent_cameraspace,
        vertexBitangent_cameraspace,
        vertexNormal_cameraspace
    ));

    for (int i = 0; i < numLights; i++) {
        lightSpacePos[i] = lightVP[i] * posWorld; //pass into output the light space pos for each light
    }

    gl_Position = proj * view * posWorld;

}
