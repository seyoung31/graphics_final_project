#version 330 core

// Task 16: Create a UV coordinate in variable
in vec2 coordUV;
// Task 8: Add a sampler2D uniform
uniform sampler2D uni_texture;
// Task 29: Add a bool on whether or not to filter the texture
uniform bool post_proc;

out vec4 fragColor;

void main()
{
    fragColor = vec4(1);
    // Task 17: Set fragColor using the sampler2D at the UV coordinate
    fragColor = texture(uni_texture, coordUV);
    // Task 33: Invert fragColor's r, g, and b color channels if your bool is true
    if (post_proc){
        fragColor.r = 1.0 - fragColor.r;
                fragColor.g = 1.0 - fragColor.g;
                fragColor.b = 1.0 - fragColor.b;
    }
}
