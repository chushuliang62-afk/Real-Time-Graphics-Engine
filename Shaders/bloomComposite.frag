#version 330 core

// Bloom Composite - Combine original scene with blurred bright areas

uniform sampler2D sceneTex;      // Original scene
uniform sampler2D bloomTex;      // Blurred bright areas
uniform float bloomIntensity;    // Bloom strength (default: 0.5)
uniform float isTransitioning;   // Transition effect flag
uniform float transitionProgress;

in vec2 TexCoords;
out vec4 fragColour;

void main() {
    // Flip texture vertically
    vec2 flippedCoords = vec2(TexCoords.x, 1.0 - TexCoords.y);

    // Sample both textures
    vec3 sceneColor = texture(sceneTex, flippedCoords).rgb;
    vec3 bloomColor = texture(bloomTex, flippedCoords).rgb;

    // Additive blending for bloom effect
    // Use bloomIntensity to control the effect strength
    vec3 finalColor = sceneColor + bloomColor * bloomIntensity;

    // Apply transition effects if transitioning
    if (isTransitioning > 0.1) {
        // Multiple transition effects for dramatic time-warp

        // 1. STRONG fade to white at midpoint (peak of transition)
        float fadeProgress = 1.0 - abs(transitionProgress * 2.0 - 1.0); // 0->1->0
        vec3 fadeColor = vec3(1.0, 1.0, 1.0); // Pure white
        finalColor = mix(finalColor, fadeColor, fadeProgress * 0.9); // 90% white at peak!

        // 2. Strong chromatic aberration effect (color separation)
        float aberration = fadeProgress * 0.03;
        vec3 rChannel = texture(sceneTex, flippedCoords + vec2(aberration, 0.0)).rgb;
        vec3 bChannel = texture(sceneTex, flippedCoords - vec2(aberration, 0.0)).rgb;
        finalColor.r = mix(finalColor.r, rChannel.r, fadeProgress * 0.8);
        finalColor.b = mix(finalColor.b, bChannel.b, fadeProgress * 0.8);

        // 3. Strong vignette effect during transition
        vec2 uv = flippedCoords * 2.0 - 1.0;
        float vignette = 1.0 - dot(uv, uv) * 0.5 * fadeProgress;
        finalColor *= vignette;

        // 4. Strong desaturation during peak
        float gray = dot(finalColor, vec3(0.299, 0.587, 0.114));
        finalColor = mix(finalColor, vec3(gray), fadeProgress * 0.6);

        // 5. Add a flash of light at the very peak
        if (fadeProgress > 0.8) {
            float flash = (fadeProgress - 0.8) / 0.2; // 0 to 1 in the peak 20%
            finalColor += vec3(flash * 0.5); // Bright flash
        }
    }

    fragColour = vec4(finalColor, 1.0);
}
