#version 330 core

uniform sampler2D diffuseTex;      // Coast sand rocks (mountain base)
uniform sampler2D sandTex;         // Sand texture (beach/low areas)
uniform sampler2D rockTex;         // Rock texture (high mountains)
uniform sampler2D mudTex;          // Brown mud texture (grass land around lake)
uniform sampler2D shadowTex;       // Shadow depth map
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

    // IMPROVED: Adaptive bias based on surface angle (prevents shadow acne and peter panning)
    vec3 normal = normalize(IN.normal);
    vec3 lightDir = normalize(lightPos - IN.worldPos);
    float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
    float bias = max(0.01 * (1.0 - cosTheta), 0.003);  // Dynamic bias: 0.003-0.01

    // IMPROVED: 5x5 PCF for softer shadows (25 samples instead of 9)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowTex, 0);

    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(shadowTex, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow /= 25.0;  // Average 25 samples (5x5 kernel)

    return shadow;
}

void main(void) {
    // Height-based texture blending for realistic terrain
    float height = IN.worldPos.y;

    // Sample all terrain textures
    vec4 sandColor = texture(sandTex, IN.texCoord * 3.0);     // Beach/shore sand
    vec4 mudColor = texture(mudTex, IN.texCoord * 2.0);       // Brown mud (grass land around lake)
    vec4 rockColor = texture(rockTex, IN.texCoord * 2.0);     // Rocky peak

    // Define height thresholds for blending
    // Water level: 10, Basin shore: 10-30, Grass land: 18-200, Rocky peak: 200+
    float sandHeight = 12.0;      // Beach/shore level (just above water at 10)
    float mudHeight = 18.0;       // Grass/mud land starts above shore
    float rockHeight = 200.0;     // Rocky peak starts here
    float blendRange = 6.0;       // Smooth transition range

    // Calculate blend factors based on height
    float sandBlend = smoothstep(sandHeight + blendRange, sandHeight, height);

    // Mud blend: grassland height (18-200)
    float mudBlend = smoothstep(mudHeight, mudHeight + blendRange, height) *
                     smoothstep(rockHeight + blendRange, rockHeight, height);

    // Rock blend: very high elevation (200+)
    float rockBlend = smoothstep(rockHeight, rockHeight + blendRange, height);

    // Slope-based blending (steeper slopes = more rock)
    float slope = 1.0 - abs(dot(normalize(IN.normal), vec3(0, 1, 0)));
    rockBlend += slope * 0.5;
    mudBlend *= (1.0 - slope * 0.3);

    // Normalize blend factors
    float totalBlend = sandBlend + mudBlend + rockBlend;
    sandBlend /= totalBlend;
    mudBlend /= totalBlend;
    rockBlend /= totalBlend;

    // Blend textures based on calculated factors
    vec4 diffuse = sandColor * sandBlend +
                   mudColor * mudBlend +
                   rockColor * rockBlend;

    vec3 normal = normalize(IN.normal);
    vec3 incident = normalize(lightPos - IN.worldPos);
    float distance = length(lightPos - IN.worldPos);
    float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

    // Ambient component (always present, not affected by shadows)
    // Increased ambient for brighter outdoor scene
    vec3 ambient = diffuse.rgb * lightColour.rgb * 0.4;

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
    fragColour.rgb = ambient + (diffuseLight + specular) * shadow;
    fragColour.a = diffuse.a;
}
