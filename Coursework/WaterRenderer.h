#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Mesh.h"
#include "../nclgl/Light.h"
#include "../nclgl/Camera.h"
#include <memory>

/**
 * WaterRenderer - Water Surface Rendering
 *
 * Handles realistic water rendering:
 * - Reflection mapping
 * - Refraction effects
 * - Normal mapping for waves
 * - Animated water surface
 * - Fresnel effect
 */
class WaterRenderer {
public:
    WaterRenderer(OGLRenderer* renderer);
    ~WaterRenderer();

    // Initialize water resources
    bool Initialize(float waterHeight = 100.0f, float waterSize = 2000.0f);

    // Render water surface
    void Render(Light* light, Camera* camera, float time, GLuint waterTexture, GLuint bumpTexture, GLuint skyboxCubemap, const Matrix4& viewMatrix, const Matrix4& projMatrix);

    // Enable/disable water rendering
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

    // Get water properties
    float GetWaterHeight() const { return waterHeight; }
    void SetWaterHeight(float height) { waterHeight = height; }

private:
    // Reference to renderer
    OGLRenderer* renderer;

    // Enabled flag
    bool enabled;

    // Water properties
    float waterHeight;
    float waterSize;

    // Water mesh (RAII: automatic cleanup)
    std::unique_ptr<Mesh> waterMesh;

    // Water shader (RAII: automatic cleanup)
    std::unique_ptr<Shader> waterShader;

    // Water textures
    GLuint waterNormalTex;
    GLuint waterDudvTex;

    // Helper functions
    bool LoadWaterShader();
    bool LoadWaterTextures();
    void CreateWaterMesh(float size);
};
