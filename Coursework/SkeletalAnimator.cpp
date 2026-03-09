#include "SkeletalAnimator.h"
#include <cmath>
#include <iostream>
#include <random>

SkeletalAnimator::SkeletalAnimator(Mesh* mesh)
    : mesh(mesh), currentTime(0.0f), animationSpeed(1.0f), animationIntensity(1.0f),
      idleAnimation(nullptr), walkAnimation(nullptr),
      currentState(IDLE), targetState(IDLE), animationTime(0.0f),
      blendTime(0.3f), blendProgress(0.0f),
      position(850.0f, 0.0f, 900.0f), previousPosition(850.0f, 0.0f, 900.0f),
      rotation(0.0f), targetPosition(850.0f, 0.0f, 900.0f),
      moveSpeed(15.0f), timeUntilNewTarget(0.0f), isMoving(false), currentVelocity(0.0f) {

    // Random number generator initialized via <random>

    if (!mesh) {
        std::cerr << "ERROR: SkeletalAnimator initialized with null mesh!" << std::endl;
        return;
    }

    // Initialize frame matrices with bind pose
    int jointCount = mesh->GetJointCount();
    frameMatrices.resize(jointCount);

    if (jointCount > 0) {
        const Matrix4* bindPose = mesh->GetBindPose();
        if (bindPose) {
            for (int i = 0; i < jointCount; ++i) {
                frameMatrices[i] = bindPose[i];
            }
        }
        // Skeletal animator ready
    } else {
        std::cerr << "WARNING: Mesh has no skeletal data!" << std::endl;
    }
}

SkeletalAnimator::~SkeletalAnimator() {
    // Animations are not owned by animator, don't delete
    // Mesh is not owned by animator, don't delete
}

void SkeletalAnimator::Update(float time) {
    currentTime = time;

    // Calculate actual velocity from position change
    Vector3 displacement = position - previousPosition;
    float distance = displacement.Length();

    // Smooth velocity changes to avoid jittering
    currentVelocity = currentVelocity * 0.8f + distance * 0.2f;

    isMoving = (currentVelocity > 0.05f);

    // Update previous position for next frame
    previousPosition = position;

    // Determine target state based on movement
    targetState = isMoving ? WALK : IDLE;

    // Check if we have data-driven animation available
    if (IsDataDriven()) {
        // Use data-driven animation system
        float dt = 0.016f; // Assume ~60 FPS, will be more accurate in real implementation

        // CRITICAL: Adjust animation speed based on actual movement velocity!
        // This makes animation sync with character movement speed
        float baseWalkSpeed = 15.0f;  // Expected walk speed (from moveSpeed)

        // Calculate velocity ratio with smoother curve
        // Use square root to make animation feel more responsive at lower speeds
        float velocityRatio = 0.0f;
        if (currentVelocity > 0.01f) {
            velocityRatio = std::sqrt(currentVelocity / baseWalkSpeed);

            // Apply smoother clamping with easing near the limits
            if (velocityRatio < 0.3f) {
                velocityRatio = 0.3f;  // Minimum animation speed (slower than before)
            } else if (velocityRatio > 1.5f) {
                velocityRatio = 1.5f;  // Maximum animation speed (reduced from 2.0)
            }
        }

        // Temporarily adjust animation speed based on movement
        float originalSpeed = animationSpeed;
        animationSpeed = velocityRatio * animationIntensity;

        UpdateDataDrivenAnimation(dt);

        // Restore original speed
        animationSpeed = originalSpeed;
    } else {
        // Fallback to procedural animation
        if (isMoving) {
            GenerateWalkAnimation(currentTime * 2.5f);
        } else {
            GenerateIdleAnimation(currentTime * animationSpeed);
        }
    }
}

