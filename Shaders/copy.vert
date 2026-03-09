#version 330 core

// Simple pass-through vertex shader for post-processing
// Must match nclgl vertex attribute layout

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texCoord;

out vec2 TexCoords;

void main() {
    gl_Position = vec4(position, 1.0);
    TexCoords = texCoord;
}
