#version 410 core
layout (location = 0) in vec3 position; // The position variable has attribute position 0
layout (location = 1) in vec3 color;   // The color variable has attribute position 1

//uniform float xOffset;

out vec3 ourColor; // Output a color to the fragment shader

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {

    // Note that we read the multiplication from right to left
    gl_Position = projection * view * model * vec4(position, 1.0);

    //dvec3 pos = dvec3(position.x+xOffset,position.y,position.z);
    //vec4 pos = transform * vec4(position, 1.0);
    //gl_Position = pos;
    //ourColor = pos.xyz + dvec3(0.1,0.1,0.1); // Set ourColor to the input color we got from the vertex data
    ourColor = color;
}
