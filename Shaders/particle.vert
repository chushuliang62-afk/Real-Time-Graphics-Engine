#version 330 core

// Per-vertex attributes (quad geometry)
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

// Per-instance attributes
layout(location = 2) in vec3 instancePosition;
layout(location = 3) in float instanceSize;
layout(location = 4) in float instanceLifetime;

// Uniforms
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform vec3 cameraPos;
uniform float time;
uniform int particleType;  // 0=RAIN, 1=SNOW

// Output to fragment shader
out vec2 TexCoords;
out float ParticleAlpha;
out float ParticleSpeed;  // For motion blur in fragment shader

void main() {
    // Billboard effect - face camera
    vec3 cameraRight = vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    vec3 cameraUp = vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

    // Calculate particle velocity for motion blur
    // Estimate speed based on gravity and lifetime (v = gt)
    float estimatedSpeed = 9.8 * instanceLifetime;
    ParticleSpeed = estimatedSpeed;

    // Rain-specific: Stretch particles vertically based on speed (motion blur effect)
    float stretchFactor = 1.0;
    if (particleType == 0) {  // RAIN
        // Stretch by 1.5-2.5x depending on speed (more subtle)
        stretchFactor = 1.5 + min(estimatedSpeed * 0.1, 1.0);
    }

    // Apply stretch to Y axis (up direction)
    vec3 scaledPos = position;
    scaledPos.y *= stretchFactor;

    // Position particle billboard
    vec3 worldPos = instancePosition
                  + cameraRight * scaledPos.x * instanceSize
                  + cameraUp * scaledPos.y * instanceSize;

    gl_Position = projMatrix * viewMatrix * vec4(worldPos, 1.0);

    TexCoords = texCoord;

    // Simple fade in at birth, no fade out
    // Raindrops are reset by C++ when they hit ground (based on emitter bounds)
    // So no need for manual fade out - just keep them visible until reset
    float fadeIn = min(instanceLifetime * 5.0, 1.0);  // Quick fade in over 0.2s
    ParticleAlpha = fadeIn;

    // Distance-based fade for more natural appearance
    // Removed - was causing rain to disappear when camera moves
    // Rain should be visible at all distances for consistent coverage
}
