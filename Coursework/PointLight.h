#pragma once

/*
 * PointLight.h
 *
 * Point Light for Deferred Rendering
 *
 * Features:
 * - Point light data structure
 * - Support physically correct attenuation (inverse-square law)
 * - Support animation effects (flicker, pulse)
 * - Fully configurable (no hardcoded values)
 *
 * Architecture Design:
 * - Simple data structure (POD style)
 * - Easy to serialize and configure
 * - Support dynamic updates
 */

#include "../nclgl/Vector3.h"
#include "../nclgl/Vector4.h"

enum class LightAnimationType {
    NONE,       // No animation
    FLICKER,    // Flicker (flame effect)
    PULSE,      // Pulse (breathing effect)
    RANDOM      // Random intensity variation
};

struct DeferredPointLight {
    // Position and color
    Vector3 position;       // World space position
    Vector4 colour;         // RGB color + A (unused, reserved)

    // Intensity and attenuation
    float intensity;        // Light intensity (base multiplier)
    float radius;           // Effective radius (attenuation to 0 beyond this distance)

    // Physical attenuation parameters (inverse-square law: 1 / (constant + linear*d + quadratic*d^2))
    float constant;         // Constant term (usually 1.0)
    float linear;           // Linear term
    float quadratic;        // Quadratic term

    // Animation parameters
    LightAnimationType animType;    // Animation type
    float animSpeed;                // Animation speed
    float animAmplitude;            // Animation amplitude (0-1)
    float animPhase;                // Animation phase offset (for desynchronization)

    // Runtime state
    float currentIntensity;         // Current actual intensity (after animation)
    float animTime;                 // Animation time accumulator

    /**
     * Default constructor
     */
    DeferredPointLight()
        : position(0, 0, 0)
        , colour(1, 1, 1, 1)
        , intensity(1.0f)
        , radius(100.0f)
        , constant(1.0f)
        , linear(0.09f)
        , quadratic(0.032f)
        , animType(LightAnimationType::NONE)
        , animSpeed(1.0f)
        , animAmplitude(0.0f)
        , animPhase(0.0f)
        , currentIntensity(1.0f)
        , animTime(0.0f)
    {
    }

    /**
     * Full constructor
     */
    DeferredPointLight(const Vector3& pos, const Vector4& col, float intensity, float radius)
        : position(pos)
        , colour(col)
        , intensity(intensity)
        , radius(radius)
        , constant(1.0f)
        , linear(0.09f)
        , quadratic(0.032f)
        , animType(LightAnimationType::NONE)
        , animSpeed(1.0f)
        , animAmplitude(0.0f)
        , animPhase(0.0f)
        , currentIntensity(intensity)
        , animTime(0.0f)
    {
    }

    /**
     * Update light animation
     * @param deltaTime Frame time (seconds)
     */
    void UpdateAnimation(float deltaTime);

    /**
     * Calculate attenuation parameters from radius
     * Ensure light intensity attenuates to near 0 at given radius
     * @param targetRadius Target radius
     */
    void CalculateAttenuationFromRadius(float targetRadius);

    /**
     * Set animation
     * @param type Animation type
     * @param speed Speed (cycles/second)
     * @param amplitude Amplitude (0-1)
     * @param phase Phase offset (radians)
     */
    void SetAnimation(LightAnimationType type, float speed, float amplitude, float phase = 0.0f) {
        animType = type;
        animSpeed = speed;
        animAmplitude = amplitude;
        animPhase = phase;
        animTime = 0.0f;
    }
};
