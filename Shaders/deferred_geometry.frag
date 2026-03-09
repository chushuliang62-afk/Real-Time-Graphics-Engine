#version 330 core

/*
 * deferred_geometry.frag
 *
 * Deferred Rendering - Geometry Pass Fragment Shader
 *
 * Features:
 * - Write to G-Buffer (position, normal, albedo)
 * - Support normal mapping
 * - Support specular intensity
 */

// Input (from vertex shader)
in Vertex {
    vec3 worldPos;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec2 texCoord;
    vec4 colour;
} IN;

// G-Buffer output
layout(location = 0) out vec3 gPosition;    // World space position
layout(location = 1) out vec3 gNormal;      // World space normal
layout(location = 2) out vec4 gAlbedoSpec;  // RGB=albedo, A=specular intensity

// Textures
uniform sampler2D diffuseTex;
uniform sampler2D normalTex;

// Control parameters
uniform bool useNormalMap;      // Whether to use normal map
uniform float specularIntensity; // Specular reflection intensity

void main(void) {
    // 1. Write world space position
    gPosition = IN.worldPos;

    // 2. Calculate and write world space normal
    vec3 normal;
    if (useNormalMap) {
        // Sample from normal map
        vec3 tangentNormal = texture(normalTex, IN.texCoord).xyz;
        tangentNormal = normalize(tangentNormal * 2.0 - 1.0);  // [0,1] -> [-1,1]

        // Build TBN matrix
        mat3 TBN = mat3(
            normalize(IN.tangent),
            normalize(IN.binormal),
            normalize(IN.normal)
        );

        // Transform to world space
        normal = normalize(TBN * tangentNormal);
    } else {
        // Use interpolated normal directly
        normal = normalize(IN.normal);
    }
    gNormal = normal;

    // 3. Write albedo and specular intensity
    vec4 albedo = texture(diffuseTex, IN.texCoord);

    // If texture has alpha channel, can be used for alpha testing
    if (albedo.a < 0.1) {
        discard;  // Discard fully transparent fragments
    }

    gAlbedoSpec.rgb = albedo.rgb * IN.colour.rgb;  // Albedo (modulated by vertex color)
    gAlbedoSpec.a = specularIntensity;             // Specular intensity
}
