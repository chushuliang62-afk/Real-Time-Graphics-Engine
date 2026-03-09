#version 330 core

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform float time;           // Total elapsed time for animation
uniform float windStrength;   // Wind intensity

// Per-vertex attributes (same quad for all instances)
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

// Per-instance attributes (one per grass blade)
// These will be set via glVertexAttribDivisor
layout(location = 3) in vec3 instancePosition;     // Base position of this grass blade
layout(location = 4) in float instanceRotation;    // Random rotation angle (0-360)
layout(location = 5) in float instanceScale;       // Random scale factor (0.7-1.3)
layout(location = 6) in float instanceRandomSeed;  // Random seed for per-blade variation

out Vertex {
    vec2 texCoord;
    vec3 normal;
    vec3 worldPos;
} OUT;

// Random function (same as original)
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main(void) {
    // Apply per-instance random variations
    vec3 localPos = position;

    // Scale based on instance data
    localPos.y *= instanceScale;
    localPos.x *= instanceScale * 0.8;  // Slightly thinner

    // Rotate around Y-axis
    float cosRot = cos(radians(instanceRotation));
    float sinRot = sin(radians(instanceRotation));
    vec3 rotatedPos;
    rotatedPos.x = localPos.x * cosRot - localPos.z * sinRot;
    rotatedPos.y = localPos.y;
    rotatedPos.z = localPos.x * sinRot + localPos.z * cosRot;

    // Transform to world space
    vec3 worldPos = instancePosition + rotatedPos;

    // Apply wind animation (only affects top vertices)
    float swayFactor = 1.0 - texCoord.y;

    // Wind wave based on world position and time
    float timeFactor = time * 0.5;
    float windWave = sin(timeFactor + worldPos.x * 0.01 + worldPos.z * 0.01) * windStrength * 0.1;

    // Apply wind offset
    float windOffset = windWave * swayFactor;
    worldPos.x += windOffset * 20.0;
    worldPos.z += windOffset * 0.5 * 15.0;

    OUT.worldPos = worldPos;

    // Transform normal
    mat3 rotMat = mat3(
        cosRot, 0.0, sinRot,
        0.0, 1.0, 0.0,
        -sinRot, 0.0, cosRot
    );
    OUT.normal = normalize(rotMat * normal);

    OUT.texCoord = texCoord;

    // Final position
    gl_Position = projMatrix * viewMatrix * vec4(worldPos, 1.0);
}
