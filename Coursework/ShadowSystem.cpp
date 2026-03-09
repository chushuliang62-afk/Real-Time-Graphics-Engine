#include "ShadowSystem.h"
#include "RenderConfig.h"  // For centralized constants
#include <iostream>

ShadowSystem::ShadowSystem(int shadowSize)
    : shadowSize(shadowSize), shadowFBO(0), shadowTex(0) {
    // shadowShader is automatically initialized to nullptr by unique_ptr
}

ShadowSystem::~ShadowSystem() {
    if (shadowFBO) {
        glDeleteFramebuffers(1, &shadowFBO);
    }
    if (shadowTex) {
        glDeleteTextures(1, &shadowTex);
    }
    // shadowShader is automatically deleted by unique_ptr
}

bool ShadowSystem::Initialize() {
    // Load shadow shader
    if (!LoadShadowShader()) {
        return false;
    }

    // Generate shadow FBO
    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

    // Create shadow depth texture
    glGenTextures(1, &shadowTex);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowSize, shadowSize, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Shadow texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Enable shadow comparison mode for PCF
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // Attach depth texture to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);

    // No color buffer needed for shadow pass
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check FBO completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow FBO Error: Framebuffer is not complete! Status: " << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ShadowSystem initialized
    return true;
}

bool ShadowSystem::LoadShadowShader() {
    shadowShader = std::make_unique<Shader>("shadow.vert", "shadow.frag");

    if (!shadowShader->LoadSuccess()) {
        std::cerr << "Failed to load shadow shader!" << std::endl;
        shadowShader.reset();  // Explicitly reset (automatic cleanup)
        return false;
    }

    return true;
}

void ShadowSystem::RenderShadowMap(SceneNode* root, Light* light, const Matrix4& viewMatrix) {
    if (!root || !light) return;

    // Bind shadow FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glViewport(0, 0, shadowSize, shadowSize);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Enable front face culling to reduce shadow acne
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);

    // Calculate light projection matrix (orthographic for directional light)
    float lightRange = RenderConfig::SHADOW_LIGHT_RANGE;
    Matrix4 lightProjection = Matrix4::Orthographic(-lightRange, lightRange, -lightRange, lightRange,
                                                     RenderConfig::SHADOW_NEAR_PLANE,
                                                     RenderConfig::SHADOW_FAR_PLANE);

    // Calculate light view matrix (looking from light position to origin)
    Vector3 lightPos = light->GetPosition();
    Matrix4 lightView = Matrix4::BuildViewMatrix(lightPos, Vector3(0, 0, 0));

    // Store light space matrix for shadow sampling
    lightSpaceMatrix = lightProjection * lightView;

    // Bind shadow shader
    glUseProgram(shadowShader->GetProgram());
    glUniformMatrix4fv(glGetUniformLocation(shadowShader->GetProgram(), "lightSpaceMatrix"),
                       1, false, lightSpaceMatrix.values);

    // Render scene from light's perspective
    RenderNodeShadow(root);

    // Restore culling
    glCullFace(GL_BACK);

    // Unbind shadow FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowSystem::RenderNodeShadow(SceneNode* node) {
    if (!node) return;

    if (node->GetMesh()) {
        // Set model matrix
        Matrix4 model = node->GetWorldTransform();
        glUniformMatrix4fv(glGetUniformLocation(shadowShader->GetProgram(), "modelMatrix"),
                          1, false, model.values);

        // Draw mesh
        node->GetMesh()->Draw();
    }

    // Recursively render children
    for (SceneNode* child : node->GetChildren()) {
        RenderNodeShadow(child);
    }
}

void ShadowSystem::BindShadowTexture(int textureUnit) {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
}
