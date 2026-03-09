#include "PointLight.h"
#include <cmath>
#include <algorithm>

void DeferredPointLight::UpdateAnimation(float deltaTime) {
    if (animType == LightAnimationType::NONE) {
        currentIntensity = intensity;
        return;
    }

    animTime += deltaTime * animSpeed;

    float modulatedValue = 0.0f;

    switch (animType) {
    case LightAnimationType::FLICKER:
        // Flicker effect: fast random variation + base sine wave
        modulatedValue = sin(animTime + animPhase) * 0.5f +
                        sin(animTime * 2.3f + animPhase * 1.7f) * 0.3f +
                        sin(animTime * 4.7f + animPhase * 2.3f) * 0.2f;
        break;

    case LightAnimationType::PULSE:
        // Pulse effect: smooth breathing
        modulatedValue = sin(animTime + animPhase);
        break;

    case LightAnimationType::RANDOM:
        // Random effect: mix multiple frequencies
        modulatedValue = sin(animTime * 0.7f + animPhase) * 0.6f +
                        sin(animTime * 1.3f + animPhase * 1.5f) * 0.4f;
        break;

    default:
        modulatedValue = 0.0f;
        break;
    }

    // Map modulatedValue (-1 to 1) to (1 - amplitude) to (1 + amplitude)
    float intensityMultiplier = 1.0f + modulatedValue * animAmplitude;

    // Ensure it doesn't become negative
    intensityMultiplier = std::max(0.0f, intensityMultiplier);

    currentIntensity = intensity * intensityMultiplier;
}

void DeferredPointLight::CalculateAttenuationFromRadius(float targetRadius) {
    // Goal: at targetRadius, attenuate to near 0 (e.g. 5% intensity)
    // Attenuation = 1 / (constant + linear*d + quadratic*d^2)
    // We want at distance = targetRadius, Attenuation ≈ 0.05
    //
    // Using common formula:
    // constant = 1.0
    // linear = 4.5 / radius
    // quadratic = 75 / (radius^2)
    //
    // At radius, attenuation is approximately 1/(1 + 4.5 + 75) ≈ 0.012 (very dim)

    radius = targetRadius;
    constant = 1.0f;
    linear = 4.5f / targetRadius;
    quadratic = 75.0f / (targetRadius * targetRadius);
}
