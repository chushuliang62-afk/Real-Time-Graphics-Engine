#version 330 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 textureMatrix;
uniform float time;  // For wave animation

in vec3 position;
in vec4 colour;
in vec3 normal;
in vec2 texCoord;

out Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 worldPos;
    vec4 clipSpace;  // For screen-space effects
} OUT;

void main(void) {
    mat4 mvp = projMatrix * viewMatrix * modelMatrix;
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

    // Animated water surface with subtle waves
    vec3 animatedPos = position;

    // Restore subtle base waves
    float wave1 = sin(position.x * 0.03 + time * 0.25) * 0.25;
    float wave2 = cos(position.z * 0.025 + time * 0.2) * 0.2;
    float wave3 = sin((position.x + position.z) * 0.02 + time * 0.18) * 0.15;

    // RAIN RIPPLES - Multiple expanding circles from random positions
    float rippleHeight = 0.0;

    // Create 8 ripples at different positions and times (more ripples = more visible)
    for (int i = 0; i < 8; i++) {
        // Pseudo-random position in local space (-0.7 to 0.7 range to stay within water)
        float timeOffset = float(i) * 0.8 + time * 0.4;  // Faster spawn rate
        float xOffset = sin(timeOffset * 0.7 + float(i) * 2.1) * 0.7;
        float zOffset = cos(timeOffset * 0.83 + float(i) * 1.7) * 0.7;
        vec2 rippleCenter = vec2(xOffset, zOffset);

        float dist = length(position.xz - rippleCenter);

        // Expanding ripple
        float rippleTime = mod(timeOffset, 2.5);  // 2.5 second lifetime (faster)
        float rippleRadius = rippleTime * 0.4;    // Faster expansion

        // Distance from expanding ring
        float ringDist = abs(dist - rippleRadius);
        float ringFade = smoothstep(0.3, 0.0, ringDist);  // Wider ring for visibility

        // Lifetime fade (stronger initial impact)
        float lifeFade = 1.0 - (rippleTime / 2.5);
        lifeFade = pow(lifeFade, 1.5);  // Sharper fade

        // Oscillating wave with more frequency
        float wave = sin(dist * 18.0 - rippleTime * 8.0);

        // BIGGER amplitude (6.0 units per ripple)
        rippleHeight += wave * ringFade * lifeFade * 6.0;
    }

    // Combine base waves with rain ripples
    animatedPos.y += wave1 + wave2 + wave3 + rippleHeight;

    vec4 worldPos = modelMatrix * vec4(animatedPos, 1.0);
    OUT.worldPos = worldPos.xyz;
    OUT.colour = colour;
    OUT.texCoord = (textureMatrix * vec4(texCoord, 0.0, 1.0)).xy * 10.0;  // Tile water texture

    // Calculate dynamic normal for lighting (including base waves + ripples)
    // Base wave derivatives
    float dx = cos(position.x * 0.03 + time * 0.25) * 0.03 * 0.25;
    float dz = -sin(position.z * 0.025 + time * 0.2) * 0.025 * 0.2;

    // Add ripple gradients (approximate)
    for (int i = 0; i < 8; i++) {
        float timeOffset = float(i) * 0.8 + time * 0.4;
        float xOffset = sin(timeOffset * 0.7 + float(i) * 2.1) * 0.7;
        float zOffset = cos(timeOffset * 0.83 + float(i) * 1.7) * 0.7;
        vec2 rippleCenter = vec2(xOffset, zOffset);
        vec2 toCenter = position.xz - rippleCenter;
        float dist = length(toCenter);

        if (dist > 0.01) {
            float rippleTime = mod(timeOffset, 2.5);
            float rippleRadius = rippleTime * 0.4;
            float ringDist = abs(dist - rippleRadius);
            float ringFade = smoothstep(0.3, 0.0, ringDist);
            float lifeFade = 1.0 - (rippleTime / 2.5);
            lifeFade = pow(lifeFade, 1.5);

            float derivative = cos(dist * 18.0 - rippleTime * 8.0) * 18.0 * ringFade * lifeFade * 6.0;
            vec2 gradient = (toCenter / dist) * derivative;
            dx += gradient.x;
            dz += gradient.y;
        }
    }

    vec3 waveNormal = normalize(vec3(-dx, 1.0, -dz));
    OUT.normal = normalize(normalMatrix * waveNormal);

    vec4 clipSpace = mvp * vec4(animatedPos, 1.0);
    OUT.clipSpace = clipSpace;
    gl_Position = clipSpace;
}
