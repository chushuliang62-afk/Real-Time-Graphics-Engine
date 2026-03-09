#pragma once
#include "../nclgl/Vector3.h"
#include "../nclgl/Vector4.h"

class DayNightCycle {
public:
    DayNightCycle();
    ~DayNightCycle() = default;

    void Update(float dt);

    // Time control
    void SetTimeOfDay(float hours);  // 0-24
    float GetTimeOfDay() const { return timeOfDay; }
    void SetCycleSpeed(float speed) { cycleSpeed = speed; }
    float GetCycleSpeed() const { return cycleSpeed; }

    // Lighting parameters
    Vector3 GetSunDirection() const;
    Vector4 GetSunColor() const;
    Vector4 GetAmbientColor() const;

    // Sky parameters
    Vector3 GetSkyColor() const;
    Vector3 GetHorizonColor() const;

    bool IsTransitioning() const { return isTransitioning; }
    float GetTransitionProgress() const { return transitionProgress; }

private:
    float timeOfDay;           // 0-24 hours
    float cycleSpeed;          // Hours per second (default 0.1 = 6 min full cycle)

    bool isTransitioning;      // True during sunrise/sunset
    float transitionProgress;  // 0-1 during transition

    // Helper functions
    float SmoothStep(float edge0, float edge1, float x) const;
    Vector3 LerpVector3(const Vector3& a, const Vector3& b, float t) const;
    Vector4 LerpVector4(const Vector4& a, const Vector4& b, float t) const;
};
