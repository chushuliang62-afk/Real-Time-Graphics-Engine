#version 330 core

/*
 * deferred_lighting.vert
 *
 * Deferred Rendering - Lighting Pass Vertex Shader
 *
 * Features:
 * - Render fullscreen quad
 * - Pass texture coordinates to fragment shader
 */

// Vertex attributes
layout(location = 0) in vec2 position;   // NDC coordinates (-1 to 1)
layout(location = 1) in vec2 texCoord;   // Texture coordinates (0 to 1)

// Output to fragment shader
out vec2 TexCoords;

void main(void) {
    TexCoords = texCoord;
    gl_Position = vec4(position, 0.0, 1.0);  // Use NDC coordinates directly
}
