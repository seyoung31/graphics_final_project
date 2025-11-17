#version 330 core
// layout (location = 0) in vec3 position;
// layout (location = 1) in vec3 color;
// out vec3 col;

// void main() {
//    gl_Position = vec4(position, 1.0);
//    col = color;
// }

// Task 4: declare a vec3 object-space position variable, using
//         the `layout` and `in` keywords.
layout(location = 0) in vec3 position3D;
layout(location = 1) in vec3 normal3D;
// Task 5: declare `out` variables for the world-space position and normal,
//         to be passed to the fragment shader
out vec3 world_pos;
out vec3 world_norm;
// Task 6: declare a uniform mat4 to store model matrix
uniform mat4 model_mat;
// Task 7: declare uniform mat4's for the view and projection matrix
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
    // Task 8: compute the world-space position and normal, then pass them to
    //         the fragment shader using the variables created in task 5
    world_pos = vec3((model_mat) * vec4(position3D, 1.0));
    world_norm = vec3((model_mat) * vec4(normal3D, 0.0)); // transpose(inverse(mat3(model_mat))) * normalize(position3D);

    // Recall that transforming normals requires obtaining the inverse-transpose of the model matrix!
    // In projects 5 and 6, consider the performance implications of performing this here.

    // Task 9: set gl_Position to the object space position transformed to clip space
    gl_Position = proj_mat * view_mat * model_mat * vec4(position3D, 1.0);

}

