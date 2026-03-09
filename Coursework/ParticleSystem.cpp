#include "ParticleSystem.h"
#include "RenderConfig.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

ParticleSystem::ParticleSystem(OGLRenderer* renderer, ParticleType type)
    : renderer(renderer), particleType(type),
      particleVAO(0), particleVertexVBO(0), particleInstanceVBO(0), particleTexture(0),
      particleCount(0), gravity(-9.8f), intensity(1.0f), enabled(true),
      emitterCenter(0, 0, 0), emitterSize(100, 100, 100), windVelocity(0, 0, 0) {
    // particleShader automatically initialized to nullptr by unique_ptr
}

ParticleSystem::~ParticleSystem() {
    if (particleVAO) glDeleteVertexArrays(1, &particleVAO);
    if (particleVertexVBO) glDeleteBuffers(1, &particleVertexVBO);
    if (particleInstanceVBO) glDeleteBuffers(1, &particleInstanceVBO);
    if (particleTexture) glDeleteTextures(1, &particleTexture);
}

bool ParticleSystem::Initialize(int count, const Vector3& center, const Vector3& size) {
    this->particleCount = count;
    this->emitterCenter = center;
    this->emitterSize = size;

    // Load shader
    particleShader = std::make_unique<Shader>("particle.vert", "particle.frag");
    if (!particleShader->LoadSuccess()) {
        particleShader = std::make_unique<Shader>("Shaders/particle.vert", "Shaders/particle.frag");
        if (!particleShader->LoadSuccess()) {
            particleShader = std::make_unique<Shader>("../Shaders/particle.vert", "../Shaders/particle.frag");
            if (!particleShader->LoadSuccess()) {
                std::cerr << "ERROR: Failed to load particle shader!" << std::endl;
                return false;
            }
        }
    }

    // Load particle texture
    const char* texturePath = nullptr;
    if (particleType == ParticleType::RAIN) {
        texturePath = "Textures/raindrop.png";  // We'll create a simple fallback if missing
    } else {
        texturePath = "Textures/snowflake.png";
    }

    particleTexture = SOIL_load_OGL_texture(
        texturePath,
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT
    );

    if (particleTexture == 0) {
        // Create a simple white texture as procedural fallback
        unsigned char whitePixel[4] = {255, 255, 255, 255};
        glGenTextures(1, &particleTexture);
        glBindTexture(GL_TEXTURE_2D, particleTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Initialize particle data
    InitializeParticles();

    // Create VAO and VBOs
    glGenVertexArrays(1, &particleVAO);
    glBindVertexArray(particleVAO);

    // Particle quad geometry (billboard)
    float quadVertices[] = {
        // positions      // texcoords
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 1.0f
    };

    glGenBuffers(1, &particleVertexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVertexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Vertex attributes (per-vertex, not per-instance)
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // texCoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // Instance data VBO (will be updated each frame)
    glGenBuffers(1, &particleInstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, particleInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, particleCount * sizeof(ParticleInstance), nullptr, GL_DYNAMIC_DRAW);

    // Instance attributes (per-instance)
    glEnableVertexAttribArray(2); // position
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), (void*)offsetof(ParticleInstance, position));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3); // size
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), (void*)offsetof(ParticleInstance, size));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4); // lifetime (for fade)
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), (void*)offsetof(ParticleInstance, lifetime));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);

    // Particle system initialized
    return true;
}

void ParticleSystem::InitializeParticles() {
    particles.clear();
    particles.reserve(particleCount);

    // Set physics based on particle type
    if (particleType == ParticleType::RAIN) {
        gravity = -18.0f;  // Rain falls faster (more realistic terminal velocity)
        windVelocity = Vector3(1.5f, 0.0f, 0.5f);  // Slight diagonal wind
    } else if (particleType == ParticleType::SNOW) {
        gravity = -2.0f;   // Snow falls slower
        windVelocity = Vector3(1.0f, 0.0f, 0.5f);  // More wind drift
    }

    // Initialize all particles with random positions and velocities
    for (int i = 0; i < particleCount; ++i) {
        ParticleInstance particle;

        // Random position within emitter bounds
        // X and Z centered around emitter, Y ranges from emitterCenter - half to + half
        particle.position.x = emitterCenter.x + ((rand() % 1000) / 1000.0f - 0.5f) * emitterSize.x;
        particle.position.y = emitterCenter.y + ((rand() % 1000) / 1000.0f - 0.5f) * emitterSize.y;  // Changed to -0.5f
        particle.position.z = emitterCenter.z + ((rand() % 1000) / 1000.0f - 0.5f) * emitterSize.z;

        // Velocity based on type with more variation
        if (particleType == ParticleType::RAIN) {
            // Add random velocity variation for more natural appearance
            float velocityVariation = ((rand() % 100) / 100.0f - 0.5f) * 2.0f;  // -1.0 to +1.0
            particle.velocity = Vector3(
                windVelocity.x + velocityVariation * 0.5f,
                -12.0f + velocityVariation,  // Base downward velocity with variation
                windVelocity.z + velocityVariation * 0.3f
            );
            // Smaller, more varied raindrop sizes
            particle.size = 1.0f + (rand() % 100) / 100.0f;  // 1.0 - 2.0 (smaller for more realistic density)
        } else {
            particle.velocity = Vector3(0, -1.0f, 0) + windVelocity;
            particle.size = 3.0f + (rand() % 100) / 50.0f;  // 3.0 - 5.0
        }

        particle.lifetime = (rand() % 1000) / 1000.0f * 5.0f;  // 0-5 seconds (stagger initial spawn)
        particle.randomSeed = (rand() % 1000) / 1000.0f;

        particles.push_back(particle);
    }
}

