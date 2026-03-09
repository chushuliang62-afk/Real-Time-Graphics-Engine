#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D shadowTex;  // Shadow depth map
uniform samplerCube cubeTex;
uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;
uniform float reflectivity; // How reflective the surface is (0-1)

in Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec3 worldPos;
    vec3 viewDir;
    vec4 shadowCoord;  // Position in light space
} IN;

out vec4 fragColour;

float CalculateShadow() {
    // Perspective divide to get NDC coordinates
    vec3 projCoords = IN.shadowCoord.xyz / IN.shadowCoord.w;

    // Transform from [-1,1] NDC to [0,1] texture coordinates
    projCoords = projCoords * 0.5 + 0.5;

    // Check if fragment is outside shadow map bounds
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;  // Fully lit if outside shadow map
    }

    float currentDepth = projCoords.z;

    // IMPROVED: Adaptive bias based on surface angle
    vec3 normal = normalize(IN.normal);
    vec3 lightDir = normalize(lightPos - IN.worldPos);
    float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
    float bias = max(0.01 * (1.0 - cosTheta), 0.003);

    // IMPROVED: 5x5 PCF for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowTex, 0);

    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(shadowTex, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow /= 25.0;  // Average 25 samples

    return shadow;
}

void main(void) {
    vec4 diffuse = texture(diffuseTex, IN.texCoord);

    vec3 normal = normalize(IN.normal);
    vec3 incident = normalize(lightPos - IN.worldPos);
    float distance = length(lightPos - IN.worldPos);
    float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

    // Ambient component (always present, not affected by shadows)
    // Increased for brighter outdoor scene
    vec3 ambient = diffuse.rgb * lightColour.rgb * 0.5;

    // Diffuse component
    float lambert = max(dot(incident, normal), 0.0);
    vec3 diffuseLight = diffuse.rgb * lightColour.rgb * lambert * attenuation;

    // Specular component
    vec3 viewDir = normalize(cameraPos - IN.worldPos);
    vec3 halfDir = normalize(incident + viewDir);
    float specFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
    specFactor = pow(specFactor, 60.0);
    vec3 specular = lightColour.rgb * specFactor * attenuation * 0.33;

    // Calculate shadow factor
    float shadow = CalculateShadow();

    // Apply shadow to diffuse and specular (but not ambient)
    vec3 litColour = ambient + (diffuseLight + specular) * shadow;

    // Environment mapping
    vec3 reflectDir = reflect(normalize(IN.viewDir), normal);
    vec4 reflectTex = texture(cubeTex, reflectDir);

    // Combine reflection with lit color
    vec3 finalColour = mix(litColour, reflectTex.rgb, reflectivity);

    fragColour = vec4(finalColour, diffuse.a * IN.colour.a);
}
