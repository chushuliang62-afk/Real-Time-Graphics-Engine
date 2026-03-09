#version 330 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 textureMatrix;
uniform mat4 shadowMatrix;  // Light's view-projection matrix

in vec3 position;
in vec4 colour;
in vec3 normal;
in vec4 tangent;
in vec2 texCoord;

out Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec3 worldPos;
    vec3 viewDir;
    vec4 shadowCoord;  // Position in light space for shadow mapping
} OUT;

void main(void) {
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

    OUT.colour = colour;
    OUT.texCoord = (textureMatrix * vec4(texCoord, 0.0, 1.0)).xy;

    OUT.normal = normalize(normalMatrix * normalize(normal));
    OUT.tangent = normalize(normalMatrix * normalize(tangent.xyz));
    OUT.binormal = normalize(cross(OUT.tangent, OUT.normal)) * tangent.w;

    vec4 worldPos = modelMatrix * vec4(position, 1.0);
    OUT.worldPos = worldPos.xyz;

    // View direction for environment mapping
    vec3 cameraWorldPos = vec3(inverse(viewMatrix)[3]);
    OUT.viewDir = normalize(worldPos.xyz - cameraWorldPos);

    // Calculate shadow coordinate (position in light's clip space)
    OUT.shadowCoord = shadowMatrix * worldPos;

    gl_Position = mvp * vec4(position, 1.0);
}
