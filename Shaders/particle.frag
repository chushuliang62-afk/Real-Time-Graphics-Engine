#version 330 core

uniform sampler2D particleTex;
uniform int particleType;  // 0=RAIN, 1=SNOW
uniform float intensity;

in vec2 TexCoords;
in float ParticleAlpha;
in float ParticleSpeed;

out vec4 fragColour;

void main() {
    // Sample texture
    vec4 texColor = texture(particleTex, TexCoords);

    // Particle color based on type
    vec3 particleColor;
    float alpha;

    if (particleType == 0) {
        // RAIN - More realistic appearance
        // Center the UVs
        vec2 uv = TexCoords - vec2(0.5);

        // Create elongated raindrop shape (stretched vertically)
        // X: narrow width, Y: elongated length
        float dropShape = 1.0 - smoothstep(0.0, 0.5, length(uv * vec2(4.0, 1.0)));

        // Add gradient along the length (brighter at the tail)
        float gradient = 1.0 - TexCoords.y;
        gradient = pow(gradient, 0.5);  // Soften the gradient

        // Combine shape and gradient
        alpha = dropShape * gradient * texColor.a;

        // Subtle blue-white tint (not too blue, more natural)
        particleColor = vec3(0.85, 0.92, 1.0);

        // Speed-based brightness: faster = brighter streaks
        float speedBrightness = 1.0 + min(ParticleSpeed * 0.05, 0.5);
        particleColor *= speedBrightness;

    } else {
        // SNOW - soft round particles
        vec2 center = TexCoords - vec2(0.5);
        float dist = length(center);

        // Soft circular gradient
        alpha = smoothstep(0.5, 0.0, dist) * texColor.a;

        // Pure white snow
        particleColor = vec3(1.0, 1.0, 1.0);
    }

    // Apply lifetime fade
    alpha *= ParticleAlpha;

    // Apply intensity
    alpha *= intensity;

    // Rain needs less alpha boost than before (more subtle)
    if (particleType == 0) {
        alpha = clamp(alpha * 1.5, 0.0, 0.8);  // Cap at 0.8 for semi-transparency
    } else {
        alpha = clamp(alpha * 2.5, 0.0, 1.0);
    }

    fragColour = vec4(particleColor, alpha);

    // Discard very transparent fragments
    if (fragColour.a < 0.01) {
        discard;
    }
}
