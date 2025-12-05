#version 330 core
layout(location = 0) in vec3 os_pos;
// Locations 1-4 are used by normal, uv, tangent, bitangent (but not needed for shadows)
// Instance model matrix at locations 5-8
layout(location = 5) in mat4 instanceModel;


uniform mat4 model_matrix;
uniform mat4 light_view_proj;

void main() {

    // Use instance model matrix if available, otherwise use uniform
    mat4 model = instanceModel;
    if (model[0][0] == 0.0 && model[1][1] == 0.0 && model[2][2] == 0.0 && model[3][3] == 0.0) {
        model = model_matrix;
    }

    vec4 world_pos = model * vec4(os_pos, 1.0);

    gl_Position = light_view_proj * world_pos; //apply the transform to the point

}