void SkeletalAnimator::GenerateIdleAnimation(float time) {
    if (!mesh || mesh->GetJointCount() == 0) {
        return;
    }

    int jointCount = mesh->GetJointCount();
    const Matrix4* bindPose = mesh->GetBindPose();
    const Matrix4* inverseBindPose = mesh->GetInverseBindPose();

    if (!bindPose || !inverseBindPose) {
        return;
    }

    // Simple procedural idle animation
    // Creates a breathing/bobbing effect by animating the spine and limbs

    for (int i = 0; i < jointCount; ++i) {
        Matrix4 localTransform = Matrix4(); // Identity

        // Get joint name to determine animation
        // Note: Joint indices are typically organized as:
        // 0 = Root/Hip, 1-3 = Spine, 4-6 = Head/Neck
        // 7-9 = Left Arm, 10-12 = Right Arm, etc.

        // Simple breathing animation - affects spine/chest area
        if (i >= 1 && i <= 3) {
            // Spine joints - gentle up/down motion
            float breathCycle = sin(time * 2.0f) * animationIntensity;
            float scaleY = 1.0f + breathCycle * 0.02f; // 2% scale variation
            localTransform = Matrix4::Scale(Vector3(1.0f, scaleY, 1.0f));
        }
        // Head bobbing
        else if (i >= 4 && i <= 6) {
            // Head/neck - subtle rotation
            float headBob = sin(time * 2.0f + 0.5f) * animationIntensity;
            float rotationAngle = headBob * 3.0f; // Up to 3 degrees
            localTransform = Matrix4::Rotation(rotationAngle, Vector3(1, 0, 0));
        }
        // Arm sway
        else if (i >= 7 && i <= 9) {
            // Left arm - gentle sway
            float armSway = sin(time * 1.5f) * animationIntensity;
            float rotationAngle = armSway * 5.0f; // Up to 5 degrees
            localTransform = Matrix4::Rotation(rotationAngle, Vector3(0, 0, 1));
        }
        else if (i >= 10 && i <= 12) {
            // Right arm - opposite phase sway
            float armSway = sin(time * 1.5f + 3.14159f) * animationIntensity;
            float rotationAngle = armSway * 5.0f;
            localTransform = Matrix4::Rotation(rotationAngle, Vector3(0, 0, 1));
        }

        // Combine with bind pose
        // Final transform = BindPose * LocalAnimation * InverseBindPose
        frameMatrices[i] = bindPose[i] * localTransform * inverseBindPose[i];
    }

    // Build hierarchy (apply parent transforms)
    for (int i = 0; i < jointCount; ++i) {
        int parentIndex = mesh->GetParentForJoint(i);
        if (parentIndex >= 0 && parentIndex < jointCount) {
            frameMatrices[i] = frameMatrices[parentIndex] * frameMatrices[i];
        }
    }
}

