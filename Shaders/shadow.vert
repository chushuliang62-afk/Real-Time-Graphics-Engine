#version 330 core

// Shadow pass only needs MVP transformation
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;    // Light's view matrix
uniform mat4 projMatrix;    // Light's projection matrix

in vec3 position;

void main(void) {
    // Transform vertex position to light clip space
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    gl_Position = mvp * vec4(position, 1.0);
}
