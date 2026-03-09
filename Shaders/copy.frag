#version 330 core

// Simple copy shader - just passes through the scene texture

uniform sampler2D sceneTex;
uniform float isTransitioning;  // 0.0 = not transitioning, 1.0 = transitioning
uniform float transitionProgress; // 0.0 to 1.0

in vec2 TexCoords;
out vec4 fragColour;

void main() {
    // Flip texture vertically to fix upside-down rendering
    vec2 flippedCoords = vec2(TexCoords.x, 1.0 - TexCoords.y);
    vec4 sceneColor = texture(sceneTex, flippedCoords);

    // Add visual feedback during transition - ALWAYS CHECK (not just > 0.5)
    if (isTransitioning > 0.1) {
        // Multiple transition effects for dramatic time-warp

        // 1. STRONG fade to white at midpoint (peak of transition)
        float fadeProgress = 1.0 - abs(transitionProgress * 2.0 - 1.0); // 0->1->0
        vec3 fadeColor = vec3(1.0, 1.0, 1.0); // Pure white
        sceneColor.rgb = mix(sceneColor.rgb, fadeColor, fadeProgress * 0.9); // 90% white at peak!

        // 2. Strong chromatic aberration effect (color separation)
        float aberration = fadeProgress * 0.03; // Increased from 0.02
        vec3 rChannel = texture(sceneTex, flippedCoords + vec2(aberration, 0.0)).rgb;
        vec3 bChannel = texture(sceneTex, flippedCoords - vec2(aberration, 0.0)).rgb;
        sceneColor.r = mix(sceneColor.r, rChannel.r, fadeProgress * 0.8); // Increased from 0.5
        sceneColor.b = mix(sceneColor.b, bChannel.b, fadeProgress * 0.8);

        // 3. Strong vignette effect during transition
        vec2 uv = flippedCoords * 2.0 - 1.0;
        float vignette = 1.0 - dot(uv, uv) * 0.5 * fadeProgress; // Increased from 0.3
        sceneColor.rgb *= vignette;

        // 4. Strong desaturation during peak
        float gray = dot(sceneColor.rgb, vec3(0.299, 0.587, 0.114));
        sceneColor.rgb = mix(sceneColor.rgb, vec3(gray), fadeProgress * 0.6); // Increased from 0.3

        // 5. Add a flash of light at the very peak
        if (fadeProgress > 0.8) {
            float flash = (fadeProgress - 0.8) / 0.2; // 0 to 1 in the peak 20%
            sceneColor.rgb += vec3(flash * 0.5); // Bright flash
        }
    }

    fragColour = sceneColor;
}
