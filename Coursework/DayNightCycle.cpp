#include "DayNightCycle.h"
#include <algorithm>
#include <cmath>

DayNightCycle::DayNightCycle()
    : timeOfDay(12.0f), cycleSpeed(0.02f), isTransitioning(false), transitionProgress(0.0f) {
    // cycleSpeed = 0.02 means 50 seconds for full 24-hour cycle
    // Adjust for desired speed: 0.01=slower, 0.05=faster
}

void DayNightCycle::Update(float dt) {
    // Update time of day - continuous 24-hour cycle
    timeOfDay += cycleSpeed * dt;
    if (timeOfDay >= 24.0f) {
        timeOfDay -= 24.0f;
    }

    // Check if we're in transition period (sunrise/sunset)
    float sunriseStart = 5.0f, sunriseEnd = 7.0f;
    float sunsetStart = 17.0f, sunsetEnd = 19.0f;

    if (timeOfDay >= sunriseStart && timeOfDay <= sunriseEnd) {
        isTransitioning = true;
        transitionProgress = (timeOfDay - sunriseStart) / (sunriseEnd - sunriseStart);
    } else if (timeOfDay >= sunsetStart && timeOfDay <= sunsetEnd) {
        isTransitioning = true;
        transitionProgress = (timeOfDay - sunsetStart) / (sunsetEnd - sunsetStart);
    } else {
        isTransitioning = false;
        transitionProgress = 0.0f;
    }
}

void DayNightCycle::SetTimeOfDay(float hours) {
    timeOfDay = std::max(0.0f, std::min(24.0f, hours));
}

Vector3 DayNightCycle::GetSunDirection() const {
    // IMPROVED: Realistic sun path across the sky
    // Sun rises in East, peaks at South (noon), sets in West
    // Uses proper spherical coordinates for natural arc movement

    const float PI = 3.14159265f;

    // Calculate sun's elevation angle (altitude above horizon)
    // Maps: 6h=horizon(0°), 12h=zenith(90°), 18h=horizon(0°), night=below(negative)
    float hourAngle = (timeOfDay - 12.0f) * (PI / 12.0f);  // -PI at midnight to PI
    float elevation = cos(hourAngle) * (PI / 2.5f);         // Peak angle ~72° (more realistic than 90°)

    // Calculate sun's azimuth (compass direction)
    // Dawn(6h)=East(90°) -> Noon(12h)=South(180°) -> Dusk(18h)=West(270°)
    float azimuth = PI / 2.0f + hourAngle;

    // Convert spherical coordinates to Cartesian (OpenGL: Y-up, X-right, Z-forward)
    float cosElev = cos(elevation);
    float x = cosElev * cos(azimuth);  // East-West
    float y = sin(elevation);           // Up-Down
    float z = cosElev * sin(azimuth);  // North-South

    // During night, create moon by inverting sun position
    // When sun is below horizon, show moon on opposite side of sky
    if (y < -0.1f) {
        // Moon is opposite to sun - if sun is down, moon is up
        y = -y * 0.3f;  // Invert and reduce (moon doesn't go as high as sun)
        x = -x * 0.8f;  // Moon on opposite side
        z = -z * 0.8f;
    }

    // Create light direction (points FROM sun TO scene)
    Vector3 dir(-x, y, -z);
    dir.Normalise();
    return dir;
}

Vector4 DayNightCycle::GetSunColor() const {
    // IMPROVED: Natural color progression throughout 24-hour cycle
    // Based on real atmospheric scattering

    if (timeOfDay < 5.0f || timeOfDay > 21.0f) {
        // Deep night (9pm-5am): Very dark blue (moonlight)
        return Vector4(0.12f, 0.14f, 0.25f, 1.0f);
    }
    else if (timeOfDay >= 5.0f && timeOfDay < 6.0f) {
        // Pre-dawn (5am-6am): Dark blue to purple
        float t = (timeOfDay - 5.0f);
        Vector4 night(0.12f, 0.14f, 0.25f, 1.0f);
        Vector4 dawn(0.4f, 0.3f, 0.5f, 1.0f);  // Purple tint
        return LerpVector4(night, dawn, SmoothStep(0.0f, 1.0f, t));
    }
    else if (timeOfDay >= 6.0f && timeOfDay < 7.5f) {
        // Sunrise (6am-7:30am): Orange to warm yellow
        float t = (timeOfDay - 6.0f) / 1.5f;
        Vector4 orange(1.0f, 0.5f, 0.2f, 1.0f);      // Deep orange
        Vector4 yellow(1.0f, 0.9f, 0.7f, 1.0f);      // Warm yellow
        return LerpVector4(orange, yellow, SmoothStep(0.0f, 1.0f, t));
    }
    else if (timeOfDay >= 7.5f && timeOfDay < 16.5f) {
        // Midday (7:30am-4:30pm): Bright neutral white
        return Vector4(1.0f, 1.0f, 0.98f, 1.0f);
    }
    else if (timeOfDay >= 16.5f && timeOfDay < 18.5f) {
        // Sunset (4:30pm-6:30pm): Warm yellow to deep orange/red
        float t = (timeOfDay - 16.5f) / 2.0f;
        Vector4 yellow(1.0f, 0.95f, 0.8f, 1.0f);
        Vector4 sunset(1.0f, 0.4f, 0.15f, 1.0f);     // Intense orange-red
        return LerpVector4(yellow, sunset, SmoothStep(0.0f, 1.0f, t));
    }
    else if (timeOfDay >= 18.5f && timeOfDay < 20.0f) {
        // Dusk (6:30pm-8pm): Orange to purple
        float t = (timeOfDay - 18.5f) / 1.5f;
        Vector4 orange(1.0f, 0.4f, 0.15f, 1.0f);
        Vector4 purple(0.5f, 0.3f, 0.6f, 1.0f);
        return LerpVector4(orange, purple, SmoothStep(0.0f, 1.0f, t));
    }
    else {
        // Evening (8pm-9pm): Purple to night blue
        float t = (timeOfDay - 20.0f);
        Vector4 purple(0.5f, 0.3f, 0.6f, 1.0f);
        Vector4 night(0.12f, 0.14f, 0.25f, 1.0f);
        return LerpVector4(purple, night, SmoothStep(0.0f, 1.0f, t));
    }
}

