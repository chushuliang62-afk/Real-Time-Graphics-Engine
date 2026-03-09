#version 330 core

// Time Transition Post-Processing Shader
// Creates dramatic visual effects during time jump transitions

uniform sampler2D sceneTex;
uniform float transitionProgress;  // 0.0 to 1.0 (0=start, 0.5=peak, 1.0=end)
uniform float time;                // Total elapsed time for animation

in vec2 texCoord;
out vec4 fragColor;

// Noise function for distortion and grain
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // Smooth interpolation

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main() {
    vec2 uv = texCoord;

    // No transition - render normally
    if (transitionProgress <= 0.0) {
        fragColor = texture(sceneTex, uv);
        return;
    }

    // Calculate transition intensity (peaks at 0.5, fades at 0 and 1)
    float intensity = sin(transitionProgress * 3.14159);  // 0 -> 1 -> 0 smooth curve

    // ==== EFFECT 1: Ripple Distortion ====
    // Circular ripples emanating from center
    vec2 center = vec2(0.5, 0.5);
    float dist = length(uv - center);
    float rippleSpeed = time * 10.0;
    float ripple = sin(dist * 30.0 - rippleSpeed) * intensity * 0.05;

    // Radial distortion
    vec2 direction = normalize(uv - center);
    uv += direction * ripple;

    // ==== EFFECT 2: Chromatic Aberration ====
    // RGB channel separation for "reality tearing" effect
    float aberration = intensity * 0.015;
    vec3 color;
    color.r = texture(sceneTex, uv + vec2(aberration, 0.0)).r;
    color.g = texture(sceneTex, uv).g;
    color.b = texture(sceneTex, uv - vec2(aberration, 0.0)).b;

    // ==== EFFECT 3: White Flash at Peak ====
    // Intense flash when transitionProgress = 0.5 (peak of transition)
    float flashIntensity = 1.0 - abs(transitionProgress - 0.5) * 2.0;  // 0 -> 1 -> 0
    flashIntensity = pow(flashIntensity, 4.0);  // Make it sharper
    color += vec3(1.0) * flashIntensity * 0.9;  // Bright white flash

    // ==== EFFECT 4: Vignette Pulse ====
    // Darken edges during transition
    float vignette = smoothstep(0.8, 0.3, dist);
    vignette = mix(1.0, vignette, intensity * 0.6);
    color *= vignette;

    // ==== EFFECT 5: Film Grain / Noise ====
    // Add texture to the transition
    float grain = noise(uv * 800.0 + time * 50.0) * 0.1;
    color += vec3(grain) * intensity;

    // ==== EFFECT 6: Wave Distortion ====
    // Horizontal wave sweep
    float wave = sin(uv.y * 20.0 + transitionProgress * 6.28) * intensity * 0.02;
    vec3 waveColor = texture(sceneTex, uv + vec2(wave, 0.0)).rgb;
    color = mix(color, waveColor, 0.3 * intensity);

    // ==== EFFECT 7: Time Particles ====
    // Add sparkling particles during transition
    float particles = 0.0;
    for (int i = 0; i < 5; ++i) {
        float particleSeed = float(i) * 1.234;
        vec2 particlePos = vec2(
            hash(vec2(particleSeed, 0.0)),
            fract(hash(vec2(particleSeed, 1.0)) + transitionProgress * 0.5)
        );
        float particleDist = length(uv - particlePos);
        particles += smoothstep(0.02, 0.0, particleDist) * 0.3;
    }
    color += vec3(particles) * intensity;

    // ==== EFFECT 8: Color Shift ====
    // Slight color grading shift during transition
    vec3 timeShift = vec3(0.1, 0.15, 0.2);  // Slight cyan tint
    color = mix(color, color * (1.0 + timeShift), intensity * 0.3);

    // Ensure color doesn't blow out
    color = clamp(color, 0.0, 1.0);

    fragColor = vec4(color, 1.0);
}
