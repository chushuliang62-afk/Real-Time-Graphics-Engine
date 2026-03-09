#pragma once

#include "Vector3.h"
#include "Vector4.h"
#include "Shader.h"

// ========================================
// LIGHT SYSTEM REFACTORED (Addressing 批评.md Section 1.1)
// ========================================
// Previous design was a simple data struct that couldn't support multiple light types.
// New design uses inheritance to support Point, Spot, and Directional lights.

// Light type enum for shader uniform uploads
enum class LightType {
    POINT = 0,
    DIRECTIONAL = 1,
    SPOT = 2
};

// Abstract base class for all lights
class Light {
public:
    Light(const Vector4& colour) : colour(colour) {}
    virtual ~Light() {}

    // Each light type must implement how it uploads its uniforms to the shader
    virtual void UploadToShader(Shader* shader, int lightIndex) const = 0;

    // Get light type for shader branching
    virtual LightType GetType() const = 0;

    // Virtual accessors for position/radius (for compatibility with shadow system)
    // Derived classes should override if they have these properties
    virtual Vector3 GetPosition() const { return Vector3(0, 0, 0); }
    virtual float GetRadius() const { return 1000.0f; }

    // Common accessors
    Vector4 GetColour() const { return colour; }
    void SetColour(const Vector4& val) { colour = val; }

protected:
    Vector4 colour;  // Light colour (RGB + intensity)
};

// Point light: emits light in all directions from a position
class PointLight : public Light {
public:
    PointLight(const Vector3& position, const Vector4& colour, float radius)
        : Light(colour), position(position), radius(radius) {}

    virtual void UploadToShader(Shader* shader, int lightIndex) const override;
    virtual LightType GetType() const override { return LightType::POINT; }

    virtual Vector3 GetPosition() const override { return position; }
    void SetPosition(const Vector3& val) { position = val; }

    virtual float GetRadius() const override { return radius; }
    void SetRadius(float val) { radius = val; }

protected:
    Vector3 position;
    float radius;
};

// Directional light: simulates distant light source (like the sun)
class DirectionalLight : public Light {
public:
    DirectionalLight(const Vector3& direction, const Vector4& colour)
        : Light(colour), direction(direction.Normalised()) {}

    virtual void UploadToShader(Shader* shader, int lightIndex) const override;
    virtual LightType GetType() const override { return LightType::DIRECTIONAL; }

    Vector3 GetDirection() const { return direction; }
    void SetDirection(const Vector3& val) { direction = val.Normalised(); }

    // For shadow mapping: compute a virtual position far away in the opposite direction
    virtual Vector3 GetPosition() const override { return -direction * 5000.0f; }

    // Directional lights don't have radius, but provide a large value for compatibility
    virtual float GetRadius() const override { return 10000.0f; }

protected:
    Vector3 direction;
};

// Spot light: emits light in a cone from a position
class SpotLight : public PointLight {
public:
    SpotLight(const Vector3& position, const Vector3& direction,
              const Vector4& colour, float radius, float cutoffAngle)
        : PointLight(position, colour, radius),
          direction(direction.Normalised()),
          cutoffAngle(cutoffAngle) {}

    virtual void UploadToShader(Shader* shader, int lightIndex) const override;
    virtual LightType GetType() const override { return LightType::SPOT; }

    Vector3 GetDirection() const { return direction; }
    void SetDirection(const Vector3& val) { direction = val.Normalised(); }

    float GetCutoffAngle() const { return cutoffAngle; }
    void SetCutoffAngle(float angle) { cutoffAngle = angle; }

protected:
    Vector3 direction;
    float cutoffAngle;  // In degrees
};
