#version 330 core

// Input vertex attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 colour;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec4 tangent;

// Instance attribute: model matrix (4 consecutive vec4 slots)
layout(location = 5) in vec4 instanceModelCol0;
layout(location = 6) in vec4 instanceModelCol1;
layout(location = 7) in vec4 instanceModelCol2;
layout(location = 8) in vec4 instanceModelCol3;

// Uniforms
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

// Output to fragment shader
out Vertex {
    vec3 worldPos;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec4 colour;
} OUT;

void main(void) {
    // Reconstruct model matrix from instance attributes
    mat4 modelMatrix = mat4(instanceModelCol0, instanceModelCol1,
                            instanceModelCol2, instanceModelCol3);

    // Transform position to clip space
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    gl_Position = mvp * vec4(position, 1.0);

    // World position
    OUT.worldPos = (modelMatrix * vec4(position, 1.0)).xyz;

    // Texture coordinates
    OUT.texCoord = texCoord;

    // Transform normal, tangent, binormal to world space
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    OUT.normal = normalize(normalMatrix * normal);
    OUT.tangent = normalize(normalMatrix * tangent.xyz);
    OUT.binormal = normalize(cross(OUT.normal, OUT.tangent)) * tangent.w;

    // Color
    OUT.colour = colour;
}
