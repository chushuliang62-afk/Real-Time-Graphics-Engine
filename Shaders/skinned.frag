#version 330 core

// Fragment shader for skinned meshes (with lighting and shadows)

uniform sampler2D diffuseTex;
uniform sampler2D shadowTex;
uniform samplerCube cubeTex;
uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;
in Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec3 worldPos;
    vec4 shadowCoord;
} IN;

out vec4 fragColour;

float CalculateShadow() {
    // Perspective divide
    vec3 projCoords = IN.shadowCoord.xyz / IN.shadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Out of shadow map bounds = fully lit
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }

    float currentDepth = projCoords.z;

    // IMPROVED: Adaptive bias
    vec3 normal = normalize(IN.normal);
    vec3 lightDir = normalize(lightPos - IN.worldPos);
    float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
    float bias = max(0.01 * (1.0 - cosTheta), 0.003);

    // IMPROVED: 5x5 PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowTex, 0);

    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(shadowTex, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow /= 25.0;

    return shadow;
}

void main(void) {
    // Sample texture - using standard texture sampling for realistic rendering
    vec4 diffuse = texture(diffuseTex, IN.texCoord);

    // Calculate lighting using standard Phong model
    vec3 normal = normalize(IN.normal);
    vec3 incident = normalize(lightPos - IN.worldPos);
    float distance = length(lightPos - IN.worldPos);
    float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

    // Ambient component (not affected by shadows)
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

    // Combine lighting components (shadow affects diffuse and specular, not ambient)
    vec3 finalColour = ambient + (diffuseLight + specular) * shadow;

    fragColour = vec4(finalColour, diffuse.a * IN.colour.a);
}