void SkeletalAnimator::GenerateWalkAnimation(float time) {
    if (!mesh || mesh->GetJointCount() == 0) {
        return;
    }

    int jointCount = mesh->GetJointCount();
    const Matrix4* bindPose = mesh->GetBindPose();
    const Matrix4* inverseBindPose = mesh->GetInverseBindPose();

    if (!bindPose || !inverseBindPose) {
        return;
    }

    // Walking animation - procedural leg and arm swinging
    float walkCycle = time * 3.0f;  // Slower walking for more natural look

    for (int i = 0; i < jointCount; ++i) {
        Matrix4 localTransform = Matrix4(); // Identity

        // Body bobbing (up and down motion while walking)
        if (i >= 1 && i <= 3) {
            // Spine - vertical bobbing (twice per walk cycle)
            float bobCycle = sin(walkCycle * 2.0f) * animationIntensity;
            float bobAmount = fabs(bobCycle) * 0.015f; // Reduced bobbing (1.5%)
            localTransform = Matrix4::Translation(Vector3(0.0f, bobAmount, 0.0f));
        }
        // Head - slight up/down nod while walking
        else if (i >= 4 && i <= 6) {
            float headNod = sin(walkCycle * 2.0f) * animationIntensity;
            float nodAngle = headNod * 1.0f; // Smaller head nod
            localTransform = Matrix4::Rotation(nodAngle, Vector3(1, 0, 0));
        }
        // ONLY animate the HIP/THIGH root joints (first joint of each leg)
        // This makes the whole leg swing naturally without bending
        else if (i == 13 || i == 14) {  // Only the very first leg joints (hips)
            bool isLeftLeg = (i == 13); // 13 = left hip, 14 = right hip

            float legSwing;
            if (isLeftLeg) {
                legSwing = sin(walkCycle) * animationIntensity;
            } else {
                legSwing = sin(walkCycle + 3.14159f) * animationIntensity; // Opposite phase
            }

            // Larger swing for more pronounced walking - 35 degrees
            float swingAngle = legSwing * 35.0f;

            // Y-axis rotation for hip (forward/backward swing for this model)
            localTransform = Matrix4::Rotation(swingAngle, Vector3(0, 1, 0));
        }
        // Optional: Add slight knee bend when leg is back (more natural)
        else if (i == 15 || i == 16) {  // Knee joints
            bool isLeftKnee = (i == 15);

            float legSwing;
            if (isLeftKnee) {
                legSwing = sin(walkCycle) * animationIntensity;
            } else {
                legSwing = sin(walkCycle + 3.14159f) * animationIntensity;
            }

            // Only bend knee when leg is going backward (negative swing)
            if (legSwing < 0.0f) {
                float kneeBend = fabs(legSwing) * 15.0f; // Slightly more knee bend to match larger swing
                localTransform = Matrix4::Rotation(-kneeBend, Vector3(0, 1, 0));  // Y-axis like hips
            }
        }
        // Left arm - opposite to left leg
        else if (i >= 7 && i <= 9) {
            float armSwing = sin(walkCycle + 3.14159f) * animationIntensity; // Opposite to leg
            float swingAngle = armSwing * 3.0f; // Reduced arm swing (from 5 to 3)
            localTransform = Matrix4::Rotation(swingAngle, Vector3(0, 0, 1)); // Z-axis
        }
        // Right arm - opposite to right leg
        else if (i >= 10 && i <= 12) {
            float armSwing = sin(walkCycle) * animationIntensity;
            float swingAngle = armSwing * 3.0f; // Reduced arm swing (from 5 to 3)
            localTransform = Matrix4::Rotation(swingAngle, Vector3(0, 0, 1)); // Z-axis
        }

        // Combine with bind pose
        frameMatrices[i] = bindPose[i] * localTransform * inverseBindPose[i];
    }

    // Build hierarchy (apply parent transforms)
    for (int i = 0; i < jointCount; ++i) {
        int parentIndex = mesh->GetParentForJoint(i);
        if (parentIndex >= 0 && parentIndex < jointCount) {
            frameMatrices[i] = frameMatrices[parentIndex] * frameMatrices[i];
        }
    }
}

// ============================================================================
// DATA-DRIVEN ANIMATION SYSTEM
// ============================================================================

void SkeletalAnimator::UpdateDataDrivenAnimation(float dt) {
    // Update animation time
    animationTime += dt * animationSpeed;

    // Handle state transitions with blending
    if (currentState != targetState) {
        // Start blending to target state
        blendProgress += dt / blendTime;

        if (blendProgress >= 1.0f) {
            // Transition complete
            currentState = targetState;
            blendProgress = 0.0f;
            animationTime = 0.0f; // Reset animation time for new state
        } else {
            // Blend between current and target animations
            std::vector<Matrix4> currentMatrices(frameMatrices.size());
            std::vector<Matrix4> targetMatrices(frameMatrices.size());

            // Get animation for current state
            MeshAnimation* currentAnim = (currentState == IDLE) ? idleAnimation : walkAnimation;
            // Get animation for target state
            MeshAnimation* targetAnim = (targetState == IDLE) ? idleAnimation : walkAnimation;

            if (currentAnim && targetAnim) {
                InterpolateFrames(currentAnim, animationTime, currentMatrices);
                InterpolateFrames(targetAnim, animationTime, targetMatrices);

                // Blend the two animations
                for (size_t i = 0; i < frameMatrices.size(); ++i) {
                    frameMatrices[i] = BlendMatrices(currentMatrices[i], targetMatrices[i], blendProgress);
                }
            }
            return;
        }
    }

    // No blending needed, just play current animation
    MeshAnimation* currentAnim = (currentState == IDLE) ? idleAnimation : walkAnimation;
    if (currentAnim) {
        InterpolateFrames(currentAnim, animationTime, frameMatrices);
    }
}

