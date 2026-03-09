#version 330 core

uniform vec4 sunColor;      // Current sun color from day-night cycle
uniform float timeOfDay;    // 0-24 hours

in Vertex {
    vec2 texCoord;
} IN;

out vec4 fragColor;

void main(void) {
    // For sphere mesh, create glowing effect
    // Calculate distance from center using texture coordinates
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(IN.texCoord, center);

    // Create soft sphere with bright core
    float intensity = smoothstep(0.5, 0.2, dist);

    // Sun/Moon color based on time
    vec3 finalColor;

    if (timeOfDay < 5.0 || timeOfDay > 20.0) {
        // Night - moon (pale blue-white)
        finalColor = vec3(0.9, 0.9, 1.0) * 1.5;
    }
    else if (timeOfDay < 7.0 || timeOfDay > 17.0) {
        // Sunrise/Sunset - warm orange
        finalColor = vec3(1.0, 0.7, 0.3) * 2.0;
    }
    else {
        // Day - bright yellow-white sun
        finalColor = vec3(1.0, 0.95, 0.8) * 3.0;
    }

    // Apply intensity and emit light
    fragColor = vec4(finalColor * intensity, intensity);

    // Discard low intensity fragments
    if (intensity < 0.05) {
        discard;
    }
}