void ParticleSystem::ResetParticle(int index) {
    if (index < 0 || index >= particleCount) return;

    ParticleInstance& p = particles[index];

    // Respawn at top of emitter area
    p.position.x = emitterCenter.x + ((rand() % 1000) / 1000.0f - 0.5f) * emitterSize.x;
    p.position.y = emitterCenter.y + emitterSize.y * 0.5f;  // Top of emitter
    p.position.z = emitterCenter.z + ((rand() % 1000) / 1000.0f - 0.5f) * emitterSize.z;

    // Reset velocity with variation
    if (particleType == ParticleType::RAIN) {
        float velocityVariation = ((rand() % 100) / 100.0f - 0.5f) * 2.0f;
        p.velocity = Vector3(
            windVelocity.x + velocityVariation * 0.5f,
            -12.0f + velocityVariation,
            windVelocity.z + velocityVariation * 0.3f
        );
        p.size = 1.0f + (rand() % 100) / 100.0f;  // New random size
    } else {
        p.velocity = Vector3(0, -1.0f, 0) + windVelocity;
    }

    p.lifetime = 0.0f;
    p.randomSeed = (rand() % 1000) / 1000.0f;  // New random seed
}

void ParticleSystem::UpdateParticles(float dt) {
    for (int i = 0; i < particleCount; ++i) {
        ParticleInstance& p = particles[i];

        // Update velocity (gravity + wind)
        p.velocity.y += gravity * dt;

        // Add subtle turbulence for more natural movement
        if (particleType == ParticleType::RAIN) {
            // Use randomSeed and lifetime for consistent but varied turbulence per particle
            float turbulence = sin(p.lifetime * 3.0f + p.randomSeed * 6.28f) * 0.3f;
            p.velocity.x += turbulence * dt;
            p.velocity.z += cos(p.lifetime * 2.5f + p.randomSeed * 6.28f) * 0.2f * dt;

            // Air resistance (terminal velocity simulation)
            // As particles fall faster, air resistance increases
            float speed = sqrt(p.velocity.x * p.velocity.x +
                             p.velocity.y * p.velocity.y +
                             p.velocity.z * p.velocity.z);
            float resistance = 0.02f * speed;
            float dampingFactor = 1.0f - resistance * dt;
            p.velocity.x *= dampingFactor;
            p.velocity.y *= dampingFactor;
            p.velocity.z *= dampingFactor;
        }

        // Update position
        p.position += p.velocity * dt;

        // Update lifetime
        p.lifetime += dt;

        // Reset particle if it falls below ground (no time limit - let them fall naturally)
        float groundLevel = emitterCenter.y - emitterSize.y * 0.5f;
        if (p.position.y < groundLevel) {
            ResetParticle(i);
        }

        // Keep particles within horizontal bounds (optional, for looping effect)
        if (fabs(p.position.x - emitterCenter.x) > emitterSize.x * 0.5f) {
            p.position.x = emitterCenter.x - (p.position.x - emitterCenter.x);
        }
        if (fabs(p.position.z - emitterCenter.z) > emitterSize.z * 0.5f) {
            p.position.z = emitterCenter.z - (p.position.z - emitterCenter.z);
        }
    }
}

void ParticleSystem::Update(float dt) {
    if (!enabled) return;

    // Scale dt by intensity
    float scaledDt = dt * intensity;

    UpdateParticles(scaledDt);

    // Upload updated particle data to GPU
    glBindBuffer(GL_ARRAY_BUFFER, particleInstanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(ParticleInstance), particles.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSystem::Render(Camera* camera, float totalTime) {
    if (!enabled || !particleShader || particleCount == 0) return;

    // Enable alpha blending for particles
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Standard alpha blending
    glDepthMask(GL_FALSE);  // Don't write to depth buffer (but still test against it)

    renderer->BindShader(particleShader.get());

    // Set identity model matrix for particles (they use instance positions directly)
    renderer->SetModelMatrix(Matrix4());

    // Set uniforms
    renderer->UpdateShaderMatrices();

    glUniform1f(glGetUniformLocation(particleShader->GetProgram(), "time"), totalTime);

    // Set particleType in BOTH vertex and fragment shaders
    GLint particleTypeVert = glGetUniformLocation(particleShader->GetProgram(), "particleType");
    if (particleTypeVert >= 0) {
        glUniform1i(particleTypeVert, static_cast<int>(particleType));
    }

    glUniform1f(glGetUniformLocation(particleShader->GetProgram(), "intensity"), intensity);

    // Camera position for billboard orientation
    Vector3 camPos = camera->GetPosition();
    glUniform3fv(glGetUniformLocation(particleShader->GetProgram(), "cameraPos"),
                 1, (float*)&camPos);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, particleTexture);
    glUniform1i(glGetUniformLocation(particleShader->GetProgram(), "particleTex"), 0);

    // Draw instanced particles
    glBindVertexArray(particleVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particleCount);
    glBindVertexArray(0);

    // Restore state
    glDepthMask(GL_TRUE);
    // Depth test stays enabled, blending stays in standard mode
}