void SkeletalAnimator::InterpolateFrames(MeshAnimation* anim, float time, std::vector<Matrix4>& outMatrices) {
    if (!anim || anim->GetFrameCount() == 0) {
        return;
    }

    float frameRate = anim->GetFrameRate();
    unsigned int frameCount = anim->GetFrameCount();

    // Calculate current frame from time
    float frameFloat = time * frameRate;

    // Loop the animation
    frameFloat = fmod(frameFloat, static_cast<float>(frameCount));

    unsigned int frame0 = static_cast<unsigned int>(frameFloat);
    unsigned int frame1 = (frame0 + 1) % frameCount;

    float t = frameFloat - static_cast<float>(frame0); // Interpolation factor (0-1)

    // Get joint data for both frames
    const Matrix4* jointData0 = anim->GetJointData(frame0);
    const Matrix4* jointData1 = anim->GetJointData(frame1);

    if (!jointData0 || !jointData1) {
        return;
    }

    // Interpolate between frames
    unsigned int jointCount = anim->GetJointCount();
    for (unsigned int i = 0; i < jointCount && i < outMatrices.size(); ++i) {
        outMatrices[i] = BlendMatrices(jointData0[i], jointData1[i], t);
    }

    // CRITICAL: Apply skeletal hierarchy (parent transforms)
    // Without this, joints move independently causing the "twitching" effect!
    if (mesh) {
        for (unsigned int i = 0; i < jointCount && i < outMatrices.size(); ++i) {
            int parentIndex = mesh->GetParentForJoint(i);
            if (parentIndex >= 0 && parentIndex < (int)jointCount) {
                outMatrices[i] = outMatrices[parentIndex] * outMatrices[i];
            }
        }
    }
}

Matrix4 SkeletalAnimator::BlendMatrices(const Matrix4& a, const Matrix4& b, float t) {
    // Linear interpolation of matrix elements with re-orthogonalization
    // to prevent skew/shear from accumulating during blending
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        result.values[i] = a.values[i] * (1.0f - t) + b.values[i] * t;
    }

    // Re-orthogonalize the upper-left 3x3 rotation component
    // using Gram-Schmidt to prevent distortion from linear matrix interpolation
    Vector3 col0(result.values[0], result.values[1], result.values[2]);
    Vector3 col1(result.values[4], result.values[5], result.values[6]);
    Vector3 col2(result.values[8], result.values[9], result.values[10]);

    col0.Normalise();
    float dot01 = Vector3::Dot(col1, col0);
    col1 = col1 - col0 * dot01;
    col1.Normalise();
    col2 = Vector3::Cross(col0, col1);

    result.values[0] = col0.x; result.values[1] = col0.y; result.values[2] = col0.z;
    result.values[4] = col1.x; result.values[5] = col1.y; result.values[6] = col1.z;
    result.values[8] = col2.x; result.values[9] = col2.y; result.values[10] = col2.z;

    return result;
}

// ============================================================================
// PROCEDURAL ANIMATION SYSTEM (FALLBACK)
// ============================================================================

void SkeletalAnimator::UpdateWandering(float dt, float minX, float maxX, float minZ, float maxZ) {
    // Countdown to choosing new target
    timeUntilNewTarget -= dt;

    // Choose new random target position
    if (timeUntilNewTarget <= 0.0f) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> distX(minX, maxX);
        std::uniform_real_distribution<float> distZ(minZ, maxZ);
        std::uniform_real_distribution<float> distTime(3.0f, 8.0f);

        targetPosition = Vector3(distX(rng), 0.0f, distZ(rng));
        timeUntilNewTarget = distTime(rng);
    }

    // Move towards target
    Vector3 direction = targetPosition - position;
    direction.y = 0.0f; // Keep movement horizontal
    float distanceToTarget = direction.Length();

    // If close enough to target, consider it reached
    if (distanceToTarget < 5.0f) {
        timeUntilNewTarget = 0.0f; // Force new target next frame
        return;
    }

    // Normalize direction and move
    if (distanceToTarget > 0.0f) {
        direction.Normalise();

        // Move towards target
        position = position + (direction * moveSpeed * dt);

        // Calculate rotation to face movement direction
        // atan2 gives angle in radians, convert to degrees
        rotation = atan2(direction.x, direction.z) * (180.0f / 3.14159265f);
    }
}
