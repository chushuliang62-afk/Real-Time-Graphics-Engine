#pragma once

#include "RenderPass.h"
#include "ShadowSystem.h"

// ========================================
// SHADOW PASS
// ========================================
// Renders scene depth from light's perspective into shadow map

class ShadowPass : public ContextualRenderPass {
public:
    ShadowPass(RenderContext* context, ShadowSystem* shadowSystem)
        : ContextualRenderPass("ShadowPass", context),
          shadowSystem(shadowSystem) {}

    virtual ~ShadowPass() {}

    virtual void Execute(OGLRenderer& renderer) override {
        if (!enabled || !shadowSystem || !context->light || !context->sceneRoot) {
            return;
        }

        // Delegate to ShadowSystem (which already encapsulates shadow rendering)
        shadowSystem->RenderShadowMap(
            context->sceneRoot,
            context->light,
            context->viewMatrix
        );

        // Update context with shadow matrix for other passes
        context->shadowMatrix = shadowSystem->GetLightSpaceMatrix();
    }

private:
    ShadowSystem* shadowSystem;  // NOT owned (managed by Renderer or Pipeline)
};
