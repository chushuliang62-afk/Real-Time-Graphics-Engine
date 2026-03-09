#version 330 core

uniform sampler2D grassTex;
uniform vec3 lightPos;
uniform vec4 lightColour;
uniform vec3 cameraPos;
uniform float lightRadius;

in Vertex {
    vec2 texCoord;
    vec3 normal;
    vec3 worldPos;
} IN;

out vec4 fragColour;

// Simple noise function for texture variation
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main(void) {
    vec2 uv = IN.texCoord;

    // Use the greenblade.png texture - single realistic grass blade
    vec4 grassTexColor = texture(grassTex, uv);

    // Use texture's RGB color
    vec3 baseColor = grassTexColor.rgb;

    // Add subtle per-blade color variation
    float noise = hash(IN.worldPos.xz * 0.05);
    baseColor *= (0.9 + noise * 0.2);

    // Use texture's alpha for blade shape
    float alpha = grassTexColor.a;

    // Discard transparent pixels
    if (alpha < 0.1) {
        discard;
    }

    // Normalize the normal vector
    vec3 normal = normalize(IN.normal);

    // Calculate lighting
    vec3 incident = normalize(lightPos - IN.worldPos);
    float dist = length(lightPos - IN.worldPos);
    float attenuation = 1.0 - clamp(dist / lightRadius, 0.0, 1.0);

    // Ambient component - grass needs high ambient for outdoor appearance
    vec3 ambient = baseColor * lightColour.rgb * 0.7;

    // Diffuse component with lambert lighting
    float lambert = max(dot(normal, incident), 0.0);
    vec3 diffuse = baseColor * lightColour.rgb * lambert * attenuation * 0.5;

    // Add subsurface scattering effect (light through grass)
    float backLight = max(dot(normal, -incident), 0.0);
    vec3 subsurface = baseColor * lightColour.rgb * backLight * 0.3;

    // Slight specular for wet grass effect
    vec3 viewDir = normalize(cameraPos - IN.worldPos);
    vec3 halfDir = normalize(incident + viewDir);
    float specFactor = pow(max(dot(halfDir, normal), 0.0), 32.0);
    vec3 specular = lightColour.rgb * specFactor * attenuation * 0.2;

    // Combine all lighting components
    vec3 finalColour = ambient + diffuse + subsurface + specular;

    // Slight saturation boost for lush appearance
    finalColour = mix(vec3(dot(finalColour, vec3(0.299, 0.587, 0.114))), finalColour, 1.2);

    // Output with alpha for realistic grass blade edges
    fragColour = vec4(finalColour, alpha);
}
