#version 330 core

// Bright Pass - Extract bright areas from the scene for bloom effect

uniform sampler2D sceneTex;
uniform float bloomThreshold;  // Brightness threshold (default: 0.8)

in vec2 TexCoords;
out vec4 fragColour;

void main() {
    // Flip texture vertically
    vec2 flippedCoords = vec2(TexCoords.x, 1.0 - TexCoords.y);
    vec3 sceneColor = texture(sceneTex, flippedCoords).rgb;

    // Calculate luminance (perceived brightness)
    float luminance = dot(sceneColor, vec3(0.2126, 0.7152, 0.0722));

    // Extract bright areas above threshold
    // Use smooth threshold for better visual quality
    float brightness = smoothstep(bloomThreshold - 0.1, bloomThreshold + 0.1, luminance);

    // Output bright areas, preserving color
    // Multiply by brightness to fade out near-threshold areas
    vec3 brightColor = sceneColor * brightness;

    fragColour = vec4(brightColor, 1.0);
}
