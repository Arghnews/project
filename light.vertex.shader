#version 410 core
layout (location = 0) in vec3 position; // The position variable has attribute position 0

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Note that we read the multiplication from right to left
    gl_Position = projection * view * model * vec4(position, 1.0);
}
