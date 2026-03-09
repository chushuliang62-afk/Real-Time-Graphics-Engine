#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Shader.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Light.h"
#include <memory>

/**
 * ShadowSystem - Shadow Mapping Management
 *
 * Handles shadow map generation and rendering:
 * - 2048x2048 shadow map texture
 * - Light space matrix calculation
 * - Shadow pass rendering
 * - PCF soft shadows
 */
class ShadowSystem {
public:
    ShadowSystem(int shadowSize = 2048);
    ~ShadowSystem();

    // Initialize shadow resources
    bool Initialize();

    // Render shadow map from light's perspective
    void RenderShadowMap(SceneNode* root, Light* light, const Matrix4& viewMatrix);

    // Bind shadow texture for use in main rendering
    void BindShadowTexture(int textureUnit);

    // Get light space matrix for shadow sampling
    Matrix4 GetLightSpaceMatrix() const { return lightSpaceMatrix; }

    // Getters
    GLuint GetShadowFBO() const { return shadowFBO; }
    GLuint GetShadowTexture() const { return shadowTex; }
    int GetShadowSize() const { return shadowSize; }

private:
    int shadowSize;

    // Shadow FBO and texture
    GLuint shadowFBO;
    GLuint shadowTex;

    // Shadow shader (RAII: automatic cleanup)
    std::unique_ptr<Shader> shadowShader;

    // Light space transformation matrix
    Matrix4 lightSpaceMatrix;

    // Helper functions
    bool LoadShadowShader();
    void RenderNodeShadow(SceneNode* node);
};
