#pragma once

#include "RenderPass.h"
#include "FBOManager.h"

// ========================================
// POST-PROCESS PASS
// ========================================
// Renders scene texture to screen with optional effects
// This includes the time transition effect

class PostProcessPass : public ContextualRenderPass {
public:
    PostProcessPass(RenderContext* context, FBOManager* fboManager)
        : ContextualRenderPass("PostProcessPass", context),
          fboManager(fboManager), particleSystem(nullptr) {}

    virtual ~PostProcessPass() {}

    // Allow setting particle system to render after post-processing
    void SetParticleSystem(class ParticleSystem* ps) { particleSystem = ps; }

    virtual void Execute(OGLRenderer& renderer) override {
        if (!enabled || !fboManager) {
            return;
        }

        // Bind default framebuffer (screen)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        // Render scene FBO to screen with transition effect
        fboManager->RenderToScreen(
            context->isInTransition,
            context->transitionProgress
        );

        // Render particles AFTER post-processing (on top of final image)
        // This is correct for transparent effects - they should be rendered last
        if (particleSystem && context->camera) {
            particleSystem->Render(context->camera, context->totalTime);
        }
    }

private:
    FBOManager* fboManager;  // NOT owned (managed by Renderer or Pipeline)
    class ParticleSystem* particleSystem;  // NOT owned
};
