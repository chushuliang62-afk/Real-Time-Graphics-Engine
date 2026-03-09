#pragma once

#include "../nclgl/Mesh.h"
#include "../nclgl/Matrix4.h"
#include "../nclgl/MeshAnimation.h"
#include <vector>

/**
 * SkeletalAnimator - Data-Driven Skeletal Animation System
 *
 * Supports both keyframe-based animation (data-driven) and procedural fallback.
 * Features:
 * - Keyframe interpolation (linear for positions, spherical for rotations)
 * - Animation state machine (Idle/Walk transitions)
 * - Smooth blending between animation states
 * - Procedural fallback if no animation data available
 */
class SkeletalAnimator {
public:
    SkeletalAnimator(Mesh* mesh);
    ~SkeletalAnimator();

    // Update animation based on elapsed time
    void Update(float time);

    // Get current frame matrices for rendering
    const std::vector<Matrix4>& GetFrameMatrices() const { return frameMatrices; }

    // Set animation data (data-driven mode)
    void SetIdleAnimation(MeshAnimation* anim) { idleAnimation = anim; }
    void SetWalkAnimation(MeshAnimation* anim) { walkAnimation = anim; }

    // Set animation parameters
    void SetAnimationSpeed(float speed) { animationSpeed = speed; }
    void SetAnimationIntensity(float intensity) { animationIntensity = intensity; }
    void SetBlendTime(float time) { blendTime = time; }

    // Movement state
    void SetPosition(const Vector3& pos) { position = pos; }
    Vector3 GetPosition() const { return position; }
    void SetRotation(float rot) { rotation = rot; }
    float GetRotation() const { return rotation; }

    // Random wandering
    void UpdateWandering(float dt, float minX, float maxX, float minZ, float maxZ);

    // Animation mode query
    bool IsDataDriven() const { return idleAnimation != nullptr || walkAnimation != nullptr; }

private:
    // Animation state enum
    enum AnimationState {
        IDLE,
        WALK
    };

    Mesh* mesh;                             // Reference to the mesh being animated
    std::vector<Matrix4> frameMatrices;     // Current frame joint transforms
    float currentTime;                       // Animation playback time
    float animationSpeed;                    // Speed multiplier (default 1.0)
    float animationIntensity;               // Intensity of movements (default 1.0)

    // Data-driven animation
    MeshAnimation* idleAnimation;            // Idle animation data (optional)
    MeshAnimation* walkAnimation;            // Walk animation data (optional)
    AnimationState currentState;             // Current animation state
    AnimationState targetState;              // Target state for blending
    float animationTime;                     // Current animation time (in seconds)
    float blendTime;                         // Blend duration between states (default 0.3s)
    float blendProgress;                     // Current blend progress (0-1)

    // Movement state
    Vector3 position;                        // Current world position
    Vector3 previousPosition;                // Position from last frame (for velocity calculation)
    float rotation;                          // Current rotation angle (degrees)
    Vector3 targetPosition;                  // Target position for wandering
    float moveSpeed;                         // Movement speed
    float timeUntilNewTarget;                // Time until choosing new target
    bool isMoving;                           // Is character currently moving?
    float currentVelocity;                   // Current movement velocity for animation sync

    // Data-driven animation functions
    void UpdateDataDrivenAnimation(float dt);
    void PlayAnimation(MeshAnimation* anim, float time, std::vector<Matrix4>& outMatrices);
    void InterpolateFrames(MeshAnimation* anim, float time, std::vector<Matrix4>& outMatrices);
    Matrix4 BlendMatrices(const Matrix4& a, const Matrix4& b, float t);

    // Procedural animation functions (fallback)
    void GenerateIdleAnimation(float time);
    void GenerateWalkAnimation(float time);
};
