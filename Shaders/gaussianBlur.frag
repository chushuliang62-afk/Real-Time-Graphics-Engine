#version 330 core

// Gaussian Blur - Two-pass separable blur for bloom effect

uniform sampler2D inputTex;
uniform bool horizontal;       // true = horizontal pass, false = vertical pass
uniform float blurRadius;      // Blur strength (default: 1.0)

in vec2 TexCoords;
out vec4 fragColour;

// 9-tap Gaussian kernel weights (sigma = 2.0)
// Optimized for quality/performance balance
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    // Flip texture vertically
    vec2 flippedCoords = vec2(TexCoords.x, 1.0 - TexCoords.y);

    // Calculate texture offset based on texture size
    vec2 texOffset = 1.0 / textureSize(inputTex, 0) * blurRadius;

    // Start with center sample
    vec3 result = texture(inputTex, flippedCoords).rgb * weights[0];

    if (horizontal) {
        // Horizontal blur
        for (int i = 1; i < 5; ++i) {
            result += texture(inputTex, flippedCoords + vec2(texOffset.x * float(i), 0.0)).rgb * weights[i];
            result += texture(inputTex, flippedCoords - vec2(texOffset.x * float(i), 0.0)).rgb * weights[i];
        }
    } else {
        // Vertical blur
        for (int i = 1; i < 5; ++i) {
            result += texture(inputTex, flippedCoords + vec2(0.0, texOffset.y * float(i))).rgb * weights[i];
            result += texture(inputTex, flippedCoords - vec2(0.0, texOffset.y * float(i))).rgb * weights[i];
        }
    }

    fragColour = vec4(result, 1.0);
}