Vector4 DayNightCycle::GetAmbientColor() const {
    // Ambient is always softer/darker than direct sunlight
    if (timeOfDay < 5.0f || timeOfDay > 20.0f) {
        // Night - very dark ambient
        return Vector4(0.08f, 0.08f, 0.15f, 1.0f);
    }
    else if (timeOfDay >= 5.0f && timeOfDay < 7.0f) {
        // Sunrise transition
        float t = (timeOfDay - 5.0f) / 2.0f;
        Vector4 dark(0.08f, 0.08f, 0.15f, 1.0f);
        Vector4 bright(0.4f, 0.4f, 0.45f, 1.0f);
        return LerpVector4(dark, bright, SmoothStep(0.0f, 1.0f, t));
    }
    else if (timeOfDay >= 7.0f && timeOfDay < 17.0f) {
        // Day - moderate ambient
        return Vector4(0.4f, 0.4f, 0.45f, 1.0f);
    }
    else if (timeOfDay >= 17.0f && timeOfDay < 19.0f) {
        // Sunset - warm amber ambient
        float t = (timeOfDay - 17.0f) / 2.0f;
        Vector4 bright(0.4f, 0.4f, 0.45f, 1.0f);
        Vector4 warm(0.3f, 0.25f, 0.2f, 1.0f);
        return LerpVector4(bright, warm, SmoothStep(0.0f, 1.0f, t));
    }
    else {
        // Evening to night
        float t = (timeOfDay - 19.0f);
        Vector4 warm(0.3f, 0.25f, 0.2f, 1.0f);
        Vector4 dark(0.08f, 0.08f, 0.15f, 1.0f);
        return LerpVector4(warm, dark, SmoothStep(0.0f, 1.0f, t));
    }
}

Vector3 DayNightCycle::GetSkyColor() const {
    // Sky color for atmospheric effects
    if (timeOfDay < 5.0f || timeOfDay > 20.0f) {
        return Vector3(0.02f, 0.02f, 0.08f);  // Dark night sky
    }
    else if (timeOfDay >= 5.0f && timeOfDay < 7.0f) {
        float t = (timeOfDay - 5.0f) / 2.0f;
        Vector3 dark(0.02f, 0.02f, 0.08f);
        Vector3 blue(0.5f, 0.7f, 1.0f);
        return LerpVector3(dark, blue, SmoothStep(0.0f, 1.0f, t));
    }
    else if (timeOfDay >= 7.0f && timeOfDay < 17.0f) {
        return Vector3(0.5f, 0.7f, 1.0f);  // Clear day sky
    }
    else if (timeOfDay >= 17.0f && timeOfDay < 19.0f) {
        float t = (timeOfDay - 17.0f) / 2.0f;
        Vector3 blue(0.5f, 0.7f, 1.0f);
        Vector3 orange(0.9f, 0.6f, 0.4f);
        return LerpVector3(blue, orange, SmoothStep(0.0f, 1.0f, t));
    }
    else {
        float t = (timeOfDay - 19.0f);
        Vector3 orange(0.9f, 0.6f, 0.4f);
        Vector3 dark(0.02f, 0.02f, 0.08f);
        return LerpVector3(orange, dark, SmoothStep(0.0f, 1.0f, t));
    }
}

Vector3 DayNightCycle::GetHorizonColor() const {
    // Horizon is always warmer/brighter than zenith
    if (timeOfDay < 5.0f || timeOfDay > 20.0f) {
        return Vector3(0.05f, 0.05f, 0.1f);
    }
    else if (timeOfDay >= 5.0f && timeOfDay < 7.0f) {
        float t = (timeOfDay - 5.0f) / 2.0f;
        Vector3 dark(0.05f, 0.05f, 0.1f);
        Vector3 warm(1.0f, 0.8f, 0.5f);
        return LerpVector3(dark, warm, SmoothStep(0.0f, 1.0f, t));
    }
    else if (timeOfDay >= 7.0f && timeOfDay < 17.0f) {
        return Vector3(0.85f, 0.9f, 0.95f);  // Pale horizon
    }
    else if (timeOfDay >= 17.0f && timeOfDay < 19.0f) {
        return Vector3(1.0f, 0.6f, 0.35f);  // Vibrant sunset
    }
    else {
        float t = (timeOfDay - 19.0f);
        Vector3 orange(1.0f, 0.6f, 0.35f);
        Vector3 dark(0.05f, 0.05f, 0.1f);
        return LerpVector3(orange, dark, SmoothStep(0.0f, 1.0f, t));
    }
}

float DayNightCycle::SmoothStep(float edge0, float edge1, float x) const {
    float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
    return t * t * (3.0f - 2.0f * t);
}

Vector3 DayNightCycle::LerpVector3(const Vector3& a, const Vector3& b, float t) const {
    return Vector3(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

Vector4 DayNightCycle::LerpVector4(const Vector4& a, const Vector4& b, float t) const {
    return Vector4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    );
}
