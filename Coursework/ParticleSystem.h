#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include "../nclgl/Shader.h"
#include <vector>
#include <memory>

// ========================================
// PARTICLE SYSTEM
// ========================================
// GPU-based instanced particle system for environmental effects
// Supports rain, snow, and other weather particles
// Uses instancing for efficient rendering of thousands of particles

enum class ParticleType {
    RAIN,
    SNOW,
    FOG  // Future: volumetric fog particles
};

struct ParticleInstance {
    Vector3 position;      // Current particle position
    Vector3 velocity;      // Particle velocity
    float lifetime;        // Current lifetime (0 to maxLifetime)
    float size;           // Particle size
    float randomSeed;     // Random seed for variation
};

class ParticleSystem {
public:
    ParticleSystem(OGLRenderer* renderer, ParticleType type = ParticleType::RAIN);
    ~ParticleSystem();

    bool Initialize(int particleCount, const Vector3& emitterCenter, const Vector3& emitterSize);
    void Update(float dt);
    void Render(Camera* camera, float totalTime);

    // Control
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }
    void ToggleEnabled() { enabled = !enabled; }
    void SetIntensity(float intensity) { this->intensity = intensity; }  // 0.0 to 1.0

    // Configuration
    void SetWind(const Vector3& wind) { this->windVelocity = wind; }
    void SetGravity(float gravity) { this->gravity = gravity; }

private:
    void InitializeParticles();
    void UpdateParticles(float dt);
    void ResetParticle(int index);

    OGLRenderer* renderer;
    ParticleType particleType;

    // Rendering resources
    std::unique_ptr<Shader> particleShader;
    GLuint particleVAO;
    GLuint particleVertexVBO;     // Quad geometry
    GLuint particleInstanceVBO;   // Instance data
    GLuint particleTexture;

    // Particle data (CPU side)
    std::vector<ParticleInstance> particles;
    int particleCount;

    // Emitter configuration
    Vector3 emitterCenter;        // Center of emission area
    Vector3 emitterSize;          // Size of emission area (X, Y, Z)

    // Physics parameters
    Vector3 windVelocity;         // Wind direction and strength
    float gravity;                // Gravity strength
    float intensity;              // Overall intensity (affects spawn rate, visibility)

    // State
    bool enabled;
};
