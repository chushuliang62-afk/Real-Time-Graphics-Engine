#version 330 core

// Vertex attributes (must match nclgl layout)
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 colour;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec4 tangent;
layout(location = 5) in vec4 skinWeights;   // Bone weights (4 bones per vertex)
layout(location = 6) in ivec4 skinIndices;  // Bone indices (4 bones per vertex)

// Uniforms
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 shadowMatrix;      // For shadow mapping

// Skeletal animation uniforms
const int MAX_JOINTS = 128;
uniform mat4 joints[MAX_JOINTS];  // Joint transformation matrices

// Output to fragment shader
out Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec3 worldPos;
    vec4 shadowCoord;
} OUT;

void main(void) {
    // Calculate skinned position
    vec4 skinnedPosition = vec4(0.0);
    vec4 skinnedNormal = vec4(0.0);
    vec4 skinnedTangent = vec4(0.0);

    // Blend position, normal, and tangent using bone weights
    for (int i = 0; i < 4; ++i) {
        int jointIndex = skinIndices[i];
        float weight = skinWeights[i];

        if (weight > 0.0 && jointIndex >= 0 && jointIndex < MAX_JOINTS) {
            mat4 jointTransform = joints[jointIndex];
            skinnedPosition += jointTransform * vec4(position, 1.0) * weight;
            skinnedNormal += jointTransform * vec4(normal, 0.0) * weight;
            skinnedTangent += jointTransform * vec4(tangent.xyz, 0.0) * weight;
        }
    }

    // If no skinning weights, use original position (fallback)
    if (skinnedPosition.w == 0.0) {
        skinnedPosition = vec4(position, 1.0);
        skinnedNormal = vec4(normal, 0.0);
        skinnedTangent = vec4(tangent.xyz, 0.0);
    }

    // Transform to world space
    vec4 worldPosition = modelMatrix * skinnedPosition;

    // Calculate TBN (tangent, binormal, normal) for lighting
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    vec3 wNormal = normalize(normalMatrix * skinnedNormal.xyz);
    vec3 wTangent = normalize(normalMatrix * skinnedTangent.xyz);
    vec3 wBinormal = cross(wTangent, wNormal) * tangent.w;

    // Output to fragment shader
    OUT.colour = colour;
    OUT.texCoord = texCoord;
    OUT.normal = wNormal;
    OUT.tangent = wTangent;
    OUT.binormal = wBinormal;
    OUT.worldPos = worldPosition.xyz;
    OUT.shadowCoord = shadowMatrix * worldPosition;

    // Final position
    gl_Position = projMatrix * viewMatrix * worldPosition;
}
