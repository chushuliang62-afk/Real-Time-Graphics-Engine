#version 330 core

uniform sampler2D waterTex;        // Water base texture
uniform sampler2D waterBumpTex;    // Water normal/bump map
uniform samplerCube cubeTex;       // Skybox for reflections
uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;
uniform float time;                // For animated ripples

in Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 worldPos;
    vec4 clipSpace;
} IN;

out vec4 fragColour;

void main(void) {
    // ========================================
    // NORMAL MAPPING: Sample normal from waterBumpTex
    // ========================================
    // Animated UV coordinates for scrolling normal map effect
    vec2 bumpUV1 = IN.texCoord * 0.5 + vec2(time * 0.01, time * 0.008);
    vec2 bumpUV2 = IN.texCoord * 0.7 - vec2(time * 0.012, time * 0.007);

    // Sample normal map twice with different UVs and blend for richer detail
    vec3 bumpNormal1 = texture(waterBumpTex, bumpUV1).rgb;
    vec3 bumpNormal2 = texture(waterBumpTex, bumpUV2).rgb;

    // Convert from [0,1] range to [-1,1] range
    bumpNormal1 = normalize(bumpNormal1 * 2.0 - 1.0);
    bumpNormal2 = normalize(bumpNormal2 * 2.0 - 1.0);

    // Blend two normal samples for more complex water surface
    vec3 bumpNormal = normalize(bumpNormal1 + bumpNormal2);

    // Combine bump normal with vertex normal (animated by ripples)
    // The vertex normal already contains ripple animation from vertex shader
    vec3 vertexNormal = normalize(IN.normal);

    // Blend bump map normal with vertex ripple normal
    // Use tangent space transformation for proper normal mapping
    vec3 normal = normalize(vertexNormal + bumpNormal * 0.3);  // 0.3 = bump strength

    // Lighting calculations
    vec3 incident = normalize(lightPos - IN.worldPos);
    float distance = length(lightPos - IN.worldPos);
    float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

    // Diffuse lighting
    float lambert = max(dot(incident, normal), 0.0);
    vec3 diffuseLight = lightColour.rgb * lambert * attenuation;

    // Enhanced specular for shimmer effect
    vec3 viewDir = normalize(cameraPos - IN.worldPos);
    vec3 halfDir = normalize(incident + viewDir);
    float specFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
    specFactor = pow(specFactor, 256.0);  // Very high shininess for sparkle
    vec3 specular = lightColour.rgb * specFactor * attenuation * 2.0;  // Boosted specular

    // Reflection from skybox
    vec3 reflectDir = reflect(-viewDir, normal);
    vec3 reflectCol = texture(cubeTex, reflectDir).rgb;

    // Enhanced Fresnel effect for depth perception
    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 3.0);
    fresnel = mix(0.2, 0.95, fresnel);  // More pronounced fresnel

    // Natural lake color gradient - realistic tones
    vec3 waterShallowCol = vec3(0.15, 0.45, 0.55);   // Muted teal (shallow, natural)
    vec3 waterDeepCol = vec3(0.05, 0.15, 0.28);      // Deep blue-gray (deep, natural)

    // Calculate depth from basin center
    vec2 basinCenter = vec2(500.0, 400.0);
    float distFromCenter = length(IN.worldPos.xz - basinCenter);
    float depth = 1.0 - clamp(distFromCenter / 360.0, 0.0, 1.0);  // 1.0 at center, 0.0 at edge

    // Strong depth gradient with quadratic falloff
    float depthFactor = depth * depth;
    vec3 waterColor = mix(waterShallowCol, waterDeepCol, depthFactor);

    // Combine with lighting - reduce ambient for more natural look
    vec3 litColor = waterColor * (diffuseLight + vec3(0.35)) + specular * 0.8;

    // Natural reflection blend - more subtle
    vec3 finalColor = mix(litColor, reflectCol, fresnel * 0.4);

    // Add slight murkiness for natural lake appearance
    vec3 murkiness = vec3(0.08, 0.12, 0.15);
    finalColor = mix(finalColor, murkiness, depth * 0.3);

    // Ensure minimum visibility
    finalColor = max(finalColor, vec3(0.04, 0.1, 0.15));

    fragColour.rgb = finalColor;

    // Strong transparency gradient
    float edgeFade = smoothstep(0.0, 0.3, depth);
    fragColour.a = mix(0.6, 0.95, edgeFade);
}
