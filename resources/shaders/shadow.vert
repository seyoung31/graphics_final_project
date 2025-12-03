#version 330 core
layout(location = 0) in vec3 os_pos;


uniform mat4 model_matrix;
uniform mat4 light_view_proj;

void main() {

    vec4 world_pos = model_matrix * vec4(os_pos, 1.0);

    gl_Position = light_view_proj * world_pos; //apply the transform to the point

}
