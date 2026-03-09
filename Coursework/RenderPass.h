#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include "../nclgl/SceneNode.h"

// ========================================
// RENDER PASS ARCHITECTURE
// ========================================
// This refactors the monolithic Renderer into a proper pipeline system.
// Each render pass is a self-contained unit that can be added, removed, or reordered.

// Forward declarations
class RenderPipeline;

// Abstract base class for all render passes
class RenderPass {
public:
    RenderPass(const std::string& name) : name(name), enabled(true) {}
    virtual ~RenderPass() {}

    // Each pass must implement how it renders
    virtual void Execute(OGLRenderer& renderer) = 0;

    // Optional: called once before first Execute (for initialization)
    virtual void Initialize(OGLRenderer& renderer) {}

    // Optional: called when pass is removed or pipeline is destroyed
    virtual void Cleanup(OGLRenderer& renderer) {}

    // Enable/disable pass without removing it
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

    // Get pass name for debugging
    const std::string& GetName() const { return name; }

protected:
    std::string name;
    bool enabled;
};

// ========================================
// RENDER CONTEXT
// ========================================
// Shared data accessible to all passes (avoids global state)
struct RenderContext {
    // Scene data
    Camera* camera;
    Light* light;
    SceneNode* sceneRoot;

    // Frame data
    float deltaTime;
    float totalTime;

    // World state
    bool isModern;              // Current time period
    bool isInTransition;        // Whether transitioning
    float transitionProgress;   // 0.0 to 1.0

    // FBO IDs (shared between passes)
    GLuint sceneFBO;
    GLuint sceneTexture;
    GLuint sceneDepthTexture;

    GLuint shadowFBO;
    GLuint shadowTexture;

    // View/Projection matrices (cached per frame)
    Matrix4 viewMatrix;
    Matrix4 projMatrix;
    Matrix4 shadowMatrix;

    // Window dimensions
    int screenWidth;
    int screenHeight;

    // Constructor with defaults
    RenderContext()
        : camera(nullptr), light(nullptr), sceneRoot(nullptr),
          deltaTime(0.0f), totalTime(0.0f),
          isModern(false), isInTransition(false), transitionProgress(0.0f),
          sceneFBO(0), sceneTexture(0), sceneDepthTexture(0),
          shadowFBO(0), shadowTexture(0),
          screenWidth(1280), screenHeight(720) {}
};

// ========================================
// RENDER PASS WITH CONTEXT ACCESS
// ========================================
// Base class that provides access to shared RenderContext
class ContextualRenderPass : public RenderPass {
public:
    ContextualRenderPass(const std::string& name, RenderContext* context)
        : RenderPass(name), context(context) {}

    virtual ~ContextualRenderPass() {}

protected:
    RenderContext* context;  // Shared context (NOT owned by this pass)
};
