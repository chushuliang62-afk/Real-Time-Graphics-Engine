#version 330 core

/*
 * deferred_geometry.vert
 *
 * Deferred Rendering - Geometry Pass Vertex Shader
 *
 * Features:
 * - Transform vertices to world space and clip space
 * - Pass world space position, normal, texture coordinates to fragment shader
 * - Support normal mapping (TBN matrix)
 */

// Vertex attributes
in vec3 position;
in vec4 colour;
in vec3 normal;
in vec4 tangent;
in vec2 texCoord;

// Uniform variables
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

// Output to fragment shader
out Vertex {
    vec3 worldPos;      // World space position
    vec3 normal;        // World space normal
    vec3 tangent;       // World space tangent
    vec3 binormal;      // World space binormal
    vec2 texCoord;      // Texture coordinates
    vec4 colour;        // Vertex color
} OUT;

void main(void) {
    // Calculate world space position
    vec4 worldPosition = modelMatrix * vec4(position, 1.0);
    OUT.worldPos = worldPosition.xyz;

    // Calculate normal matrix (for correct normal transformation)
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

    // Transform normal to world space
    OUT.normal = normalize(normalMatrix * normal);

    // Transform tangent and binormal to world space (for normal mapping)
    OUT.tangent = normalize(normalMatrix * tangent.xyz);
    OUT.binormal = normalize(cross(OUT.normal, OUT.tangent)) * tangent.w;

    // Pass texture coordinates and vertex color
    OUT.texCoord = texCoord;
    OUT.colour = colour;

    // Calculate clip space position
    gl_Position = projMatrix * viewMatrix * worldPosition;
}
