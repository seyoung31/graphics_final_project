#version 330 core

// Object-space attributes from VBO
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in mat4 instanceModel;

// Declare 'out' variable for the world-space position and normal
// to be passed to the fragment shader
out vec3 w_pos;
out vec3 w_normal;

// Declare a uniform mat4 to store model matrix
// uniform mat4 model;

// Declare uniform mat4's for the view and projection matrix
uniform mat4 view;
uniform mat4 proj;

void main() {

    mat4 model = instanceModel;

    // World-space position
    vec4 posWorld = model * vec4(position, 1.0);
    w_pos = posWorld.xyz;

    // World-space normal
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    w_normal = normalize(normalMatrix * normal);

    gl_Position = proj * view * posWorld;

}
