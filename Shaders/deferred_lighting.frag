#version 330 core

/*
 * deferred_lighting.frag
 *
 * Deferred Rendering - Lighting Pass Fragment Shader
 *
 * Features:
 * - Read position, normal, albedo from G-Buffer
 * - Calculate multi-point light illumination (Phong model)
 * - Support physically correct attenuation (inverse-square law)
 * - Ambient light + multiple point lights
 */

// Input
in vec2 TexCoords;

// Output
out vec4 FragColour;

// G-Buffer textures
uniform sampler2D gPosition;    // World space position
uniform sampler2D gNormal;      // World space normal
uniform sampler2D gAlbedoSpec;  // RGB=albedo, A=specular intensity

// Camera position
uniform vec3 cameraPos;

// Ambient light
uniform vec3 ambientColour;
uniform float ambientIntensity;

// Directional light (sun)
uniform vec3 sunDirection;   // Direction TO the sun (normalized)
uniform vec3 sunColour;
uniform float sunIntensity;

// Point light structure
struct PointLight {
    vec3 position;
    vec3 colour;
    float intensity;
    float radius;
    float constant;
    float linear;
    float quadratic;
};

// Point light array (maximum 32)
#define MAX_LIGHTS 32
uniform PointLight lights[MAX_LIGHTS];
uniform int numLights;

/**
 * Calculate single point light contribution
 */
vec3 CalculatePointLight(PointLight light, vec3 fragPos, vec3 normal, vec3 albedo, float specular, vec3 viewDir) {
    // Calculate light direction and distance
    vec3 lightDir = light.position - fragPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);

    // Distance attenuation (physically correct: inverse-square law)
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    // Radius cutoff (complete attenuation beyond radius)
    if (distance > light.radius) {
        attenuation = 0.0;
    }

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo * light.colour;

    // Specular (Blinn-Phong model)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);  // Shininess = 32
    vec3 specularLight = spec * specular * light.colour;

    // Apply attenuation and light intensity
    vec3 result = (diffuse + specularLight) * attenuation * light.intensity;

    return result;
}

void main(void) {
    // Sample from G-Buffer
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec4 albedoSpec = texture(gAlbedoSpec, TexCoords);
    vec3 albedo = albedoSpec.rgb;
    float specular = albedoSpec.a;

    // Calculate view direction
    vec3 viewDir = normalize(cameraPos - fragPos);

    // 1. Ambient light
    vec3 ambient = ambientColour * ambientIntensity * albedo;

    // 2. Directional light (sun)
    vec3 sunLight = vec3(0.0);
    if (sunIntensity > 0.0) {
        float sunDiff = max(dot(normal, sunDirection), 0.0);
        vec3 sunDiffuse = sunDiff * albedo * sunColour;

        vec3 sunHalf = normalize(sunDirection + viewDir);
        float sunSpec = pow(max(dot(normal, sunHalf), 0.0), 32.0);
        vec3 sunSpecular = sunSpec * specular * sunColour;

        sunLight = (sunDiffuse + sunSpecular) * sunIntensity;
    }

    // 3. Accumulate all point lights
    vec3 lighting = vec3(0.0);
    for (int i = 0; i < numLights && i < MAX_LIGHTS; ++i) {
        lighting += CalculatePointLight(lights[i], fragPos, normal, albedo, specular, viewDir);
    }

    // Final color = ambient + sun + all point lights
    vec3 finalColour = ambient + sunLight + lighting;

    // Output (HDR, can apply tone mapping later)
    FragColour = vec4(finalColour, 1.0);
}
