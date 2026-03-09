#version 330 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

in vec3 position;
in vec2 texCoord;

out Vertex {
    vec2 texCoord;
} OUT;

void main(void) {
    // Simple rendering without billboard - just transform normally
    gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
    OUT.texCoord = texCoord;
}
